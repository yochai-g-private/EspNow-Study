
// D1 Mini PRO: 50:02:91:E9:D1:74
// WeMos Mini:  48:3F:DA:74:27:D2

#include <ESP8266WiFi.h>
#include <espnow.h>

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
const char  sender_MAC_str[]    = "48:3F:DA:74:27:D2",
            receiver_MAC_str[]  = "FC:F5:C4:A9:F2:5F";

const u8    sender_MAC_bin[]    = { 0x48,0x3F,0xDA,0x74,0x27,0xD2 },
            receiver_MAC_bin[]  = { 0xFC,0xF5,0xC4,0xA9,0xF2,0x5F };
bool        is_sender;
const u8*   pair_address = NULL;

enum SendState  { SS_Unknown, SS_Failed, SS_Success };
SendState send_state;
bool message_received = false;

uint32_t myData = 778778;

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) 
{
  //Serial.print("Last Packet Send Status, ");
  if (sendStatus == 0)
  {
    //Serial.println("Message delivery success");
    send_state = SS_Success;
  }
  else{
    //Serial.println("Message delivery fail");
    send_state = SS_Failed;
  }
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) 
{
    if(!is_sender)
        Serial.println("Message received");
    message_received = true;
}

void send_message()
{
    send_state = SS_Unknown;
    message_received = false;
    esp_now_send((u8*)pair_address, (uint8_t *) &myData, sizeof(myData));
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    String MAC = WiFi.macAddress();
    WiFi.setOutputPower(20);

    is_sender = MAC == sender_MAC_str;

    if (esp_now_init() != 0) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    pair_address = (is_sender) ? receiver_MAC_bin : sender_MAC_bin;

    if(is_sender)
    {
        esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    }
    else
    {
        esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
    esp_now_add_peer((u8*)pair_address, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

    Serial.print((is_sender) ? "SENDER" : "RECEIVER");
    Serial.println(" ready!");
}

#define MAX_RETRIES 100
#define RETRY_DELAY 5

void loop() 
{
    if(is_sender)
    {
        for(int cnt = 0; cnt < MAX_RETRIES; cnt++)
        {
            //if(!(cnt % 10))
                //Serial.print(".");
                
            send_message();
            while(send_state == SS_Unknown)         delay(1);
            if(send_state == SS_Success)
            {
                for(int cnt2 = 0; cnt2 < MAX_RETRIES; cnt2++)
                    if(!message_received)           delay(1);
                    else                            break;

                if(message_received)
                    break;
            }
            else
            {
                delay(RETRY_DELAY);
            }
        }

        if(!message_received)
            Serial.println("NO ANSWER!!!");
        else if(send_state == SS_Success)
            Serial.println("Message delivery success");
        else
            Serial.println("Message delivery fail");

        delay(2000);
    }
    else
    {
        if(!message_received)
            return;

        for(int cnt = 0; cnt < MAX_RETRIES; cnt++)
        {
            send_message();
            while(send_state == SS_Unknown)         delay(1);
            if(send_state == SS_Success)
                break;
 
            delay(RETRY_DELAY);
       }
    }
}