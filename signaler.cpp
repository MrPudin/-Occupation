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


int read_average_u(int (*rfunc)(), int count, int udelay)
{
    long sum = 0;
    dprintf("delay: %d\r\n", udelay);
    for(int i = 0; i < count; i++)
    {
        sum += (*rfunc)();
        
        if(udelay >= 1000)
        {
            fiber_sleep(1);
            udelay -= 1000;
        }
        else wait_us(udelay);
    }

    return sum / count;
}

int read_average(int (*rfunc)(), int count, int mdelay)
{
    return read_average_u(rfunc, count, mdelay * 1000);
}

int read_motion()
{
    int sensor_value = uBit.io.P0.getAnalogValue();
    //dprintf("read motion: sensor read: %d\r\n", sensor_value);

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
    }
    else 
    {
        dprint("measure status: vacant");
        measure_status = occupy_status_vacant;
    }
}

//Event Handlers
void onButtonAB(MicroBitEvent e)
{
    if(e.value == MICROBIT_BUTTON_EVT_HOLD)
    {
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

    uBit.messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_EVT_ANY, onButtonA);
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_EVT_ANY, onButtonB);
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_AB, MICROBIT_EVT_ANY, onButtonAB);
    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onData);
    

    uBit.radio.setGroup(radio_group);
    uBit.radio.enable();
    
    uBit.display.readLightLevel();
    uBit.display.printChar('I');

    
    while(1)
    {
        measure_occupation();
    }
}
