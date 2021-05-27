#pragma once

#include <ESP8266WiFi.h>
#include "EspNet.h"

#include "RGB_Led.h"
#include "Timer.h"
#include "Toggler.h"

#if 1
#define ESPNET_TRACE()     LOGGER << __FUNCTION__ << " : " << __LINE__ << NL
#else
#define ESPNET_TRACE()
#endif

#define LOG_ERRORS      0

extern const uint8_t* pair_address;

typedef uint32_t Counter;

#define PRETEST 1

#if PRETEST
enum { MAX_MESSAGES                 =   10,
       GAP_BETWEEN_TESTS_SECONDS    =   10,
       MAX_INTERVAL                 = 1000,
       MIN_INTERVAL                 =  250,
       MAX_ESP_NOW_MESSAGE_LENGTH   =  250 };
#else
enum { MAX_MESSAGES                 = 1000,
       GAP_BETWEEN_TESTS_SECONDS    =   60,
       MAX_INTERVAL                 = 2000,
       MIN_INTERVAL                 =  125,
       MAX_ESP_NOW_MESSAGE_LENGTH   =  250 };
#endif

extern const uint8_t channels[2];
extern const char common_report_header[];

extern RGB_Led  led;
extern Timer    timer;
extern Toggler  toggler;

//=================================================
// TEST PARAMETERS
// Channel: 1, 13
// Send frequency:  2000, 1000, 500, 250, 125
// Message size:    
//      small:  Counter + Frequency
//      medium: Counter + Frequency + filler up to 125
//      maximal:Counter + Frequency + filler up to 250

struct TestParams
{
    uint32_t total_messages;
    uint16_t frequency;
    uint8_t  size;
};

struct Header : TestParams
{
    Counter counter;
};

struct Msg : Header
{
    char filler[MAX_ESP_NOW_MESSAGE_LENGTH - (sizeof(Header) + 2)];
};

enum MsgSize    { MS_SMALL      = sizeof(Header), 
                  MS_MEDIUM     = sizeof(Msg) / 2, 
                  MS_MAXIMAL    = sizeof(Msg) };


extern const char CSV[5];
