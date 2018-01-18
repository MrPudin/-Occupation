/*
 * signaler.cpp
 * Occupation - Signaler Server
*/
#include "MicroBit.h"
#include "occupy.h"


MicroBit uBit;

static int radio_group = OCCUPY_RADIO_GROUP;
static bool changing_group = false;
static occupy_status measure_status = occupy_status_unknown;

//Storage
void commit_state()
{
    uBit.storage.put(occupy_storage_group, (uint8_t *) &radio_group, sizeof(int));
    uBit.storage.put(occupy_storage_status, (uint8_t *) &measure_status, sizeof(occupy_status));
}

void load_state()
{
    KeyValuePair* group_data = uBit.storage.get(occupy_storage_group);
    KeyValuePair* status_data = uBit.storage.get(occupy_storage_status);

    if(group_data)
        memcpy(&radio_group, group_data->value, sizeof(int));
    else if(status_data)
        memcpy(&measure_status, status_data->value, sizeof(occupy_status));
}

void clear_state()
{
    uBit.storage.remove(occupy_storage_group);
    uBit.storage.remove(occupy_storage_status);
}

//IO
int read_average_u(int (*rfunc)(), int count, int udelay)
{
    long sum = 0;
    int value  = 0;
    int fail_count = 0;
    int delay;
    for(int i = 0; i < count; i++)
    {
        delay = udelay;

        value = (*rfunc)();
        if(value == -1)
        {
            i --; 
            fail_count ++;
            continue;
        }
        else if(fail_count > OCCUPY_IO_READ_FAIL_LIMIT)
        {
            dprint("Too many consecutive IO read problems, resetting Microbit...");
            commit_state();
            uBit.reset();
        }
        else 
        {
            sum += value;
            fail_count = 0;
        }

        
        for(; delay >= 1000; delay -= 1000)
            fiber_sleep(1);

        wait_us(delay);
    }

    return sum / count;
}

int read_average(int (*rfunc)(), int count, int mdelay)
{
    return read_average_u(rfunc, count, mdelay * 1000);
}

int read_motion()
{
    uBit.io.P0.setPull(PullDown);
    int sensor_value = uBit.io.P0.getAnalogValue();

    dprintf("read motion: sensor read: %d\r\n", sensor_value);

    if(sensor_value == 255) return -1;

    return sensor_value;
}

int read_light()
{
    int sensor_value = uBit.display.readLightLevel();
    //dprintf("read light: sensor read: %d\r\n", sensor_value);

    return sensor_value;
}

//Display
//Messurement
void measure_occupation()
{
    uBit.display.printChar('M');

    int motion_avg = read_average(&read_motion, OCCUPY_MEASURE_READ_COUNT,\
            OCCUPY_MEASURE_DELAY);
    
    dprintf("measure status: measure complete: %d\r\n", motion_avg);
    if(motion_avg  >= OCCUPY_MEASURE_THRESHOLD)
    {
        dprint("measure status: occupied");
        measure_status = occupy_status_occupied;
        uBit.display.printChar('O');
    }
    else 
    {
        dprint("measure status: vacant");
        measure_status = occupy_status_vacant;
        uBit.display.printChar('V');
    }
}

//Event Handlers
void onButtonAB(MicroBitEvent e)
{
    if(e.value == MICROBIT_BUTTON_EVT_HOLD)
    {
        dprint("Clearing persistent state");
        clear_state();
        
        if(!changing_group)
        {
            dprint("changing group");
            uBit.display.scroll("Change Group:");
            uBit.display.scroll(radio_group);
            changing_group = true;
        }
        else if(changing_group)
        {
            changing_group = false;
            
            dprintf("Changed Group to %d\r\n", radio_group);
            uBit.radio.setGroup(radio_group);
        }
    }
}

void onButtonA(MicroBitEvent e)
{
    if(e.value == MICROBIT_BUTTON_EVT_CLICK)
    {
        if(changing_group)
        {
            radio_group --;
            uBit.display.scroll(radio_group);
        }
    }
}


void onButtonB(MicroBitEvent e)
{
    if(e.value == MICROBIT_BUTTON_EVT_CLICK)
    {
        if(changing_group)
        {
            radio_group ++;
            uBit.display.scroll(radio_group);
        }
    }
}

//Network
void onData(MicroBitEvent e)
{
    if(e.value == MICROBIT_RADIO_EVT_DATAGRAM)
    {
        ManagedString data = uBit.radio.datagram.recv();
        dprintf("radio: recieved packet with data: %s\r\n", data.toCharArray());
        if(data == occupy_transmit_query) //Recieve Query for Room Status
        {
            dprint("radio: recieved query for room status");
            
            char send_buf[2] = {'\0', '\0'};
            send_buf[0] = '0' + measure_status;
            uBit.radio.datagram.send(ManagedString(send_buf));

            uBit.display.printChar('R');
            dprintf("radio: sent room status: %d\r\n", measure_status);
        }
        else if(data == occupy_transmit_ping) //Recieved Ping Packet
        {
            dprint("radio: ping: recieved ping packet");
            uBit.radio.datagram.send(ManagedString(occupy_transmit_ack));
            uBit.display.printChar('P');
            dprint("radio: ping: send acknowledgement packet");
        } 
        else if(data == occupy_transmit_ack) {}
        else
        {
            uBit.display.printChar('?');
            dprint("radio: Unknown packet recieved, sliently dropping...");
        }
    }
}

int main()
{
    uBit.init();
    
    load_state();

    uBit.messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_EVT_ANY, onButtonA);
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_EVT_ANY, onButtonB);
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_AB, MICROBIT_EVT_ANY, onButtonAB);
    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onData);
    
    uBit.display.readLightLevel();
    uBit.radio.setGroup(radio_group);
    uBit.radio.enable();

    dprintf("radio: set group to %d\r\n", radio_group);
    dprint("Setup Complete");
    uBit.display.printChar('I');

    while(1)
    {
        measure_occupation();
        commit_state();
    }
}
