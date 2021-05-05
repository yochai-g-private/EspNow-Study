/*
 Name:		LogWriter.ino
 Created:	9/24/2020 3:20:39 PM
 Author:	YLA
*/

#include "MicroSD.h"
#include "RedGreenLed.h"
#include "Toggler.h"

static MicroSD::IFile* f;
#define LOGFILE (*f)

enum
{
    RED_LED_PIN     = D9,
    GREEN_LEN_PIN   = D7,
    SD_MOSI         = D11,
    SD_MISO         = D12,
    SD_CLK          = D13,
};

static RedGreenLed  led(RED_LED_PIN, GREEN_LEN_PIN);
static Toggler      toggler;

void setup()
{
    Logger::Initialize();

    LOGGER << "Initializing MicroSD" << NL;

    bool Ok = MicroSD::Begin();

    if (!Ok)
    {
        LOGGER << "Failed to initialize MicroSD" << NL;

        toggler.StartOnOff(led.GetRed(), 125);
        return;
    }

    LOGGER << "MicroSD successful initialized" << NL;

    LOGGER << "GetFn" << NL;

    for(;;)
    {
        while(!Serial.available())
        {
            led.GetGreen().On();  delay(250);
            led.GetGreen().Off(); delay(250);
        }

        String fn = Serial.readString();
        fn.trim();

        LOGGER << "Got from Serial: '" << fn << "'." << NL;

        if(fn.indexOf("FN="))
        {
            led.GetRed().On();  delay(250);
            led.GetRed().Off(); delay(250);

            continue;
        }

        fn = fn.substring(3);
        fn.trim();

        LOGGER << "Opening file '" << fn << "'..." << NL;

        f = MicroSD::Open(fn.c_str(), false);

        if(f)
            break;

        LOGGER << "Failed to open file '" << fn << "'." << NL;
    }
    
    LOGGER << "Ready" << NL;
}
//-------------------------------------------------------------
void loop()
{
    toggler.Toggle();

    if (f && Serial.available())
    {
        String s = Serial.readString();

        bool is_CSV = s.startsWith("CSV:");

        if(is_CSV)
        {
            s = s.substring(4);
            f->Print(s);
            f->Flush();
        }
//        Serial.print(s);
    }
}
//-------------------------------------------------------------

