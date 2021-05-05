#include <NYG/NYG.h>

#include "main.h"

#include "Cfg.h"
#include "MicroController.h"

extern const char CSV[5] = "CSV:";

#if 0
D1 Mini PRO: 50:02:91:E9:D1:74

WeMos Mini:  

1:  48:3F:DA:74:27:D2
2:  FC:F5:C4:A9:F2:5F
3:  48:3F:DA:73:FE:BD
4:  48:3F:DA:65:CD:7D
5:  48:3F:DA:74:CC:9A
6:  48:3F:DA:73:FC:1F
7:  48:3F:DA:66:39:68

#endif //0

const char       MAC_str[][19]    = { "48:3F:DA:74:27:D2", "FC:F5:C4:A9:F2:5F" };

const uint8_t    MAC_bin[][6]    = { { 0x48,0x3F,0xDA,0x74,0x27,0xD2 }, { 0xFC,0xF5,0xC4,0xA9,0xF2,0x5F } };

const uint8_t*   pair_address = NULL;
bool             sender;
const uint8_t    channels[2] = { 1, 13 };

Counter counter, messages_sent;

bool m_connected;

void initialize_sender();
void initialize_receiver();
void proceed_sender();
void proceed_receiver();
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus);
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len);

enum IoPins
{
    BLUE_LED_PIN                = D4,
    GREEN_LED_PIN               = D6,    
    RED_LED_PIN                 = D0,  
};

RGB_Led  led(RED_LED_PIN, GREEN_LED_PIN, BLUE_LED_PIN);
Timer    timer;
Toggler  toggler;

static uint16_t st_run_counter, hold_run_counter;


struct MyCfg : public Cfg
{
    MyCfg() : Cfg("UniDirectional", 0)   {   }

private:

    struct Root : Cfg::NodeBase
    {
        struct RunCounter : Cfg::Leaf
        {
            const char* GetName()  const    { return "RC"; }

            bool SetValue(String& s)    { return convert(s.c_str(), st_run_counter); }
            String ToString()   const   { return Cfg::Leaf::ToString(st_run_counter); }
            uint32_t getSize()  const   { return sizeof(st_run_counter); }
            void*    getData()  const   { return &st_run_counter; }
        };

        RunCounter  m_run_counter;
        DECLARE_CFG_NODE_ITERATOR_FUNCS_1(m_run_counter);
    };

    NodeBase& GetRoot()     { return root; }

    Root root;
};

static MyCfg cfg;

static void wait_for_serial(const char* expected)
{
    String s;

    while(true)
    {
        while(Serial.available() == false)
        {
            led.GetRed().On();   delay(250);
            led.GetRed().Off();  delay(250);
        }

        s = Serial.readString();

        if(s.indexOf(expected) >= 0)
            break;
    }
}
void setup() 
{
    Logger::Initialize();

    LOGGER << NL;
    LOGGER << "UniDirectional" << NL << NL;
    
    if (esp_now_init() != 0) {
        LOGGER << "Error initializing ESP-NOW" << NL;
        return;
    }

    bool cfg_OK = cfg.Load();
    if(!cfg_OK)     LOGGER << "Configuration load failed" << NL;
    st_run_counter++;
    cfg.Store();

    hold_run_counter = st_run_counter;

    LOGGER << "Run counter: " << st_run_counter << NL;

    String MAC = WiFi.macAddress();
    WiFi.setOutputPower(20);

    pair_address = (MAC == MAC_str[0]) ? MAC_bin[1] : MAC_bin[0];

    sender = (MAC == MAC_str[0]);
    //sender = (MAC != MAC_str[0]);   // DELETE THIS LINE!!!

    LOGGER << "My MAC: " << MAC;

    if(sender)
        LOGGER << " - *** SENDER ***";
    else
        LOGGER << " - *** RECEIVER ***";

    LOGGER << NL;

    Serial.readString();

    wait_for_serial("GetFn");

    char fn[32];
    sprintf(fn,"FN=UD_%05d.csv", st_run_counter);

    LOGGER << fn << NL;

    wait_for_serial("Ready");

    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    if(sender)
        initialize_sender();
    else
        initialize_receiver();
}
//=================================================
void loop() 
{
    if(hold_run_counter != st_run_counter)
        return;

    if(Serial.available())
    {
        String s = Serial.readString();
        LOGGER << "Got from serial: " << s << NL;

        static uint16_t prev_run_counter = st_run_counter;

        if(cfg.DoCommand(s) && prev_run_counter != st_run_counter)
        {
            LOGGER << "Restart required" << NL;
            return;
        }
    }

    toggler.Toggle();

    if(sender)
        proceed_sender();
    else
        proceed_receiver();
}
//=================================================

//=================================================
// Combinations: channels * frequencies * sizes

#if LOG_ERRORS
const char common_report_header[] = "Type,Channel,Frequency,Size,";
#else
const char common_report_header[] = "Channel,Frequency,Size,";
#endif //LOG_ERRORS

