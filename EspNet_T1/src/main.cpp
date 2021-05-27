#include <NYG/NYG.h>

#include "main.h"

#include "MicroController.h"
#include "Toggler.h"
#include "MACAddress.h"

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

const MACAddress MACs[] = { MACAddress(),
                            MACAddress(String("48:3F:DA:74:27:D2")), 
						    MACAddress(String("FC:F5:C4:A9:F2:5F")), 
						    MACAddress(String("48:3F:DA:73:FE:BD")), 
						    MACAddress(String("48:3F:DA:65:CD:7D")), 
						    MACAddress(String("48:3F:DA:74:CC:9A")), 
						    MACAddress(String("48:3F:DA:73:FC:1F")), 
						    MACAddress(String("48:3F:DA:66:39:68")), };

const MACAddress* SR[2] = { &MACs[1], &MACs[2] };

const uint8_t*   pair_address = NULL;
int              sender = -1;
const uint8_t    channels[2] = { 1, 13 };

Counter counter, messages_sent;

bool m_connected;

void initialize();
void deinitialize();
void send_data();
void more_logs();
void less_logs();

enum IoPins
{
    BLUE_LED_PIN                = D4,
    GREEN_LED_PIN               = D6,    
    RED_LED_PIN                 = D0,  
};

RGB_Led  led(RED_LED_PIN, GREEN_LED_PIN, BLUE_LED_PIN);

static DigitalOutputPin     builtin_led(LED_BUILTIN);
static Toggler              builtin_led_toggler;

void setup() 
{
    Logger::Initialize();

    LOGGER << NL;
    LOGGER << "---------------------------------------------------------------------------------" << NL;
    LOGGER << "EspNet_T1" << NL << NL;
    LOGGER << "---------------------------------------------------------------------------------" << NL;
    
    String MACstr = WiFi.macAddress();
    LOGGER << "My MAC: " << MACstr << NL;

    MACAddress MyMAC(MACstr);

    WiFi.setOutputPower(20);

    if(MyMAC == *SR[0])  { pair_address = *SR[1]; sender = 1; }
    else
    if(MyMAC == *SR[1])  { pair_address = *SR[0]; sender = 0; }
    else
    {
        LOGGER << "My MAC does not match SENDER or RECEIVER" << NL;
        return;
    }
    

    LOGGER << "    to: " << SR[sender]->ToString() << NL;

    LOGGER << "I am the ";
    if(sender)
        LOGGER << " SENDER";
    else
        LOGGER << " RECEIVER";

    LOGGER << NL;

    EspNet::SetDebuggingDepth(EspNet::DEBUG_APP_TASK);
    EspNet::Begin();

    builtin_led_toggler.StartOnOff(builtin_led, 1000);

    LOGGER << "Ready!" << NL;
}
//=================================================
void loop() 
{
    builtin_led_toggler.Toggle();

    if(sender < 0)
        return;

    char c = 0;

    if(Serial.available())
    {
        String s = Serial.readString();
        s.trim();

        c = *s.c_str();

        switch(c)
        {
            case '1' : initialize();    break;
            case '0' : deinitialize();  break;
            case 'd' :
            case 'D' : send_data();     break;
            case '^' : more_logs();     break;
            case 'v' : 
            case 'V' : less_logs();     break;
            default  : c = 0;           break;
        }
    }

    if(c)
        return;

    EspNet::OnLoop();
}
//---------------------------------------------
typedef uint32_t    Counter;
//---------------------------------------------
class Consumer : public EspNet::IEventConsumer
{
    void OnDataReceived(void* data, EspNet::MsgLen length)
    {
        Counter n = *((Counter*)data);
        EspNet::WriteToLog([n](ITextOutput& out) -> ITextOutput& { return out << n << " received!!!"; });
    }

    void OnConnectionEstablished(const uint16& peer_version)  const
    {
        EspNet::WriteToLog([](ITextOutput& out) -> ITextOutput& { return out << "Connection established!!!"; });
    }

    void OnConnectionLost()  const
    {
        EspNet::WriteToLog([](ITextOutput& out) -> ITextOutput& { return out << "Connection lost!!!"; });
    }
};
//---------------------------------------------
class Receiver : public Consumer
{
    const char* GetName()   const   { return "RECEIVER"; }
};
//---------------------------------------------
class Sender : public Consumer
{
    const char* GetName()   const   { return "SENDER"; }
};
//---------------------------------------------
static Consumer* consumer = NULL;
//---------------------------------------------
void initialize()
{
    if(consumer)
        return;

    if(sender)  consumer = new Sender();
    else        consumer = new Receiver();

    MACAddress pair_mac = pair_address;

    EspNet::CreateConnectionCtx cc(pair_mac, *consumer, sender ? EspNet::CLIENT : EspNet::SERVER);
    bool OK = EspNet::CreateConnection(cc);
    LOGGER << "EspNet::CreateConnection " << (OK ? "succeeded" : "failed") << NL;

    if(!OK)
        deinitialize();
}
//---------------------------------------------
void deinitialize()
{
    if(!consumer)
        return;

    delete consumer;
    consumer = NULL;
}
//---------------------------------------------
void send_data()
{
    if(!consumer)
        return;

    static Counter n = 0;
    n++;

    EspNet::WriteToLog([n](ITextOutput& out) -> ITextOutput& { return out << "sending " << n; });

    consumer->m_connection->Send(&n, sizeof(n));
}
//---------------------------------------------
void more_logs()
{
    EspNet::DbgDepth depth = EspNet::GetDebuggingDepth();
    if(depth < EspNet::__max_depth__)
    {
        depth = (EspNet::DbgDepth)(depth + 1);
        EspNet::SetDebuggingDepth(depth);
        LOGGER << "DebuggingDepth=" << depth << NL;
    }
}
//---------------------------------------------
void less_logs()
{
    EspNet::DbgDepth depth = EspNet::GetDebuggingDepth();
    if(depth > EspNet::__min_depth__)
    {
        depth = (EspNet::DbgDepth)(depth - 1);
        EspNet::SetDebuggingDepth(depth);
        LOGGER << "DebuggingDepth=" << depth << NL;
    }
}
//---------------------------------------------


