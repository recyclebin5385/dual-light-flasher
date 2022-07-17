#include <xc.h>
#include "global.h"

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

    // update subframe count
    if (!--subframeCounter) {
        // process for each frame
        subframeCounter = SUBFRAME_COUNTER_UPPER_LIMIT;
        framePending = 0;
    }
}
