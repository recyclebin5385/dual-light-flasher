#ifndef CONSTANT_H
#define CONSTANT_H

/**
 * Initial value of TMR0.
 *
 * CPU Clock = 1MHz
 * TMR0 countup cycle = Instruction cycle = CPU Clock / 4 = 250kHz
 * Frame rate = 100Hz
 * Subframe rate = Frame rate * 64 = 6400Hz
 * TMR0 count per subframe = TMR0 countup cycle / Subframe rate = 39
 * Cycles before TMR0 reset in isr = 6
 * Delay cycles after writing TMR0 = 2
 * TMR0 initial value = 256 - TMR0 count per subframe + Cycles before TMR0 reset in isr + Delay cycles after writing TMR0 = 225
 */
#define TMR0_INITIAL_VALUE 225

/**
 * Upper limit of subframe counter.
 *
 * Equals to maximum output level.
 */
#define SUBFRAME_COUNTER_UPPER_LIMIT 64

#endif /* CONSTANT_H */
