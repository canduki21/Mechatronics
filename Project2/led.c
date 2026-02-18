#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    DDRC |= 0b11110000;   // PC4–PC7 output

    while(1)
    {
        PORTC |= (1 << PC4);
        _delay_ms(500);
        PORTC &= ~(1 << PC4);

        PORTC |= (1 << PC5);
        _delay_ms(500);
        PORTC &= ~(1 << PC5);

        PORTC |= (1 << PC6);
        _delay_ms(500);
        PORTC &= ~(1 << PC6);

        PORTC |= (1 << PC7);
        _delay_ms(500);
        PORTC &= ~(1 << PC7);
    }
}
