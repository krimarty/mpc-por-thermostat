/* =============================================================
 * Project:   MPC-POR projekt
 * File:      relay.c
 * Author:    Martin Kriz
 * ============================================================= */

#include "../include/relay.h"
#include "../include/timer.h"

#include <avr/io.h>

/* -----------------------------------------------------------------
 * Piny (aktivni LOW — vystup 0 = rele sepnuto)
 * ----------------------------------------------------------------- */
#define RELAY_HEAT_PIN   PB3
#define RELAY_COOL_PIN   PB4

/* -----------------------------------------------------------------
 * Minimalni interval mezi zmenami stavu jednoho rele.
 * getTime() vraci tiky po 0.5 us → 1 s = 2 000 000 tiku.
 * ----------------------------------------------------------------- */
#define RELAY_MIN_TICKS  2000000UL

/* -----------------------------------------------------------------
 * Stavove promenne
 * ----------------------------------------------------------------- */
static uint8_t  heat_state    = 0;
static uint8_t  cool_state    = 0;
static uint32_t heat_last_chg = 0;
static uint32_t cool_last_chg = 0;

/* -----------------------------------------------------------------
 * Pomocna funkce: fyzicky nastav pin rele
 * on=1 → vystup LOW (sepnuto), on=0 → vystup HIGH (rozepnuto)
 * ----------------------------------------------------------------- */
static void set_pin(uint8_t pin, uint8_t on)
{
    if (on)
        PORTB &= ~(1 << pin);   /* LOW = sepnuto */
    else
        PORTB |=  (1 << pin);   /* HIGH = rozepnuto */
}

/* -----------------------------------------------------------------
 * relay_init
 * ----------------------------------------------------------------- */
void relay_init(void)
{
    /* Nejdriv nastav HIGH (vypnuto), pak nastav jako vystup
     * aby behem prepnuti DDR nedoslo ke kratke pulzu na LOW */
    PORTB |= (1 << RELAY_HEAT_PIN) | (1 << RELAY_COOL_PIN);
    DDRB  |= (1 << RELAY_HEAT_PIN) | (1 << RELAY_COOL_PIN);

    heat_state    = 0;
    cool_state    = 0;
    heat_last_chg = 0;
    cool_last_chg = 0;
}

/* -----------------------------------------------------------------
 * relay_set_heating
 * ----------------------------------------------------------------- */
void relay_set_heating(uint8_t on)
{
    /* Zadna zmena — nic nedelat */
    if (on == heat_state)
        return;

    /* Nelze zapnout topeni pokud je aktivni chlazeni */
    if (on && cool_state)
        return;

    /* Dodrzet minimalni interval spinani 1 Hz */
    if ((getTime() - heat_last_chg) < RELAY_MIN_TICKS)
        return;

    heat_state    = on;
    heat_last_chg = getTime();
    set_pin(RELAY_HEAT_PIN, on);
}

/* -----------------------------------------------------------------
 * relay_set_cooling
 * ----------------------------------------------------------------- */
void relay_set_cooling(uint8_t on)
{
    if (on == cool_state)
        return;

    /* Nelze zapnout chlazeni pokud je aktivni topeni */
    if (on && heat_state)
        return;

    if ((getTime() - cool_last_chg) < RELAY_MIN_TICKS)
        return;

    cool_state    = on;
    cool_last_chg = getTime();
    set_pin(RELAY_COOL_PIN, on);
}

/* -----------------------------------------------------------------
 * Gettery
 * ----------------------------------------------------------------- */
uint8_t relay_get_heating(void) { return heat_state; }
uint8_t relay_get_cooling(void) { return cool_state; }
