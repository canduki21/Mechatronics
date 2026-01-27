#define F_CPU 16000000UL  // 16 MHz for Arduino Uno/Nano

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>

// Global variables
volatile uint32_t stopwatch_ticks = 0;
volatile bool running = false;

// 7-segment font (abcdefg)
const uint8_t seg_map[10] = {
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F  // 9
};

void set_segments(uint8_t value, bool show_decimal)
{
  // For COMMON CATHODE: Invert! LOW = ON, HIGH = OFF
  
  // Segments A-F on A0-A5 (PC0-PC5)
  for (int i = 0; i < 6; i++)
  {
    if (value & (1 << i))
      PORTC &= ~(1 << i);  // Clear bit (LOW = ON)
    else
      PORTC |= (1 << i);   // Set bit (HIGH = OFF)
  }
  
  // Segment G on D6 (PD6)
  if (value & (1 << 6))
    PORTD &= ~(1 << PD6);  // LOW = ON
  else
    PORTD |= (1 << PD6);   // HIGH = OFF
  
  // Decimal point on D7 (PD7)
  if (show_decimal)
    PORTD &= ~(1 << PD7);  // LOW = ON
  else
    PORTD |= (1 << PD7);   // HIGH = OFF
}

void disable_digits(void)
{
  // For COMMON CATHODE: LOW to disable
  PORTD &= ~((1 << PD0) | (1 << PD1));  // D0, D1 LOW
  PORTB &= ~((1 << PB2) | (1 << PB3));  // D10, D11 LOW
}

void enable_digit(uint8_t d)
{
  disable_digits();
  // For COMMON CATHODE: HIGH to enable
  switch(d)
  {
    case 0: PORTD |= (1 << PD0); break;  // D0 (digit 1)
    case 1: PORTD |= (1 << PD1); break;  // D1 (digit 2)
    case 2: PORTB |= (1 << PB2); break;  // D10 (digit 3)
    case 3: PORTB |= (1 << PB3); break;  // D11 (digit 4)
  }
}

void display_time(uint32_t ticks)
{
  uint16_t ms = ticks % 100;
  uint16_t sec = (ticks / 100) % 60;

  uint8_t digits[4] = {
    sec / 10,
    sec % 10,
    ms / 10,
    ms % 10
  };

  for (uint8_t i = 0; i < 4; i++)
  {
    set_segments(seg_map[digits[i]], i == 1);
    enable_digit(i);
    _delay_ms(2);
  }
}

// Timer1 Compare Match A interrupt (10ms interval)
ISR(TIMER1_COMPA_vect)
{
  if (running)
  {
    stopwatch_ticks++;
  }
}

void init_timer(void)
{
  // Timer1 in CTC mode for 10ms interrupt
  // Prescaler = 64, OCR1A = 2499
  // (16MHz / 64) / 2500 = 100 Hz (10ms)
  
  TCCR1B |= (1 << WGM12);                    // CTC mode
  TCCR1B |= (1 << CS11) | (1 << CS10);       // Prescaler = 64
  OCR1A = 2499;                              // Compare value
  TIMSK1 |= (1 << OCIE1A);                   // Enable compare interrupt
}

void init_gpio(void)
{
  // PORTC (A0-A5) as outputs for segments A-F
  DDRC |= 0x3F;  // Set PC0-PC5 as outputs
  PORTC |= 0x3F; // All HIGH (segments OFF for common cathode inverted)
  
  // PORTD setup
  DDRD |= (1 << PD0) | (1 << PD1);           // D0, D1 as outputs (digits 1, 2)
  DDRD |= (1 << PD6) | (1 << PD7);           // D6, D7 as outputs (segment G, decimal)
  PORTD &= ~((1 << PD0) | (1 << PD1));       // Digits LOW (OFF)
  PORTD |= (1 << PD6) | (1 << PD7);          // Segment G and decimal HIGH (OFF)
  
  // PORTB setup
  DDRB |= (1 << PB2) | (1 << PB3);           // D10, D11 as outputs (digits 3, 4)
  PORTB &= ~((1 << PB2) | (1 << PB3));       // Digits LOW (OFF)
  
  DDRB &= ~(1 << PB4);                       // D12 as input (button)
  PORTB |= (1 << PB4);                       // Enable pull-up on button
}

int main(void)
{
  init_gpio();
  init_timer();
  
  sei();  // Enable global interrupts
  
  // Button handling variables
  bool last_button_state = true;  // Pull-up = HIGH when not pressed
  
  while (1)
  {
    display_time(stopwatch_ticks);
    
    // Simple button polling with debounce
    bool button_state = (PINB & (1 << PB4)) == 0;  // LOW when pressed
    if (button_state && !last_button_state)
    {
      _delay_ms(50);  // Debounce
      if ((PINB & (1 << PB4)) == 0)  // Still pressed
      {
        running = !running;
      }
    }
    last_button_state = button_state;
  }
}
avrdude -c arduino -p m328p -P COM3 -b 115200 -U flash:w:stopwatch_avr.hex:i
avrdude.exe -c arduino -p m328p -P COM3 -b 115200 -U flash:w:"C:\Users\candela\Documents\Atmel Studio\7.0\GccApplication1\Debug\GccApplication1.hex"
  C:\Users\candela\AppData\Local\Arduino15\packages\arduino\tools\avrdude\8.0.0-arduino1\bin>avrdude.exe -c arduino -p m328p -P COM3 -b 115200 -U flash:w:"C:\Users\candela\Documents\Atmel Studio\7.0\stopwatch\stopwatch\Debug"
OS error: file Debug is not readable: (not a regular or character file?)
OS error: unable to open C:\Users\candela\Documents\Atmel Studio\7.0\stopwatch\stopwatch\Debug: Permission denied
Error: cannot determine file format for C:\Users\candela\Documents\Atmel Studio\7.0\stopwatch\stopwatch\Debug, specify explicitly
Error: reading from file Debug failed

Avrdude done.  Thank you.

C:\Users\candela\AppData\Local\Arduino15\packages\arduino\tools\avrdude\8.0.0-arduino1\bin>


