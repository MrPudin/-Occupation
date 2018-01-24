/*
 * occupy.h
 * Occupation - Shared Definations
 *
*/
#define DEBUG 1

#ifndef __OCCUPY_H__
#define __OCCUPY_H__ value

#define OCCUPY_RADIO_GROUP 80 /* Default Radio Group */
#define OCCUPY_RADIO_TRANSMIT_POWER 3
#define OCCUPY_RADIO_TIMEOUT 300

#define OCCUPY_IO_DATA_MAX 1000
#define OCCUPY_IO_DATA_MIN 0

#define OCCUPY_DATA_SAMPLE_SIZE 60
#define OCCUPY_DATA_SAMPLE_DELAY 80
#define OCCUPY_DATA_LIGHT_TOLERANCE 100
#define OCCUPY_DATA_LIGHT_THRESHOLD 250
#define OCCUPY_DATA_MOTION_THRESHOLD 500

#define OCCUPY_DISPLAY_MIN_BRIGHTNESS 25
#define OCCUPY_DISPLAY_SCROLL_SPEED 65

typedef enum
{
    occupy_status_unknown = 0,
    occupy_status_vacant = 1,
    occupy_status_occupied = 2,
}occupy_status;

const char *occupy_transmit_query = "^@?";
const char *occupy_transmit_ping = "^@P";
const char *occupy_transmit_ack = "^@A";


const char *occupy_storage_group = "RADIO_GROUP";
const char *occupy_storage_status = "STATUS";

/* DEBUG PRINT */
#ifdef DEBUG
#define dprint(msg) uBit.serial.printf("DEBUG: %d: %s: %s\r\n",__LINE__,__func__,msg)
#define dprintf(...) uBit.serial.printf("DEBUG: " __VA_ARGS__)
#else
#define dprint(msg)
#define dprintf(...)
// ^^ Empty Define ^^
#endif /* DEBUG PRINT */

#endif /* ifndef __OCCUPY_H__ */
