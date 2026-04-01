/* =============================================================
 * Project:   MPC-POR projekt
 * File:      encoder.h
 * Author:    Martin Kriz
 * Created:   2026-03-14
 * -------------------------------------------------------------
 * Description:
 *   Knihovna pro rotacni enkoder s tlacitkem.
 *   Otaceni: INT0/INT1 + prechodova tabulka (4 pulzy na detent).
 *   Tlacitko: polling v encoder_tick() - counter-increment debounce.
 *
 * Pouziti:
 *   1. Zavolej encoder_init().
 *   2. Volej encoder_tick() pravidelne kazdych 10 ms (napr. z hlavni smycky).
 *   3. Cti encoder_get_count() pro pohyb, encoder_get_btn_event() pro tlacitko.
 *
 * Debounce tlacitka:
 *   Mechanicke zakmity trvaji typicky < 10 ms. Prah pro platny stisk
 *   je BTN_DEBOUNCE_TICKS (5 ticku = 50 ms), takze zakmity prahem
 *   nikdy neprojdou - neni potreba casovac ani getTime().
 *   Dlouhy stisk (BTN_LONG_TICKS = 100 ticku = 1 s) se detekuje
 *   prekonanim vyssibo prahu behem drzeni tlacitka.
 * ============================================================= */

#ifndef ENCODER_H_
#define ENCODER_H_

#include <stdint.h>

/* Typy udalosti tlacitka */
typedef enum {
    ENC_BTN_NONE  = 0,  /* zadna udalost */
    ENC_BTN_SHORT = 1,  /* kratky stisk (uvolneni po debounce, pred long) */
    ENC_BTN_LONG  = 2   /* dlouhy stisk (1 s drzeni, fire ihned) */
} enc_btn_event_t;

/* Inicializace pinu, preruseni INT0/INT1 a Timer2 (tick kazd 10 ms) */
void encoder_init(void);

/* Pocitadlo detentu enkoderu (kladne = CW, zaporne = CCW).
 * Jeden detent = 4 kvadraturni pulzy -> vysledek je vydejen 4. */
int16_t encoder_get_count(void);

/* Vynuluje pocitadlo enkoderu (zavolej po zpracovani pohybu) */
void encoder_clear_count(void);

/* Vrati udalost tlacitka a okamzite ji vynuluje (volej v hlavni smycce) */
enc_btn_event_t encoder_get_btn_event(void);

#endif /* ENCODER_H_ */
