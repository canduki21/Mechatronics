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
  uint8_t output = 0;
  for (int i = 0; i < 7; i++)
  {
    if (value & (1 << i))
      output &= ~(1 << i);  // Clear bit (LOW = ON)
    else
      output |= (1 << i);   // Set bit (HIGH = OFF)
  }
  
  // Decimal point (bit 7)
  if (show_decimal)
    output &= ~(1 << 7);  // LOW = ON
  else
    output |= (1 << 7);   // HIGH = OFF
  
  PORTC = output;
}

void disable_digits(void)
{
  // For COMMON CATHODE: LOW to disable
  PORTD &= ~((1 << PD0) | (1 << PD1) | (1 << PD2) | (1 << PD3));
}

void enable_digit(uint8_t d)
{
  disable_digits();
  // For COMMON CATHODE: HIGH to enable
  switch(d)
  {
    case 0: PORTD |= (1 << PD0); break;  // D1
    case 1: PORTD |= (1 << PD1); break;  // D2
    case 2: PORTD |= (1 << PD2); break;  // D3
    case 3: PORTD |= (1 << PD3); break;  // D4
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

// External interrupt for button (INT0 on PD4)
ISR(INT0_vect)
{
  _delay_ms(50);  // Simple debounce
  if (PIND & (1 << PD4))  // Check if still pressed
  {
    running = !running;
  }
}

void init_timer(void)
{
  // Timer1 in CTC mode for 10ms interrupt
  // Prescaler = 64, OCR1A = 2499
  // (16MHz / 64) / 2500 = 100 Hz (10ms)
  
  TCCR1B |= (1 << WGM12);           // CTC mode
  TCCR1B |= (1 << CS11) | (1 << CS10);  // Prescaler = 64
  OCR1A = 2499;                     // Compare value
  TIMSK1 |= (1 << OCIE1A);          // Enable compare interrupt
}

void init_button(void)
{
  // PD4 as input with pull-up
  DDRD &= ~(1 << PD4);
  PORTD |= (1 << PD4);
  
  // Enable INT0 (PD2 on most Arduinos, but we'll use Pin Change Interrupt instead)
  // Actually, let's use polling in main loop for simplicity
}

void init_gpio(void)
{
  // PORTC (PC0-PC7) as outputs for segments
  DDRC = 0xFF;
  PORTC = 0xFF;  // All HIGH (segments OFF for common cathode inverted)
  
  // PORTD (PD0-PD3) as outputs for digits
  DDRD |= (1 << PD0) | (1 << PD1) | (1 << PD2) | (1 << PD3);
  PORTD &= ~((1 << PD0) | (1 << PD1) | (1 << PD2) | (1 << PD3));  // All LOW (digits OFF)
  
  // PD4 as input for button
  DDRD &= ~(1 << PD4);
  PORTD |= (1 << PD4);  // Enable pull-up
}

int main(void)
{
  init_gpio();
  init_timer();
  init_button();
  
  sei();  // Enable global interrupts
  
  // Button handling variables
  static bool last_button_state = true;  // Pull-up = HIGH when not pressed
  
  while (1)
  {
    display_time(stopwatch_ticks);
    
    // Simple button polling with debounce
    bool button_state = (PIND & (1 << PD4)) == 0;  // LOW when pressed
    if (button_state && !last_button_state)
    {
      _delay_ms(50);  // Debounce
      if ((PIND & (1 << PD4)) == 0)  // Still pressed
      {
        running = !running;
      }
    }
    last_button_state = button_state;
  }
}
