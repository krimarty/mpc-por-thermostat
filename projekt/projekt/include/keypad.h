/* =============================================================
 * Project:   MPC-POR cviceni 6
 * File:      keypad.h
 * Author:    Martin Kriz
 * Created:   2026-03-15
 * -------------------------------------------------------------
 * Description:
 *   Knihovna pro obsluhu 4x3 maticove klavesnice.
 *   Radky: PD2, PD3, PB3, PB4 (vystupy)
 *   Sloupce: PC1, PC2, PC3   (vstupy s pull-up)
 * ============================================================= */

#ifndef KEYPAD_H_
#define KEYPAD_H_

#include <stdint.h>

void keypad_init(void);

/* Vrati stisknuty znak (pouze jednou za stisk, s debounce), nebo 0 */
char keypad_get_key(void);

#endif /* KEYPAD_H_ */
