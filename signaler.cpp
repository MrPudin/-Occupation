/*
 * signaler.cpp
 * Occupation - Signaler Server
*/
#include <cmath>
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

// Read Functions: Return value between 0 -1024 or -1 
// Returns -1 if read has error
int read_motion() //PIR motion sensor on Pin 0
{
    int sensor_value = uBit.io.P0.getAnalogValue();
    if(sensor_value == 255) return -1;

    //dprintf("read motion: sensor read: %d\r\n", sensor_value);

    return sensor_value;
}

int read_light() //Light sensor on Pin 1
{
    int sensor_value = uBit.io.P1.getAnalogValue();
    if(sensor_value == 255) return -1;

    //dprintf("read light: sensor read: %d\r\n", sensor_value);
    return sensor_value;
}

//Data 
int comp_mean(int *data, int len)
{
    int sum = 0;
    for(int i = 0; i < len; i++)
    {
        sum += data[i];
    }
  
    return sum / len;
}

double comp_std_dev(int *data, int len, int mean=0)
{
    if(mean == 0) mean = comp_mean(data, len);

    int sum = 0, diff;
    for(int i = 0; i < len; i ++)
    {
        diff =  data[i]  - mean;
        sum += (diff * diff);
    }

    double sq_dev = (double) sum / (double) len;
    return sqrt(sq_dev);
}

void gather_data(int (*rfunc)(), int *data,int len, int delay_ms)
{
    int rst = 0;
    int cdelay; 
    for(int i = 0; i < len; i++)
    {
        cdelay = delay_ms;
        rst = (*rfunc)();
        if(rst == -1)
        {
            i--;
            continue;
        }

        data[i] = rst;
    
        //Wait delay
        for(; cdelay >= 6; cdelay -= 6) uBit.sleep(6);
        wait_ms(cdelay);
    }
    
    dprintf("gather_data(): Gathered data of size: %d\r\n",len);
}

void __listen_data__(void *input)
{
    //Expand Args
    void **args = (void **)input;
    int (*rfunc)() =  (int (*)()) args[0];
    void (*callback)(int *,int) = (void (*)(int *,int)) args[1];

    int *data = new int[OCCUPY_DATA_SAMPLE_SIZE];
    
    while(1)
    {
        memset(data, 0, sizeof(int) * OCCUPY_DATA_SAMPLE_SIZE); //Clear Data
        gather_data(rfunc, data, OCCUPY_DATA_SAMPLE_SIZE, OCCUPY_DATA_SAMPLE_DELAY);
        
        (*callback)(data, OCCUPY_DATA_SAMPLE_SIZE);
    }
}

void listen_data(int (*rfunc)(), void (*callback)(int *,int))
{
    void **input = new void *[2];
    input[0] = (void *)rfunc;
    input[1] = (void *)callback;
 
    create_fiber(&__listen_data__, input);
}

//Callbacks
bool motion_status = false;
bool light_status = false;
bool sound_status = false;

void light_callback(int *data, int len)
{
    static int prev_dev = -1;
    static int light_level = OCCUPY_DATA_LIGHT_THRESHOLD;
    static bool first_data = true;

    if(first_data) 
    {
        first_data = false;
        return;
    }

    if(!light_status)
    {
        int dev = comp_std_dev(data, len);
        dprintf("light_callback(): Computed standard deviation: %d\r\n", dev);
        
        if(prev_dev != -1) //Not the first computation
        {
            dprintf("light(): Computed standard deviation diff: %d\r\n", abs(dev-prev_dev));
            if(abs(dev - prev_dev) > OCCUPY_DATA_LIGHT_TOLERANCE) 
            {
                light_status = true;
                light_level = max(comp_mean(data, len), OCCUPY_DATA_LIGHT_THRESHOLD);
            }
            else
            {
                light_status = false;
            }
        }
        
        prev_dev = (light_status) ? -1 : dev;
    }
    else
    {
        int mean = comp_mean(data, len);

        dprintf("light_status(): Computed mean: %d\r\n", mean);

        if(mean < light_level)
        {
            light_status = false;
            light_level = OCCUPY_DATA_MOTION_THRESHOLD;
        }
    }
    dprintf("light_callback(): light detection result: %d\r\n", (int)light_status);
}

void motion_callback(int *data, int len)
{
    motion_status = false;
    for(int i = 0; i < len; i++)
    {
        if(data[i] > OCCUPY_DATA_MOTION_THRESHOLD)
        {
            motion_status = true;
            break;
        }
    }
    dprintf("motion_callback(): motion detection result: %d\r\n", (int)motion_status);
}

void update_measure_status()
{
    if(!motion_status & !light_status && !sound_status)
    {
        measure_status = occupy_status_vacant; 
        uBit.display.printChar('V');
        //dprint(" Measure Status: Vacant");
    }
    else if(motion_status || light_status || sound_status)
    {
        measure_status = occupy_status_occupied; 
        uBit.display.printChar('O');
        //dprint(" Measure Status: Occupied ");
    }
    else 
    {
        measure_status = occupy_status_unknown;
        uBit.display.printChar('?');
        //dprint(" Measure Status: Unknown ");
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
            commit_state();
            dprintf("Changed Group to %d\r\n", radio_group);
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
    uBit.radio.setTransmitPower(OCCUPY_RADIO_TRANSMIT_POWER);
    uBit.radio.enable();

    dprintf("radio: set group to %d\r\n", radio_group);
    dprint("Setup Complete");
    
    if(read_light() > OCCUPY_DATA_LIGHT_THRESHOLD) light_status = true;
    
    listen_data(&read_light, &light_callback);

    uBit.sleep(uBit.random(2000));
    listen_data(&read_motion, &motion_callback);

    
    uBit.display.printChar('I');
    
    while(1)
    {
        update_measure_status();
        uBit.sleep(1000 * 1);

        if(uBit.systemTime() > 1000 * 60 * 60 * 6)
        {
            uBit.reset();
        }
    }
}
