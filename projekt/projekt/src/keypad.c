/* =============================================================
 * Project:   MPC-POR cviceni 6
 * File:      keypad.c
 * Author:    Martin Kriz
 * Created:   2026-03-15
 * ============================================================= */

#include "../include/keypad.h"
#include "../include/timer.h"

#include <avr/io.h>

/* Radky: vystupy */
#define ROW0_PORT  PORTD
#define ROW0_DDR   DDRD
#define ROW0_PIN   PD2

#define ROW1_PORT  PORTD
#define ROW1_DDR   DDRD
#define ROW1_PIN   PD3

#define ROW2_PORT  PORTB
#define ROW2_DDR   DDRB
#define ROW2_PIN   PB3

#define ROW3_PORT  PORTB
#define ROW3_DDR   DDRB
#define ROW3_PIN   PB4

/* Sloupce: vstupy s pull-up */
#define COL_DDR    DDRC
#define COL_PORT   PORTC
#define COL_PIN    PINC
#define COL0_PIN   PC1
#define COL1_PIN   PC2
#define COL2_PIN   PC3

/* Debounce: 20 ms * 2 tiky/us = 40000 tiku */
#define KEY_DEBOUNCE_TICKS  40000UL

static const char key_map[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

/* Vsechny radky jako vstupy floating (DDR=0, PORT=0 - zadny pull-up) */
static void rows_idle(void)
{
    ROW0_PORT &= ~(1 << ROW0_PIN); ROW0_DDR &= ~(1 << ROW0_PIN);
    ROW1_PORT &= ~(1 << ROW1_PIN); ROW1_DDR &= ~(1 << ROW1_PIN);
    ROW2_PORT &= ~(1 << ROW2_PIN); ROW2_DDR &= ~(1 << ROW2_PIN);
    ROW3_PORT &= ~(1 << ROW3_PIN); ROW3_DDR &= ~(1 << ROW3_PIN);
}

/* Vybij sloupce na GND (odstrani naboj z predchoziho scanu) */
static void cols_discharge(void)
{
    COL_PORT &= ~((1 << COL0_PIN) | (1 << COL1_PIN) | (1 << COL2_PIN));
    COL_DDR  |=   (1 << COL0_PIN) | (1 << COL1_PIN) | (1 << COL2_PIN);
    busyDelay(4);
    COL_DDR  &= ~((1 << COL0_PIN) | (1 << COL1_PIN) | (1 << COL2_PIN));
}

/* Jeden scan – vraci stisknuty znak nebo 0 */
static char scan_once(void)
{
    for (uint8_t row = 0; row < 4; row++)
    {
        rows_idle();
        cols_discharge();

        /* Nastav jeden radek na HIGH: nejdriv PORT=1, pak DDR=1 (vystup) */
        switch (row)
        {
            case 0: ROW0_PORT |= (1 << ROW0_PIN); ROW0_DDR |= (1 << ROW0_PIN); break;
            case 1: ROW1_PORT |= (1 << ROW1_PIN); ROW1_DDR |= (1 << ROW1_PIN); break;
            case 2: ROW2_PORT |= (1 << ROW2_PIN); ROW2_DDR |= (1 << ROW2_PIN); break;
            case 3: ROW3_PORT |= (1 << ROW3_PIN); ROW3_DDR |= (1 << ROW3_PIN); break;
        }

        /* Kratka prodleva pro ustleni signalu */
        busyDelay(4);

        /* Zkontroluj sloupce (HIGH = stisknuto) */
        uint8_t pins = COL_PIN;
        for (uint8_t col = 0; col < 3; col++)
        {
            uint8_t col_pin = (col == 0) ? COL0_PIN :
                              (col == 1) ? COL1_PIN : COL2_PIN;

            if (pins & (1 << col_pin))
            {
                rows_idle();
                return key_map[row][col];
            }
        }
    }

    rows_idle();
    return 0;
}

void keypad_init(void)
{
    /* Radky jako vstupy (idle stav) */
    rows_idle();

    /* Sloupce jako vstupy floating */
    COL_DDR  &= ~((1 << COL0_PIN) | (1 << COL1_PIN) | (1 << COL2_PIN));
    COL_PORT &= ~((1 << COL0_PIN) | (1 << COL1_PIN) | (1 << COL2_PIN));
}

char keypad_get_key(void)
{
    static char     current_key  = 0;
    static uint8_t  reported     = 0;
    static uint32_t stable_since = 0;

    char     key = scan_once();
    uint32_t now = getTime();

    /* Zmena stavu – restart debounce */
    if (key != current_key)
    {
        current_key  = key;
        stable_since = now;
        reported     = 0;
    }

    /* Klávesa stabilní déle než debounce, ještě nenahlášena */
    if (current_key && !reported && (now - stable_since) >= KEY_DEBOUNCE_TICKS)
    {
        reported = 1;
        return current_key;
    }

    return 0;
}
