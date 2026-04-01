/* =============================================================
 * Project:   MPC-POR cviceni 5
 * File:      lcd_pins.h
 * Author:    Martin Kriz
 * Created:   2026-03-10
 * -------------------------------------------------------------
 * Description:
 *   Definice pinu LCD Keypad Shieldu na ATmega328P.
 *   RS -> PB0, E -> PB1, D4-D7 -> PD4-PD7, RW -> GND
 * Notes:
 * ============================================================= */

#ifndef LCD_PINS_H_
#define LCD_PINS_H_

#include <avr/io.h>

/* Rizeni - PORTB */
#define LCD_RS_DDR      DDRB
#define LCD_RS_PORT     PORTB
#define LCD_RS_PIN      PORTB0

#define LCD_E_DDR       DDRB
#define LCD_E_PORT      PORTB
#define LCD_E_PIN       PORTB1

/* Data D4-D7 - horni nibble PORTD */
#define LCD_DATA_DDR    DDRD
#define LCD_DATA_PORT   PORTD
#define LCD_D4_PIN      PORTD4
#define LCD_D5_PIN      PORTD5
#define LCD_D6_PIN      PORTD6
#define LCD_D7_PIN      PORTD7

#endif /* LCD_PINS_H_ */
