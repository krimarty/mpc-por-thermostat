/* =============================================================
 * Project:   MPC-POR cviceni 7
 * File:      at24c32.h
 * Author:    Martin Kriz
 * Created:   2026-03-25
 * -------------------------------------------------------------
 * Description:
 *   Knihovna pro obsluhu EEPROM AT24C32N pres I2C.
 *   Funkce:
 *     - Zapis 1 az 32 bajtu
 *     - Cteni 1 az 32 bajtu
 * Notes:
 *   AT24C32N I2C adresa: 0x57 (A2=A1=A0=1, typicke zapojeni
 *   na DS1307 RTC modulu)
 *   Po zapisu je nutno pockat 5 ms (write cycle time).
 *   Datasheet AT24C32N: sekce "Write Operations", "Read Operations"
 * ============================================================= */

#ifndef AT24C32_H_
#define AT24C32_H_

#include <stdint.h>

/* I2C adresa AT24C32N (7-bit): 1010 + A2 + A1 + A0
 * A2=A1=A0=0 (vychozi, propojky neosazene) -> 0b1010000 = 0x50
 * A2=A1=A0=1 (vsechny propojky spojene na VCC) -> 0b1010111 = 0x57 */
#define AT24C32_ADDR       0x50

/* Maximalni pocet bajtu pro jeden page write (dle datasheetu) */
#define AT24C32_PAGE_SIZE  32

/* ---------------------------------------------------------------
 * Zapis len bajtu (1-32) na adresu addr.
 * Vsechny bajty museji lezet na stejne strance (32B).
 * Po zapisu ceka 5 ms (write cycle time).
 * --------------------------------------------------------------- */
void at24c32_write(uint16_t addr, const uint8_t *data, uint8_t len);

/* ---------------------------------------------------------------
 * Precti len bajtu (1-32) z adresy addr.
 * Pouziva Random Read (dummy write pro nastaveni adresy,
 * pak Repeated START + cteni).
 * --------------------------------------------------------------- */
void at24c32_read(uint16_t addr, uint8_t *data, uint8_t len);

#endif /* AT24C32_H_ */
