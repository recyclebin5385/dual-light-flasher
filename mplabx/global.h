#ifndef GLOBAL_H
#define	GLOBAL_H

#include <xc.h>  

#include "constant.h"

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

    /**
     * Strengths for output channel 0.
     *
     * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT].
     */
    __bank(0) extern volatile unsigned char outputValue0;

    /**
     * Strengths for output channel 1.
     *
     * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT].
     */

    __bank(0) extern volatile unsigned char outputValue1;

    /**
     * Flag to wait for frame.
     */
    __bank(0) extern volatile unsigned char framePending;

    /**
     * Subframe counter which counts down at every subframe.
     * 
     * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT).
     */
    __bank(0) extern volatile unsigned char subframeCounter;
#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* GLOBAL_H */

