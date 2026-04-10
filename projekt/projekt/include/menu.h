/* =============================================================
 * Project:   MPC-POR projekt
 * File:      menu.h
 * Author:    Martin Kriz
 * -------------------------------------------------------------
 * Description:
 *   Stavovy automat menu termostatu.
 *   Vstup: rotacni enkoder (get_count / get_btn_event).
 *   Vystup: LCD 16x2.
 * ============================================================= */

#ifndef MENU_H_
#define MENU_H_

void menu_init(void);
void menu_tick(void);
void menu_request_redraw(void);  /* vyzada prekreslen hlavni obrazovky */

#endif /* MENU_H_ */
