#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

// ---------------- USART ----------------
void USART_init(unsigned int ubrr)
{
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void USART_transmit(char data)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

char USART_receive()
{
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

void USART_print(const char *str)
{
    while (*str)
        USART_transmit(*str++);
}

// ---------------- STEPPER ----------------
const uint8_t stepSequence[8] = {
    0b0001,
    0b0011,
    0b0010,
    0b0110,
    0b0100,
    0b1100,
    0b1000,
    0b1001
};

int currentStepIndex = 0;
long currentPosition = 0;

void singleStep(uint8_t clockwise)
{
    if (clockwise)
        currentStepIndex++;
    else
        currentStepIndex--;

    if (currentStepIndex > 7) currentStepIndex = 0;
    if (currentStepIndex < 0) currentStepIndex = 7;

    PORTA = (PORTA & 0xF0) | stepSequence[currentStepIndex];
    _delay_ms(2);
}

void moveSteps(long steps, uint8_t clockwise)
{
    for (long i = 0; i < steps; i++)
    {
        singleStep(clockwise);

        if (clockwise)
            currentPosition++;
        else
            currentPosition--;
    }

    PORTA &= 0xF0;
}

void moveStepperAngle(int angle, uint8_t clockwise)
{
    long steps = (long)angle * 2048 / 360;
    moveSteps(steps, clockwise);
}

void returnStepperToZero()
{
    if (currentPosition > 0)
        moveSteps(currentPosition, 0);
    else if (currentPosition < 0)
        moveSteps(-currentPosition, 1);

    currentPosition = 0;
}

// ---------------- SERVO ----------------
void servo_init()
{
    DDRB |= (1 << PB5); // OC1A

    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

    ICR1 = 39999; // 50Hz
}

void servo_set_angle(uint8_t angle)
{
    uint16_t pulse = 2000 + ((uint32_t)angle * 2000) / 180;
    OCR1A = pulse;
}

// ---------------- MAIN ----------------
int main(void)
{
    DDRA |= 0x0F;   // Stepper pins
    servo_init();
    USART_init(103); // 9600 baud

    while (1)
    {
        USART_print("\r\nSelect Motor:\r\n");
        USART_print("1 - Servo\r\n");
        USART_print("2 - Stepper\r\n");

        char motorChoice = USART_receive();
        USART_transmit(motorChoice);

        USART_print("\r\nSelect Direction:\r\n");
        USART_print("1 - CW\r\n");
        USART_print("2 - CCW\r\n");

        char directionChoice = USART_receive();
        USART_transmit(directionChoice);

        uint8_t clockwise = (directionChoice == '1');

        USART_print("\r\nEnter Angle (two digits): ");

        char d1 = USART_receive();
        USART_transmit(d1);
        char d2 = USART_receive();
        USART_transmit(d2);

        char angleStr[3];
        angleStr[0] = d1;
        angleStr[1] = d2;
        angleStr[2] = '\0';

        int angle = atoi(angleStr);

        USART_print("\r\nMoving...\r\n");

        if (motorChoice == '1')  // SERVO
        {
            if (!clockwise)
                angle = 180 - angle;

            servo_set_angle(angle);
            _delay_ms(3000);
            servo_set_angle(0);
        }
        else  // STEPPER
        {
            moveStepperAngle(angle, clockwise);
            _delay_ms(3000);
            returnStepperToZero();
        }

        USART_print("Done. Returning to menu...\r\n");
        _delay_ms(2000);
    }
}
