// https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/
// https://randomnerdtutorials.com/esp-now-esp8266-nodemcu-arduino-ide/

#include <ESP8266WiFi.h>

void setup(){
  Serial.begin(115200);
  Serial.println();
  Serial.print("ESP8266 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
}
 
void loop(){

}

// D1 Mini PRO: 50:02:91:E9:D1:74
// WeMos Mini:  48:3F:DA:74:27:D2