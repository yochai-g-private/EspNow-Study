#include <NYG/NYG.h>

#include <ESP8266WiFi.h>
#include "EspNet.h"

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

typedef uint32_t Counter;

Counter counter, messages_sent;

EspNet::IChannel* st_channel;

bool m_connected;

void SendCounter(const Counter& c)
{
    if(st_channel->Send(&c, sizeof(c)))
    {
        messages_sent++;
        LOGGER << "OK" << NL;
    }
    else
    {
        LOGGER << "FAILED" << NL;
    }
}

struct Sender : EspNet::IEventConsumer
{
    void OnDataReceived(void* data, uint8_t length)
    {
        Counter c = *((Counter*)data);
        LOGGER << "Rcvd: " << c << " / " << messages_sent << NL;
    }

    const char* GetName()   const   { return "Sender"; }

    void OnConnectionEstablished(const uint16& peer_version)  const
    {
        EspNet::IEventConsumer::OnConnectionEstablished(peer_version);
        m_connected = true;
        counter = 0;
    }

    void OnConnectionLost()  const
    {
        EspNet::IEventConsumer::OnConnectionLost();
        m_connected = false;
    }

}   st_sender;

struct Receiver : EspNet::IEventConsumer
{
    void OnConnectionEstablished(const uint16& peer_version)  const
    {
        EspNet::IEventConsumer::OnConnectionEstablished(peer_version);
        m_connected = true;
        counter = 0;
    }

    void OnConnectionLost()  const
    {
        EspNet::IEventConsumer::OnConnectionLost();
        m_connected = false;
    }

    void OnDataReceived(void* data, uint8_t length)
    {
        Counter c = *((Counter*)data);
        LOGGER << "Rcvd: " << c << NL;

        //SendCounter(c);
    }

    const char* GetName()   const   { return "Receiver"; }
}   st_receiver;

void setup() 
{
    Logger::Initialize();
    LOGGER << NL;
    LOGGER << "EspNowTest" << NL << NL;
    
    String MAC = WiFi.macAddress();
    WiFi.setOutputPower(20);

    LOGGER << "My MAC: " << MAC << NL;

    pair_address = (MAC == MAC_str[0]) ? MAC_bin[1] : MAC_bin[0];

    sender = (MAC == MAC_str[0]);

    EspNet::IEventConsumer& S = st_sender,
                          & R = st_receiver;

    EspNet::IEventConsumer& consumer = (sender) ? S : R;

    EspNet::Begin();

    st_channel = EspNet::CreateChannel(pair_address, consumer);

    if(st_channel) LOGGER << ((sender) ? "Sender" : "Receiver") << " ready" << NL;
    else           LOGGER << "Failed to initialize" << NL;
}

void loop() 
{
    if(!st_channel)
        return;

    EspNet::OnLoop();

    if(!sender)
        return;

    if(!m_connected)
        return;
    
    if(counter >= 10)
    {
        static bool done = false;

        if(!done)
        {
            done = true;
            st_channel->ShowCounters();
        }

        return;
    }

    static uint32_t prev = 0;
    uint32_t now = millis() / 1000;

    if(prev != now)
    {
#if 0
        static bool done = false;
        if(done)    return;
        done = true;
#endif
        prev = now;

        counter++;
        
        LOGGER << "Send: " << counter << " : ";

        SendCounter(counter);
    }
}
