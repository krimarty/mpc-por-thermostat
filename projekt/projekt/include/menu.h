/* =============================================================
 * Project:   MPC-POR projekt
 * File:      menu.h
 * Author:    Martin Kriz
 * -------------------------------------------------------------
 * Description:
 *   Stavovy automat menu termostatu.
 *   Vstup: rotacni enkoder (get_count / get_btn_event).
 *   Vystup: LCD 16x2.
 *
 * Pouziti:
 *   menu_init() pri startu (po lcd_init, encoder_init, settings_load).
 *   menu_tick() volat v kazde iteraci hlavni smycky.
 *   menu_request_redraw() zavolat kdyz se zmeni teplota nebo cas
 *   (napr. po thermostat_tick) aby se hlavni obrazovka aktualizovala.
 * ============================================================= */

#ifndef MENU_H_
#define MENU_H_

void menu_init(void);
void menu_tick(void);
void menu_request_redraw(void);  /* vyzada prekreslen hlavni obrazovky */

#endif /* MENU_H_ */
