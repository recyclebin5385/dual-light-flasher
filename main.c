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
 * CPU Clock = 4MHz
 * Instruction cycle = CPU Clock / 4 = 1MHz
 * TMR0 countup cycle = Instruction cycle / 2 = 500kHz
 * Frame rate = 60Hz
 * Subframe rate = Frame rate * 128 = 7680Hz
 * TMR0 count per subframe = TMR0 countup cycle / Subframe rate = 65
 * TMR0 initial value = 256 - 65 = 191
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
 * Initial value and increment for each output channel.
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
} ChannelValue;

/**
 * Fragment of LED flash pattern.
 */
typedef struct {
    /**
     * Number of frames for this fragment.
     */
    unsigned char length;

    /**
     * Initial value and increment for each output channel.
     */
    ChannelValue channelValues[2];
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
 * Current state of pattern progress.
 */
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
     * Subframe counter which counts down at every subframe.
     * 
     * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT).
     */
    unsigned char subframeCounter;

    /**
     * Strengths for each output channel.
     *
     * Valid range is [0, SUBFRAME_COUNTER_UPPER_LIMIT].
     */
    unsigned char channelValues[2];
} currentPatternProgress;

/**
 * GPIO for next subframe.
 */
unsigned char nextGpio;

/**
 * Counter for latent window of input which counts down at every frame.
 *
 * Input is ignored while not 0.
 */
unsigned char inputLatentFrameCounter;

void __interrupt() intr(void) {
    // reset timer interrupt
    T0IF = 0;
    TMR0 = TMR0_INITIAL_VALUE;

    // update GPIO state
    GPIO = nextGpio | (GPIO & 0b00000100);

    // update subframe count
    currentPatternProgress.subframeCounter--;

    // calculate GPIO state of the next subframe
    nextGpio = (currentPatternProgress.channelValues[0] > currentPatternProgress.subframeCounter ? 0b01 : 0b00)
            | (currentPatternProgress.channelValues[1] > currentPatternProgress.subframeCounter ? 0b10 : 0b00);

    if (currentPatternProgress.subframeCounter == 0) {
        // reset subframe count
        currentPatternProgress.subframeCounter = SUBFRAME_COUNTER_UPPER_LIMIT;
        currentPatternProgress.channelValues[0] += currentPatternProgress.pPatternFragment->channelValues[0].increment;
        currentPatternProgress.channelValues[1] += currentPatternProgress.pPatternFragment->channelValues[1].increment;

        // change pattern if GP2 input is off
        if (inputLatentFrameCounter > 0) {
            inputLatentFrameCounter--;
        } else {
            if ((GPIO & 0b00000100) == 0) {
                // if GP2 input is off, change pattern
                inputLatentFrameCounter = INPUT_LATENT_WINDOW_LENGTH;

                currentPatternProgress.pPattern++;
                if (*currentPatternProgress.pPattern == 0) {
                    currentPatternProgress.pPattern = PATTERNS;
                }
                currentPatternProgress.pPatternFragment = *currentPatternProgress.pPattern;
                currentPatternProgress.frameCounter = currentPatternProgress.pPatternFragment->length;
                currentPatternProgress.channelValues[0] = currentPatternProgress.pPatternFragment->channelValues[0].initialValue;
                currentPatternProgress.channelValues[1] = currentPatternProgress.pPatternFragment->channelValues[1].initialValue;
            }
        }

        // update frame count
        currentPatternProgress.frameCounter--;

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
                currentPatternProgress.channelValues[0] = currentPatternProgress.pPatternFragment->channelValues[0].initialValue;
                currentPatternProgress.channelValues[1] = currentPatternProgress.pPatternFragment->channelValues[1].initialValue;
            }
        }
    }
}

void main(void) {
    OSCCON = 0b01100000; // clock frequency = 4MHz, oscillator depends on FOSC
    ANSEL = 0b00000000; // no analog input
    TRISIO = 0b00000100; // GP0, 1 as output, GP2 as input
    GPIO = 0b11111111; // GPIO initialization
    CMCON0 = 0b00000111; // comparator 0-2 off
    OPTION_REG = 0b00000000; // prescaler 1:2
    WPU = 0b11111111; // pull up all inputs
    IOC = 0b00000000; // disable GP interrupt
    INTCON = 0b10100000; // enable TMR0 interrput

    TMR0 = TMR0_INITIAL_VALUE;

    currentPatternProgress.pPattern = PATTERNS;
    currentPatternProgress.pPatternFragment = *currentPatternProgress.pPattern;
    currentPatternProgress.frameCounter = currentPatternProgress.pPatternFragment->length;
    currentPatternProgress.subframeCounter = SUBFRAME_COUNTER_UPPER_LIMIT;
    currentPatternProgress.channelValues[0] = currentPatternProgress.pPatternFragment->channelValues[0].initialValue;
    currentPatternProgress.channelValues[1] = currentPatternProgress.pPatternFragment->channelValues[1].initialValue;
    nextGpio = 0;
    inputLatentFrameCounter = INPUT_LATENT_WINDOW_LENGTH;

    while (1) {
    }
}
