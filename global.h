#ifndef GLOBAL_H
#define	GLOBAL_H

#include <xc.h>  

/**
 * Initial value of TMR0.
 *
 * CPU Clock = 2MHz
 * TMR0 countup cycle = Instruction cycle = CPU Clock / 4 = 500kHz
 * Frame rate = 60Hz
 * Subframe rate = Frame rate * 128 = 7680Hz
 * TMR0 count per subframe = TMR0 countup cycle / Subframe rate = 65
 * TMR0 initial value = 256 - TMR0 count per subframe = 191
 */
#define TMR0_INITIAL_VALUE 191

/**
 * Upper limit of subframe counter.
 * 
 */
#define SUBFRAME_COUNTER_UPPER_LIMIT 128

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * Strengths for output channel 0.
     *
     * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT].
     */
    extern volatile unsigned char outputValue0;

    /**
     * Strengths for output channel 1.
     *
     * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT].
     */

    extern volatile unsigned char outputValue1;

    /**
     * Flag to wait for frame.
     */
    extern volatile unsigned char framePending;

    /**
     * Subframe counter which counts down at every subframe.
     * 
     * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT).
     */
    extern volatile unsigned char subframeCounter;
#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* GLOBAL_H */

