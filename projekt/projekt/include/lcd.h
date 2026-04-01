/* =============================================================
 * Project:   MPC-POR cviceni 5
 * File:      lcd.h
 * Author:    Martin Kriz
 * Created:   2026-03-10
 * -------------------------------------------------------------
 * Description:
 *   Knihovna pro obsluhu LCD displeje HD44780 v rezimu 4-bit.
 *   16x2 znaku, bez cteni Busy Flagu (RW = GND).
 * Notes:
 *   Vyzaduje inicializovany timer - zavolej timerInit() pred lcd_init()
 *   Datasheet HD44780U: Figure 24 (4-bit init), Table 6 (instrukce)
 * ============================================================= */

#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>
#include <stdio.h>

/* Stream pro fprintf/printf - pouziti: fprintf(&lcd_stream, "text %d", val)
 * nebo: stdout = &lcd_stream; printf("text");
 * '\n' presune kurzor na zacatek dalsiho radku (0->1->0) */
extern FILE lcd_stream;

void lcd_init(void);
void lcd_clear(void);
void lcd_home(void);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_write_char(char c);
void lcd_write_string(const char *str);
void lcd_shift_display(uint8_t right);

/* Nahraje vlastni znak do CGRAM (HD44780 ma 8 slotu, index 0-7).
 * bitmap: pole 8 bajtu, kazdy bajt = 1 radek (pouzivaji se bity 4:0).
 * Po zapisu automaticky vrati kurzor na DDRAM pozici 0,0. */
void lcd_create_char(uint8_t index, const uint8_t *bitmap);

/* display: 1=zapnut, 0=vypnut
 * cursor:  1=kurzor viditelny, 0=skryty
 * blink:   1=blikajici kurzor, 0=neblika */
void lcd_config(uint8_t display, uint8_t cursor, uint8_t blink);

#endif /* LCD_H_ */
