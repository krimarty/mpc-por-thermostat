/* =============================================================
 * Project:   MPC-POR projekt
 * File:      relay.h
 * Author:    Martin Kriz
 * -------------------------------------------------------------
 * Description:
 *   Knihovna pro ovladani 2 rele (topeni / chlazeni).
 *   Modul je aktivni LOW: vystup 0 = rele sepnuto.
 *   Topeni:  PB3 (In1)
 *   Chlazeni: PB4 (In2)
 *
 * Omezeni:
 *   - Max. frekvence spinani: 1 Hz (hardwarove vynuceno)
 *   - Topeni a chlazeni nemohou byt aktivni zaroven
 * ============================================================= */

#ifndef RELAY_H_
#define RELAY_H_

#include <stdint.h>

/* Inicializace — PB3, PB4 jako vystupy, oboje vypnuto */
void relay_init(void);

/* Zapni / vypni topeni (1 = zapnout, 0 = vypnout).
 * Pokud by doslo ke spinani drive nez 1 s od posledni zmeny,
 * nebo je aktivni chlazeni, prikaz se ignoruje. */
void relay_set_heating(uint8_t on);

/* Zapni / vypni chlazeni (1 = zapnout, 0 = vypnout).
 * Stejna omezeni jako relay_set_heating(). */
void relay_set_cooling(uint8_t on);

/* Vrati aktualni stav (1 = zapnuto, 0 = vypnuto) */
uint8_t relay_get_heating(void);
uint8_t relay_get_cooling(void);

#endif /* RELAY_H_ */
