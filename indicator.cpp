/*
 * indicator.cpp
 * Occupation - Indicator
 * 
*/

#include "MicroBit.h"
#include "occupy.h"

MicroBit uBit;

typedef enum
{
    transmit_status_null = 0,
    transmit_status_querying = 1,
    transmit_status_pinging = 2,
}transmit_status;

static int radio_group = OCCUPY_RADIO_GROUP;
static bool changing_group = false;
static transmit_status tstatus = transmit_status_null;
static occupy_status room_status = occupy_status_unknown;
static bool ping_ack = false;

//Storage
void commit_state()
{
    uBit.storage.put(occupy_storage_group, (uint8_t *) &radio_group, sizeof(int));
}

void load_state()
{
    KeyValuePair* group_data = uBit.storage.get(occupy_storage_group);

    if(group_data)
        memcpy(&radio_group, group_data->value, sizeof(int));
}

void clear_state()
{
    uBit.storage.remove(occupy_storage_group);
}

//Display
static MicroBitImage display_image_occupied("\
        0 1 1 1 0\n\
        1 1 1 1 1\n\
        1 0 0 0 1\n\
        1 1 1 1 1\n\
        0 1 1 1 0\n");

static MicroBitImage display_image_vacant("\
        0 0 0 0 0\n\
        0 0 0 0 1\n\
        0 0 0 1 0\n\
        1 0 1 0 0\n\
        0 1 0 0 0\n");

void calibrate_display_ambient()
{
    uBit.display.clear();
    int ambient_light = uBit.display.readLightLevel();
    uBit.display.setBrightness(ambient_light);
}

//Networking
bool network_ping()
{
    uBit.radio.enable();
    ping_ack = false;
    tstatus = transmit_status_pinging;
    
    uBit.radio.datagram.send(ManagedString(occupy_transmit_ping));
    dprint("radio: transmitted ping, waiting for response.");

    fiber_sleep(OCCUPY_RADIO_TIMEOUT);
    
    if(ping_ack)
    { 
        dprint("radio: got acknowledgement for ping."); 
    }
    else 
    {
        dprint("radio: Failed to get response for query.");
    }
    
    tstatus = transmit_status_null;
    uBit.radio.disable();

    return ping_ack;
}
    
occupy_status network_query_status()
{
    uBit.radio.enable();
    room_status = occupy_status_unknown;
    tstatus = transmit_status_querying;

    uBit.radio.datagram.send(ManagedString(occupy_transmit_query));
    dprint("Query Status: transmitted query for occupy status, waiting for response.");

    fiber_sleep(OCCUPY_RADIO_TIMEOUT);
    
    if(room_status != occupy_status_unknown)
    {
        dprint("Query Status: got response for query.");
    }
    else
    {
        dprint("Query Status: Failed to get response for query."); 
    }

    tstatus = transmit_status_null;
    uBit.radio.disable();

    return room_status;
}

void onData(MicroBitEvent e)
{
    if(e.value == MICROBIT_RADIO_EVT_DATAGRAM)
    {
        ManagedString data = uBit.radio.datagram.recv();
        dprintf("radio: recieved packet with data: %s\r\n", data.toCharArray());

        //Sliently Ignore transmition not meant for indicators.
        if(data == occupy_transmit_query || data == occupy_transmit_ping){}
        //Recieved Server Acknowledge
        else if(data == occupy_transmit_ack && tstatus == transmit_status_pinging) 
        {
            ping_ack = true;
            dprint("radio: recieved acknowledgement packet");
        }
        else if(tstatus == transmit_status_querying)
        {
            //Data Check
            const char *cdata = data.toCharArray();
            bool data_valid = true;
            for(int i = 0; i < data.length() - 1; i ++) //Dont Check \0 Bytr
            {
                uBit.serial.printf("cdata[i]: %c", cdata[0]);
                if(!isdigit(cdata[i])) data_valid = false;
            }

            if(data_valid) 
            {
                dprint("radio: recieved query reply packet");
                room_status = (occupy_status)atoi(cdata);
            }
        }
        else { dprint("radio: Unknown packet recieved, sliently dropping..."); }
    }
}

//Display
void display_occupancy()
{
    occupy_status status = network_query_status();
    if(status == occupy_status_occupied)
    {
        calibrate_display_ambient();
        uBit.display.print(display_image_occupied);
        uBit.sleep(1000);
        uBit.display.scroll("Occupied",OCCUPY_DISPLAY_SCROLL_SPEED);
        uBit.display.clear();
    }
    else if(status == occupy_status_vacant)
    {
        calibrate_display_ambient();
        uBit.display.print(display_image_vacant);
        uBit.sleep(1000);
        uBit.display.scroll("Vacant",OCCUPY_DISPLAY_SCROLL_SPEED);
        uBit.display.clear();
    }
    else
    {
        calibrate_display_ambient();
        uBit.display.print("?");
        uBit.sleep(1000);
        uBit.display.scroll("Unknown",OCCUPY_DISPLAY_SCROLL_SPEED);
        uBit.display.clear();
    }

}


//Event Handlers
void onButtonAB(MicroBitEvent e)
{
    if(e.value == MICROBIT_BUTTON_EVT_HOLD)
    {
        clear_state();
        if(!changing_group)
        {
            dprint("changing group");
            uBit.display.scroll("Change Group:",OCCUPY_DISPLAY_SCROLL_SPEED);
            uBit.display.scroll(radio_group,OCCUPY_DISPLAY_SCROLL_SPEED);
            changing_group = true;
        }
        else if(changing_group)
        {
            changing_group = false;

            uBit.radio.setGroup(radio_group);

            uBit.display.scroll("Group:", OCCUPY_DISPLAY_SCROLL_SPEED);
            uBit.display.scroll(radio_group, OCCUPY_DISPLAY_SCROLL_SPEED);
            dprintf("Changed Group to %d\r\n", radio_group);
            commit_state();
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
            uBit.display.scroll(radio_group,OCCUPY_DISPLAY_SCROLL_SPEED);
        }
        else display_occupancy();
    }
}

void onButtonB(MicroBitEvent e)
{
    if(e.value == MICROBIT_BUTTON_EVT_CLICK)
    {
        if(changing_group)
        {
            radio_group ++;
            uBit.display.scroll(radio_group,OCCUPY_DISPLAY_SCROLL_SPEED);
        }
        else
        {
            if(!network_ping()) uBit.display.scroll("NO SIGNAL",OCCUPY_DISPLAY_SCROLL_SPEED);
            else uBit.display.scroll("OK",OCCUPY_DISPLAY_SCROLL_SPEED);
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

    uBit.io.P0.setPull(PullDown);

    uBit.display.readLightLevel();
    calibrate_display_ambient();

    //Setup Interfaces
    uBit.radio.setGroup(radio_group);
    uBit.radio.disable();
    
    dprintf("radio: set group to %d\r\n", radio_group);
    dprint("Setup Complete");
    
    release_fiber();
}

