#include <xc.inc>

#include "constant.h"

;;; section to backup context before interrupt
psect ctxBackup,local,space=1,class=COMMON
; W register backup
w_temp:
    ds 1

; STATUS register backup
status_temp:
    ds 1

;;; section to define variable used in interrupt service routine
psect variable,global,space=1,class=BANK0

; temporary GPIO value
gpio_tmp:
    ds 1

; output channel 0 value
_outputValue0:
    ds 1

; output channel 1 value
_outputValue1:
    ds 1

; frame pending flag
_framePending:
    ds 1

; subframe counter
_subframeCounter:
    ds 1

global _outputValue0,_outputValue1,_framePending,_subframeCounter

;;; section for interrupt service routine
psect intentry,space=0,class=CODE,delta=2
_isr:
    ; -------------------------------- 
    ; save context
    ; --------------------------------

    ; saving PCLATH is not necessary for PIC12F683

    ; save W register
    movwf w_temp

    ; save status register
    swapf STATUS,w
    movwf status_temp

    ; -------------------------------- 
    ; reset TMR0 interrupt
    ; --------------------------------

    ; select bank0
    bcf STATUS, 0x5
    
    ; clear TMR0 interrupt flag
    bcf INTCON,2

    ; set TMR0 register
    movlw TMR0_INITIAL_VALUE
    movwf TMR0

    ; -------------------------------- 
    ; set GPIO value
    ; --------------------------------
    
    ; clear temporary variable
    clrf gpio_tmp

    ; if subframeCounter < outputValue0, set bit 0 of temporary variable
    movf _subframeCounter,w
    subwf _outputValue0,w
    btfsc STATUS,0
    bsf gpio_tmp,0

    ; if subframeCounter < outputValue1, set bit 1 of temporary variable
    movf _subframeCounter,w
    subwf _outputValue1,w
    btfsc STATUS,0
    bsf gpio_tmp,1

    ; move temporary variable to GPIO
    movf gpio_tmp,w
    movwf GPIO
    
    decfsz _subframeCounter, f
    goto $ + 4
    movlw SUBFRAME_COUNTER_UPPER_LIMIT
    movwf _subframeCounter
    clrf _framePending

    ; -------------------------------- 
    ; restore context
    ; --------------------------------

    ; restore status register
    ;
    swapf status_temp,w
    movwf STATUS
    
    ; restore W register
    swapf w_temp,f
    swapf w_temp,w
    
    retfie
