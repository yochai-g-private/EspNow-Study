#include <ESP8266WiFi.h>

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

const char  MAC_str[][19]    = { "48:3F:DA:74:27:D2", "FC:F5:C4:A9:F2:5F" };

const u8    MAC_bin[][6]    = { { 0x48,0x3F,0xDA,0x74,0x27,0xD2 }, { 0xFC,0xF5,0xC4,0xA9,0xF2,0x5F } };

const u8*   pair_address = NULL;

void setup() 
{
    Serial.begin(115200);
    Serial.println();

    String MAC = WiFi.macAddress();
    WiFi.setOutputPower(20);

    pair_address = (MAC == MAC_str[0]) ? MAC_bin[1] : MAC_bin[0];
}

void loop() 
{
}
