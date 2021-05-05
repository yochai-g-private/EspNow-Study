#include "main.h"

static uint8_t st_channel_idx  = 0;

//=================================================
// Receiver mesuremests:
//      Expected number of messages 
//      Number of messages lost
//      Number of invalid messages
//=================================================

struct ReceiverCounters
{
    uint32_t    lost;
    uint32_t    invalid;
    uint32_t    overlapped;
};

static ReceiverCounters     receiver_counters;
static Header               prev_received;
static Header               received;
static uint8_t              received_length;
static unsigned long        last_received_at;
static bool                 message_received;
//=================================================
static void initialize_receiver_test()
{
    static bool remove = false;

    if(remove)
    {
        esp_now_del_peer((u8*)pair_address);
        delay(500);
    }
    else
    {
        remove = true;
    }
    
    if(prev_received.frequency >= MAX_INTERVAL)
    {
        st_channel_idx++;
        if(st_channel_idx >= countof(channels))
        {
            toggler.StartOnOff(led.GetGreen(), 1000);
            return;
        }
    }
    
    esp_now_add_peer((u8*)pair_address, ESP_NOW_ROLE_COMBO, channels[st_channel_idx], NULL, 0);
    
    memzero(prev_received);
    memzero(receiver_counters);

    received_length = 0;
    message_received = false;
    last_received_at = 0;
}
//=================================================
void initialize_receiver()
{
    LOGGER << common_report_header << "Total,Lost,Invalid,Overlapped" << NL;

    initialize_receiver_test();
}
//=================================================
static void receiver_test_done()
{
    if(prev_received.size == 0)
    {
        // TODO
        return;
    }

    receiver_counters.lost += prev_received.total_messages - prev_received.counter;
    
        LOGGER  
#if LOG_ERRORS
                << "T,"
#endif //LOG_ERRORS
                << channels[st_channel_idx]     << "," 
                << prev_received.frequency      << ','
                << prev_received.size           << ','
                << prev_received.total_messages << ","
                << receiver_counters.lost       << ","
                << receiver_counters.invalid    << ","
                << receiver_counters.overlapped << NL;

    initialize_receiver_test();
}
//=================================================
void proceed_receiver()
{
    if(!message_received)
    {
        if(last_received_at)
        {
            unsigned long now       = millis();
            unsigned long elapsed   = now - last_received_at;

            if(elapsed >= (((GAP_BETWEEN_TESTS_SECONDS / 10) * 9) * 1000))
            {
                receiver_test_done();
            }
        }   
        
        return;
    }

    message_received = false;
    led.GetBlue().Off();

    if(received_length != received.size)
    {
        received_length = 0;
        receiver_counters.invalid++;

#if LOG_ERRORS
        LOGGER << "E," << received.counter << NL;
#endif //LOG_ERRORS
        return;
    }

    if(memequal<Header>(&received, &prev_received))
    {
        prev_received.counter++;
        return;
    }

    uint32_t lost = 0;

    if(prev_received.counter > received.counter)
    {
        receiver_test_done();
        lost = prev_received.total_messages - prev_received.counter;
    }
    else
    {
        lost = prev_received.total_messages - prev_received.counter;
    }

    if(lost)
    {
        toggler.StartOnce(led.GetRed(), 40);
        receiver_counters.lost += lost;
    }

    prev_received = received;
    prev_received.counter++;
}
//=================================================
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len)
{
    Header* header = (Header*)incomingData;

    if(received_length)
    {
        receiver_counters.overlapped++;
#if LOG_ERRORS
        LOGGER << "T," << header->counter << NL;
#endif //LOG_ERRORS
        return;
    }

    last_received_at = millis();

    received_length = len;
    memcpy(&received, header, sizeof(Header));

    led.GetBlue().On();

    message_received = true;
}
//=================================================

