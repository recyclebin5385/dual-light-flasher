#include <xc.h>
#include "global.h"

#define QUOTE2(s) #s

/**
 * Quote number literal.
 */
#define QUOTE(s) QUOTE2(s)

/**
 * Interrupt sevice routine.
 */
void __interrupt() isr(void) {
    // reset timer interrupt
    T0IF = 0;
    TMR0 = TMR0_INITIAL_VALUE;

    // update GPIO state
    if (outputValue0 > subframeCounter) {
        GPIObits.GP0 = 1;
    } else {
        GPIObits.GP0 = 0;
    }

    if (outputValue1 > subframeCounter) {
        GPIObits.GP1 = 1;
    } else {
        GPIObits.GP1 = 0;
    }

    // count down subframeCounter
    // if subframeCounter = 0, set maximum value to it and
    // reset frame pending flag 
    __asm("DECFSZ _subframeCounter, 1");
    __asm("GOTO $ + 4");
    __asm("MOVLW " QUOTE(SUBFRAME_COUNTER_UPPER_LIMIT));
    __asm("MOVWF _subframeCounter");
    __asm("CLRF _framePending");
}
