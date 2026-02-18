#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <math.h>

// ===== REQUIRED MACROS =====
#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

// ======================================================
// ===================== STEPPER ========================
// Using PORTA pins 0–3 (Mega pins 22–25)
// ======================================================

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

int stepIndex = 0;
long stepPosition = 0;

void stepper_step(uint8_t clockwise)
{
    if (clockwise)
        stepIndex++;
    else
        stepIndex--;

    if (stepIndex > 7) stepIndex = 0;
    if (stepIndex < 0) stepIndex = 7;

    PORTA = (PORTA & 0xF0) | stepSequence[stepIndex];
    _delay_ms(2);
}

void stepper_move_angle(int angle, uint8_t clockwise)
{
    long steps = (long)angle * 2048 / 360;

    for(long i = 0; i < steps; i++)
    {
        stepper_step(clockwise);

        if(clockwise) stepPosition++;
        else stepPosition--;
    }

    PORTA &= 0xF0;
}

void stepper_return_zero()
{
    if(stepPosition > 0)
        stepper_move_angle((stepPosition * 360) / 2048, 0);
    else if(stepPosition < 0)
        stepper_move_angle((-stepPosition * 360) / 2048, 1);

    stepPosition = 0;
}

// ======================================================
// ====================== SERVO =========================
// Using Timer3 – OC3C – Pin 3 (PE5)
// ======================================================

void servo_init()
{
    sbi(DDRE, DDE5);  // Pin 3 output

    // Fast PWM, ICR3 as TOP
    sbi(TCCR3A, COM3C1);
    sbi(TCCR3A, WGM31);

    sbi(TCCR3B, WGM33);
    sbi(TCCR3B, WGM32);

    // Prescaler 8
    sbi(TCCR3B, CS31);

    ICR3 = 39999;  // 20ms period (50Hz)
}

void servo_set_angle(uint8_t angle)
{
    // 1ms–2ms pulse width
    uint16_t pulse = 2000 + ((uint32_t)angle * 2000) / 180;
    OCR3C = pulse;
}

// ======================================================
// ======================== ADC =========================
// ======================================================

void adc_init()
{
    sbi(ADMUX, REFS0);  // AVcc reference

    sbi(ADCSRA, ADEN);  // Enable ADC
    sbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0); // Prescaler 128
}

uint16_t adc_read(uint8_t channel)
{
    ADMUX = (ADMUX & 0xF0) | channel;

    sbi(ADCSRA, ADSC);

    while(ADCSRA & (1 << ADSC));

    return ADC;
}

// ======================================================
// ======================== MAIN ========================
// ======================================================

int main(void)
{
    // Stepper outputs
    sbi(DDRA, DDA0);
    sbi(DDRA, DDA1);
    sbi(DDRA, DDA2);
    sbi(DDRA, DDA3);

    servo_init();
    adc_init();

    while(1)
    {
        // ===== SERVO TEST =====
        servo_set_angle(90);
        _delay_ms(3000);
        servo_set_angle(0);
        _delay_ms(2000);

        // ===== STEPPER TEST =====
        stepper_move_angle(90, 1);
        _delay_ms(3000);
        stepper_return_zero();
        _delay_ms(2000);
    }
}
