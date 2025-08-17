#ifndef __AVR__
#error "This file has to be compiled with AVR C compiler"
#endif

#ifndef __AVR_ATtiny85__
#error "Only ATtiny85 is supported"
#endif

#include <avr/interrupt.h>
#include <avr/io.h>

#define SINUSDEF static inline
#include <sinus.h>

int
main (void)
{
    while (1)
    {
    }

    return 0;
}
