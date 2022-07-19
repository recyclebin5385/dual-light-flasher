/*
 * Main code.
 */

#include <xc.h>
#include "global.h"

#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA4/OSC2/CLKOUT pin, I/O function on RA5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select bit (MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Detect (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)

/**
 * Special pattern fragment length indicating end of a pattern.
 */
#define PATTERN_FRAGMENT_LENGTH_END_OF_PATTERN 255

/**
 * Special pattern fragment length indicating the pattern fragment lasts forever.
 */
#define PATTERN_FRAGMENT_LENGTH_FOREVER 0

/**
 * Number of frames while input is ignored after an input event.
 */
#define INPUT_LATENT_WINDOW_LENGTH 60

/**
 * Definition of function whose output linearly increases for each frame.
 */
typedef struct {
    /**
     * Value for the first frame.
     *
     * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT].
     */
    unsigned char initialValue;

    /**
     * Increment of value per frame.
     *
     * Set (256 - true value) for decrement.
     */
    unsigned char increment;
} LinearFunction;

/**
 * Fragment of LED flash pattern.
 */
typedef struct {
    /**
     * Number of frames for this fragment.
     */
    unsigned char length;

    /**
     * Function defining each output channel value for each frame.
     */
    LinearFunction outputFunctions[2];
} FlashPatternFragment;

/**
 * LED flash pattern which always turns on LEDs.
 */
static const FlashPatternFragment ALWAYS_ON_PATTERN[] = {
    {PATTERN_FRAGMENT_LENGTH_FOREVER,
        {
            {64, 0},
            {64, 0}
        }}
};
/**
 * LED flash pattern which turns on and off LEDs mutually.
 */
static const FlashPatternFragment MUTUAL_TRIANGLE_PATTERN[] = {
    {32,
        {
            {0, 2},
            {64, 254}
        }},
    {32,
        {
            {64, 254},
            {0, 2}
        }},
    {PATTERN_FRAGMENT_LENGTH_END_OF_PATTERN}
};

/**
 * LED flash pattern which turns on and off LEDs at the same time.
 */
static const FlashPatternFragment SYNCHRONIZED_TRIANGLE_PATTERN[] = {
    {32,
        {
            {0, 2},
            {0, 2}
        }},
    {32,
        {
            {64, 254},
            {64, 254}
        }},
    {PATTERN_FRAGMENT_LENGTH_END_OF_PATTERN}
};

/**
 * LED flash pattern which turns on and off LEDs mutually.
 */
static const FlashPatternFragment MUTUAL_SAW_PATTERN[] = {
    {32,
        {
            {32, 255},
            {64, 255}
        }},
    {32,
        {
            {64, 255},
            {32, 255}
        }},
    {PATTERN_FRAGMENT_LENGTH_END_OF_PATTERN}
};

/**
 * LED flash pattern which turns on and off LEDs at the same time.
 */
static const FlashPatternFragment SYNCHRONIZED_SAW_PATTERN[] = {
    {64,
        {
            {64, 255},
            {64, 255}
        }},
    {PATTERN_FRAGMENT_LENGTH_END_OF_PATTERN}
};

/**
 * LED flash pattern which turns on and off LEDs mutually.
 */
static const FlashPatternFragment MUTUAL_SQUARE_PATTERN[] = {
    {32,
        {
            {0, 0},
            {64, 0}
        }},
    {32,
        {
            {64, 0},
            {0, 0}
        }},
    {PATTERN_FRAGMENT_LENGTH_END_OF_PATTERN}
};

/**
 * LED flash pattern which turns on and off LEDs at the same time.
 */
static const FlashPatternFragment SYNCHRONIZED_SQUARE_PATTERN[] = {
    {32,
        {
            {64, 0},
            {64, 0}
        }},
    {32,
        {
            {0, 0},
            {0, 0}
        }},
    {PATTERN_FRAGMENT_LENGTH_END_OF_PATTERN}
};

/**
 * List of LED flash pattern.
 */
static const FlashPatternFragment* PATTERNS[] = {
    ALWAYS_ON_PATTERN,
    MUTUAL_TRIANGLE_PATTERN,
    SYNCHRONIZED_TRIANGLE_PATTERN,
    MUTUAL_SAW_PATTERN,
    SYNCHRONIZED_SAW_PATTERN,
    MUTUAL_SQUARE_PATTERN,
    SYNCHRONIZED_SQUARE_PATTERN,
    0 /* end of array */
};

#define PATTERN_COUNT 7

__EEPROM_DATA(0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);

/**
 * Main routine.
 */
void main(void) {
    // initialize registers
    OSCCON = 0b01000000; // clock frequency = 1MHz, oscillator depends on FOSC
    ANSEL = 0b00000000; // no analog input
    TRISIO = 0b00000100; // GP0, 1 as output, GP2 as input
    GPIO = 0b00000000; // GPIO initialization
    CMCON0 = 0b00000111; // comparator 0-2 off
    OPTION_REG = 0b00001000; // GPIO pullup enabled, TMR0 source is internal instruction cycle clock without prescaler
    WPU = 0b00000100; // pull up GP2 input
    IOC = 0b00000000; // disable GP interrupt

    // initialize global variables
    outputValue0 = 0;
    outputValue1 = 0;
    framePending = 1;
    subframeCounter = SUBFRAME_COUNTER_UPPER_LIMIT;

    //current pattern index
    unsigned char patternIndex = EEPROM_READ(0);
    if (patternIndex > PATTERN_COUNT) {
        patternIndex = 0;
    }

    // current state of pattern progress

    struct {
        /** Current pattern. */
        const FlashPatternFragment** pPattern;

        /** Current pattern fragment. */
        const FlashPatternFragment* pPatternFragment;

        /**
         * Frame counter which counts down at every frame.
         *
         * If the value becomes 0, moves to next pattern fragment.
         */
        unsigned char frameCounter;

        /**
         * Strengths for each output channel.
         *
         * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT].
         */
        unsigned char outputValues[2];
    } currentPatternProgress = {
        PATTERNS + patternIndex,
        PATTERNS[patternIndex],
        PATTERNS[patternIndex]->length, {
            PATTERNS[patternIndex]->outputFunctions[0].initialValue,
                    PATTERNS[patternIndex]->outputFunctions[1].initialValue,
        }
    };

    // Counter for latent window of input which counts down at every frame
    // input is ignored while not 0
    unsigned char inputLatentFrameCounter = INPUT_LATENT_WINDOW_LENGTH;

    // enable TMR0
    TMR0 = TMR0_INITIAL_VALUE;
    INTCON = 0b10100000; // enable TMR0 interrput

    while (1) {
        while (framePending) {
        }

        // process for each frame

        framePending = 1;

        // set current output channel values
        outputValue0 = currentPatternProgress.outputValues[0];
        outputValue1 = currentPatternProgress.outputValues[1];

        // update frame count
        currentPatternProgress.frameCounter--;
        currentPatternProgress.outputValues[0] += currentPatternProgress.pPatternFragment->outputFunctions[0].increment;
        currentPatternProgress.outputValues[1] += currentPatternProgress.pPatternFragment->outputFunctions[1].increment;

        // change pattern if button is pressed
        if (inputLatentFrameCounter > 0) {
            inputLatentFrameCounter--;
        } else {
            if ((GPIO & 0b00000100) == 0) {
                inputLatentFrameCounter = INPUT_LATENT_WINDOW_LENGTH;

                patternIndex++;
                if (patternIndex > PATTERN_COUNT) {
                    patternIndex = 0;
                }
                EEPROM_WRITE(0, patternIndex);

                currentPatternProgress.pPattern++;
                if (*currentPatternProgress.pPattern == 0) {
                    currentPatternProgress.pPattern = PATTERNS;
                }
                currentPatternProgress.pPatternFragment = *currentPatternProgress.pPattern;
                currentPatternProgress.frameCounter = currentPatternProgress.pPatternFragment->length;
                currentPatternProgress.outputValues[0] = currentPatternProgress.pPatternFragment->outputFunctions[0].initialValue;
                currentPatternProgress.outputValues[1] = currentPatternProgress.pPatternFragment->outputFunctions[1].initialValue;
            }
        }

        if (currentPatternProgress.frameCounter == 0) {
            if (currentPatternProgress.pPatternFragment->length != PATTERN_FRAGMENT_LENGTH_FOREVER) {
                // move to next pattern fragment
                currentPatternProgress.pPatternFragment++;
                switch (currentPatternProgress.pPatternFragment->length) {
                    case PATTERN_FRAGMENT_LENGTH_END_OF_PATTERN:
                        currentPatternProgress.pPatternFragment = *currentPatternProgress.pPattern;
                        break;

                    default:
                        break;
                }
                currentPatternProgress.frameCounter = currentPatternProgress.pPatternFragment->length;
                currentPatternProgress.outputValues[0] = currentPatternProgress.pPatternFragment->outputFunctions[0].initialValue;
                currentPatternProgress.outputValues[1] = currentPatternProgress.pPatternFragment->outputFunctions[1].initialValue;
            }
        }
    }
}
