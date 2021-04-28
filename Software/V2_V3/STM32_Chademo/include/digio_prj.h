#ifndef PinMode_PRJ_H_INCLUDED
#define PinMode_PRJ_H_INCLUDED

#include "hwdefs.h"

/* Here you specify generic IO pins, i.e. digital input or outputs.
 * Inputs can be floating (INPUT_FLT), have a 30k pull-up (INPUT_PU)
 * or pull-down (INPUT_PD) or be an output (OUTPUT)
*/

#define DIG_IO_LIST \
    DIG_IO_ENTRY(test_in,               GPIOB, GPIO5,  PinMode::INPUT_FLT)   \
    DIG_IO_ENTRY(charger_in_1,          GPIOB, GPIO6,  PinMode::INPUT_FLT)   \
    DIG_IO_ENTRY(charger_in_2,          GPIOB, GPIO7,  PinMode::INPUT_FLT)   \
    DIG_IO_ENTRY(car_in,                GPIOB, GPIO3,  PinMode::INPUT_FLT)   \
    DIG_IO_ENTRY(extra_out,             GPIOB, GPIO14, PinMode::OUTPUT)      \
    DIG_IO_ENTRY(charge_enable_out,     GPIOB, GPIO13, PinMode::OUTPUT)      \
    DIG_IO_ENTRY(hv_enable_out,         GPIOB, GPIO12, PinMode::OUTPUT)      \
    DIG_IO_ENTRY(led_out,               GPIOC, GPIO13, PinMode::OUTPUT)

#endif // PinMode_PRJ_H_INCLUDED
