#include "main.h"

static uint8_t      st_channel_idx  = 0;
static uint16_t     st_frequency    = MIN_INTERVAL;
static int          st_msg_size_idx = 0;

#if PRETEST
static uint8_t      msg_sizes[3] = { MS_MAXIMAL, MS_MEDIUM, MS_SMALL };
#else
static uint8_t      msg_sizes[3] = { MS_SMALL, MS_MEDIUM, MS_MAXIMAL };
#endif

//=================================================
// Sender mesuremests:
//      Total number of messages 
//      Number of sends failed
//      Send call elapsed time
//      Callback elapsed time
//=================================================
static Msg  msg;
static bool send_ack;
static bool send_failed;
static bool between_tests;

struct SendCounters
{
    uint32_t failed;
    uint64_t send_elapsed,
             ack_elapsed;
};

static SendCounters send_counters;

static void initialize_sender_test()
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
    
    esp_now_add_peer((u8*)pair_address, ESP_NOW_ROLE_COMBO, channels[st_channel_idx], NULL, 0);
    
    memzero(send_counters);
    memzero(msg);

    msg.total_messages  = MAX_MESSAGES;
    msg.frequency       = st_frequency;
    msg.size            = msg_sizes[st_msg_size_idx];

    //LOGGER << "Size=" << msg.size << NL;

    between_tests = true;
    timer.StartOnce(GAP_BETWEEN_TESTS_SECONDS, SECS);
    toggler.StartOnOff(led.GetGreen(), 250);
}
//=================================================
void initialize_sender()
{
    LOGGER << CSV << common_report_header << "Total,Failed,Send elapsed,Callback elapsed" << NL;

    initialize_sender_test();
}
//=================================================
void proceed_sender()
{
    if(!timer.Test())
        return;

    if(between_tests)
    {
        between_tests = false;
        toggler.Stop();
    }
    
//  LOGGER << '.';
    send_ack = false;
    led.GetBlue().On();
    unsigned long before = micros();
    esp_now_send((u8*)pair_address, (uint8_t *) &msg, msg_sizes[st_msg_size_idx]);
    unsigned long after = micros();
    while(send_ack == false)
        delay(1);
    unsigned long ack = micros();
    led.GetBlue().Off();

    if(send_failed)
    {
        toggler.StartOnce(led.GetRed(), 40);

#if LOG_ERRORS
        LOGGER  << "E,"
                << msg.counter << NL;
#endif //LOG_ERRORS

        send_counters.failed++;
    }
    
    send_counters.send_elapsed += after - before;
    send_counters.ack_elapsed  += ack   - before;

    msg.counter++;

    uint32_t avg_send_elapsed = send_counters.send_elapsed / msg.counter,
             avg_ack_elapsed  = send_counters.ack_elapsed  / msg.counter;

    if(msg.counter >= MAX_MESSAGES)
    {
//      LOGGER << NL;

        LOGGER  << CSV
#if LOG_ERRORS
                << "T,"
#endif //LOG_ERRORS
                << channels[st_channel_idx] << "," 
                << msg.frequency            << ','
                << msg.size                 << ','
                << msg.total_messages       << ","
                << send_counters.failed     << ","
                << avg_send_elapsed         << ","
                << avg_ack_elapsed          << NL;

        msg.counter = 0;
        st_msg_size_idx++;

        if(st_msg_size_idx >= countof(msg_sizes))
        {
            st_msg_size_idx = 0;
            st_frequency *= 2;

            if(st_frequency > MAX_INTERVAL)
            {
                st_frequency = MIN_INTERVAL;
                st_channel_idx++;

                if(st_channel_idx >= countof(channels))
                {
                    toggler.StartOnOff(led.GetGreen(), 2000);
                    LOGGER << "TEST TERMINATED" << NL;
                    return;
                }
            }
        }

        initialize_sender_test();
    }
    else
    {
        timer.StartOnce(msg.frequency);
    }
}
//=================================================
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
    send_failed = sendStatus != 0;
    send_ack = true;
}
//=================================================

