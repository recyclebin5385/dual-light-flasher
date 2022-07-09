/*
 * Main code.
 */

#include <xc.h>

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
 */
#define SUBFRAME_COUNTER_UPPER_LIMIT 128

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

// pattern which turns on and off LEDs mutually
const FlashPatternFragment MUTUAL_PATTERN[] = {
    {16,
        {
            {0, 8},
            {128, 248}
        }},
    {16,
        {
            {128, 248},
            {0, 8}
        }},
    {PATTERN_FRAGMENT_LENGTH_END_OF_PATTERN}
};

// pattern which turns on and off LEDs at the same time
const FlashPatternFragment SYNCHRONIZED_PATTERN[] = {
    {16,
        {
            {0, 8},
            {0, 8}
        }},
    {16,
        {
            {128, 248},
            {128, 248}
        }},
    {PATTERN_FRAGMENT_LENGTH_END_OF_PATTERN}
};

// pattern which always turns on LEDs
const FlashPatternFragment ALWAYS_ON_PATTERN[] = {
    {PATTERN_FRAGMENT_LENGTH_FOREVER,
        {
            {128, 0},
            {128, 0}
        }}
};

const FlashPatternFragment* PATTERNS[] = {
    MUTUAL_PATTERN,
    SYNCHRONIZED_PATTERN,
    ALWAYS_ON_PATTERN,
    0 /* end of array */
};

/**
 * Strengths for each output channel.
 *
 * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT].
 */
volatile unsigned char outputValues[2];

/**
 * Flag to wait for frame.
 */
volatile unsigned char framePending;

/**
 * Subframe counter which counts down at every subframe.
 * 
 * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT).
 */
volatile unsigned char subframeCounter;

void __interrupt() intr(void) {
    // reset timer interrupt
    T0IF = 0;
    TMR0 = TMR0_INITIAL_VALUE;

    // update GPIO state
    if (outputValues[0] > subframeCounter) {
        GPIObits.GP0 = 1;
    } else {
        GPIObits.GP0 = 0;
    }

    if (outputValues[1] > subframeCounter) {
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

void main(void) {
    OSCCON = 0b01010000; // clock frequency = 2MHz, oscillator depends on FOSC
    ANSEL = 0b00000000; // no analog input
    TRISIO = 0b00000100; // GP0, 1 as output, GP2 as input
    GPIO = 0b00000000; // GPIO initialization
    CMCON0 = 0b00000111; // comparator 0-2 off
    OPTION_REG = 0b00001000; // GPIO pullup enabled, TMR0 source is internal instruction cycle clock without prescaler
    WPU = 0b00000100; // pull up GP2 input
    IOC = 0b00000000; // disable GP interrupt
    INTCON = 0b10100000; // enable TMR0 interrput

    TMR0 = TMR0_INITIAL_VALUE;

    outputValues[0] = 0;
    outputValues[1] = 0;
    framePending = 1;
    subframeCounter = SUBFRAME_COUNTER_UPPER_LIMIT;

    // Current state of pattern progress.

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
        PATTERNS,
        PATTERNS[0],
        PATTERNS[0]->length, {
            PATTERNS[0]->outputFunctions[0].initialValue,
                    PATTERNS[0]->outputFunctions[1].initialValue,
        }
    };

    // Counter for latent window of input which counts down at every frame.
    // Input is ignored while not 0.
    unsigned char inputLatentFrameCounter = INPUT_LATENT_WINDOW_LENGTH;

    while (1) {
        while (framePending) {
        }

        // process for each frame

        framePending = 1;

        // set current output channel values
        outputValues[0] = currentPatternProgress.outputValues[0];
        outputValues[1] = currentPatternProgress.outputValues[1];

        // change pattern if button is pressed
        if (inputLatentFrameCounter > 0) {
            inputLatentFrameCounter--;
        } else {
            if ((GPIO & 0b00000100) == 0) {
                inputLatentFrameCounter = INPUT_LATENT_WINDOW_LENGTH;

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

        // update frame count
        currentPatternProgress.frameCounter--;
        currentPatternProgress.outputValues[0] += currentPatternProgress.pPatternFragment->outputFunctions[0].increment;
        currentPatternProgress.outputValues[1] += currentPatternProgress.pPatternFragment->outputFunctions[1].increment;

        if (currentPatternProgress.frameCounter == 0) {
            if (currentPatternProgress.pPatternFragment != PATTERN_FRAGMENT_LENGTH_FOREVER) {
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
