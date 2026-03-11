#ifndef capture_event_data
#define capture_event_data




#include "stdint.h"

typedef struct scanner_event_data{

    uint8_t source_number;          //based on pwm width array
    uint8_t line_number;            //1 , 2 3 instead of gpio number 222,34 etc

}scanner_event_data_t;




#endif