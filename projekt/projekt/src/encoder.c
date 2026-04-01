#include "../include/encoder.h"

#include <avr/io.h>
#include <avr/interrupt.h>

/* -----------------------------------------------------------------
 * Piny
 * ----------------------------------------------------------------- */
#define ENC_A_PIN   PD2     /* INT0 - CLK kanal A */
#define ENC_B_PIN   PD3     /* INT1 - DT  kanal B */
#define ENC_SW_PIN  PC2     /* SW tlacitko (aktivni LOW) */

/* -----------------------------------------------------------------
 * Timer2 CTC — generuje tick kazdych 10 ms
 *
 * F_CPU = 16 MHz, prescaler = 1024 -> f_tick = 15625 Hz -> T = 64 us
 * OCR2A = 155: compare po 156 taktech -> 156 * 64 us = 9.984 ms ~ 10 ms
 * ----------------------------------------------------------------- */
#define TIMER2_OCR  155U

/* -----------------------------------------------------------------
 * Prahy tlacitka (v poctu ticku Timer2, 1 tick = ~10 ms)
 *
 * BTN_DEBOUNCE_TICKS:
 *   Minimalni pocet po-sobe-jdoucich ticku kdy je tlacitko drzeno,
 *   aby byl stisk povazovan za platny. Mechanicke zakmity trvaji
 *   typicky 1-5 ms, tedy < 1 tick — nikdy prahem neprojdou.
 *   5 ticku * 10 ms = 50 ms
 *
 * BTN_LONG_TICKS:
 *   Pocet ticku drzeni po ktery se stisk stane dlouhym.
 *   100 ticku * 10 ms = 1000 ms = 1 s
 * ----------------------------------------------------------------- */
#define BTN_DEBOUNCE_TICKS  5U
#define BTN_LONG_TICKS      100U

/* Forward declaration — definice je az za ISR */
static void encoder_tick(void);

/* -----------------------------------------------------------------
 * Prechodova tabulka kvadraturniho enkoderu
 * Index = (predchozi_stav << 2) | aktualni_stav
 * ----------------------------------------------------------------- */
static const int8_t enc_table[16] = {
/*       00   01   10   11  */
/*00*/    0,  +1,  -1,   0,
/*01*/   -1,   0,   0,  +1,
/*10*/   +1,   0,   0,  -1,
/*11*/    0,  -1,  +1,   0
};

/* -----------------------------------------------------------------
 * Stavove promenne
 * ----------------------------------------------------------------- */
static volatile int16_t enc_count = 0;   /* syrovy pocet pulzu */
static          uint8_t enc_state = 0;   /* posledni stav A/B */

static          uint8_t         btn_hold  = 0;              /* pocitadlo ticku drzeni (Timer2) */
static          uint8_t         btn_fired = 0;              /* 1 = long uz byl vydan */
static volatile enc_btn_event_t btn_event = ENC_BTN_NONE;   /* cekajici udalost */

/* -----------------------------------------------------------------
 * ISR pro enkoder — INT0 (kanal A) a INT1 (kanal B), jakakoli hrana
 * Quadraturni tabulka filtruje zakmity sama — zadny casovy debounce.
 * ----------------------------------------------------------------- */
static void enc_process(void)
{
    uint8_t a   = (PIND >> ENC_A_PIN) & 1;
    uint8_t b   = (PIND >> ENC_B_PIN) & 1;
    uint8_t cur = (a << 1) | b;
    uint8_t idx = (enc_state << 2) | cur;

    enc_count += enc_table[idx];
    enc_state  = cur;
}

ISR(INT0_vect) { enc_process(); }
ISR(INT1_vect) { enc_process(); }

/* -----------------------------------------------------------------
 * encoder_init
 * ----------------------------------------------------------------- */
void encoder_init(void)
{
    /* Vstupy s pull-up pro A, B a SW */
    DDRD  &= ~((1 << ENC_A_PIN) | (1 << ENC_B_PIN));
    PORTD |=   (1 << ENC_A_PIN) | (1 << ENC_B_PIN);

    DDRC  &= ~(1 << ENC_SW_PIN);
    PORTC |=  (1 << ENC_SW_PIN);

    /* Zapamatuj pocatecni polohu enkoderu */
    enc_state = (((PIND >> ENC_A_PIN) & 1) << 1)
              |  ((PIND >> ENC_B_PIN) & 1);

    /* Preruseni na jakoukoliv hranu pro INT0 a INT1 */
    EICRA = (0 << ISC11) | (1 << ISC10) |
            (0 << ISC01) | (1 << ISC00);
    EIMSK = (1 << INT1) | (1 << INT0);

    /* Tlacitko SW: bez PCINT — obsluha je v encoder_tick() z Timer2 ISR */

    /* Timer2 CTC: tick kazdych ~10 ms
     * WGM21 = CTC mod, CS22|CS21|CS20 = prescaler 1024 */
    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
    OCR2A  = TIMER2_OCR;
    TIMSK2 = (1 << OCIE2A);
}

ISR(TIMER2_COMPA_vect)
{
    encoder_tick();
}

/* -----------------------------------------------------------------
 * encoder_tick — interni funkce, volana z ISR(TIMER2_COMPA_vect)
 *
 * Logika:
 *   - Pokud je SW stisknuto (aktivni LOW), inkrementuj btn_hold.
 *   - Pri uvolneni: pokud btn_hold >= DEBOUNCE ale long jeste
 *     neodeslal, vydej SHORT event.
 *   - Pokud btn_hold dosahne LONG_TICKS behem drzeni, vydej LONG
 *     event okamzite (necekat na uvolneni).
 *   - Po vydani eventu nastav btn_fired = 1, aby se event nevydal
 *     dvakrat pri jednom stisku.
 * ----------------------------------------------------------------- */
static void encoder_tick(void)
{
    uint8_t pressed = !((PINC >> ENC_SW_PIN) & 1);  /* aktivni LOW */

    if (pressed)
    {
        if (btn_hold < 255)
            btn_hold++;

        /* Dlouhy stisk — vydej ihned (jen jednou za stisk) */
        if (btn_hold >= BTN_LONG_TICKS && !btn_fired)
        {
            btn_event = ENC_BTN_LONG;
            btn_fired = 1;
        }
    }
    else
    {
        /* Tlacitko uvolneno */
        if (btn_hold >= BTN_DEBOUNCE_TICKS && !btn_fired)
            btn_event = ENC_BTN_SHORT;

        btn_hold  = 0;
        btn_fired = 0;
    }
}

/* -----------------------------------------------------------------
 * encoder_get_count
 * Vraci pocet detentu (1 detent = 1 nastupna hrana INT0).
 * ----------------------------------------------------------------- */
int16_t encoder_get_count(void)
{
    cli();
    int16_t val = enc_count;
    sei();
    return val / 4;
}

/* -----------------------------------------------------------------
 * encoder_clear_count — vynuluj pocitadlo po zpracovani pohybu
 * ----------------------------------------------------------------- */
void encoder_clear_count(void)
{
    cli();
    enc_count = 0;
    sei();
}

/* -----------------------------------------------------------------
 * encoder_get_btn_event — vrat a vynuluj cekajici udalost
 * ----------------------------------------------------------------- */
enc_btn_event_t encoder_get_btn_event(void)
{
    enc_btn_event_t e = btn_event;
    btn_event = ENC_BTN_NONE;
    return e;
}
