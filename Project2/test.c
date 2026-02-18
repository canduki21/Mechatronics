#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

// Step sequence
const uint8_t seq[8] = {
    0b0001,
    0b0011,
    0b0010,
    0b0110,
    0b0100,
    0b1100,
    0b1000,
    0b1001
};

int main(void)
{
    // Set PC4–PC7 as output (Mega pins 30–33)
    sbi(DDRC, DDC4);
    sbi(DDRC, DDC5);
    sbi(DDRC, DDC6);
    sbi(DDRC, DDC7);

    while(1)
    {
        for(int i=0;i<8;i++)
        {
            // Clear PC4–PC7
            PORTC &= 0b00001111;

            // Shift sequence into PC4–PC7
            PORTC |= (seq[i] << 4);

            _delay_ms(4);
        }
    }
}
