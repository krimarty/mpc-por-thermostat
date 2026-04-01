/* =============================================================
 * Project:   MPC-POR cviceni 7
 * File:      ds1307.h
 * Author:    Martin Kriz
 * Created:   2026-03-24
 * -------------------------------------------------------------
 * Description:
 *   Knihovna pro obsluhu RTC DS1307 pres I2C.
 *   Funkce:
 *     - Nastaveni data a casu
 *     - Cteni casu
 *     - Cteni data
 *     - Nastaveni SQW vystupniho signalu
 * Notes:
 *   DS1307 I2C adresa: 0x68 (fixni, nelze menit)
 *   Format dat: BCD (Binary Coded Decimal)
 *   24-hodinovy rezim hodin (bit 6 reg 2 = 0)
 * ============================================================= */

#ifndef DS1307_H_
#define DS1307_H_

#include <stdint.h>

/* I2C adresa DS1307 (7-bit) */
#define DS1307_ADDR  0x68

/* Struktura pro datum a cas */
typedef struct
{
    uint8_t sec;    /* sekundy     0-59 */
    uint8_t min;    /* minuty      0-59 */
    uint8_t hour;   /* hodiny      0-23 (24h rezim) */
    uint8_t day;    /* den tydne   1-7  (1=nedele) */
    uint8_t date;   /* den mesice  1-31 */
    uint8_t month;  /* mesic       1-12 */
    uint8_t year;   /* rok         0-99 (2000 + year) */
} ds1307_time_t;

/* Rezim SQW vystupu (registr 0x07)
 * Bit 4: SQWE - povoleni generatoru
 * Bit 7: OUT  - uroven vystupu pri SQWE=0
 * Bity 1:0: RS - vyber frekvence */
typedef enum
{
    DS1307_SQW_1HZ     = 0x10,  /* SQWE=1, RS=00 -> 1 Hz     */
    DS1307_SQW_4096HZ  = 0x11,  /* SQWE=1, RS=01 -> 4096 Hz  */
    DS1307_SQW_8192HZ  = 0x12,  /* SQWE=1, RS=10 -> 8192 Hz  */
    DS1307_SQW_32768HZ = 0x13,  /* SQWE=1, RS=11 -> 32768 Hz */
    DS1307_SQW_OFF     = 0x00,  /* SQWE=0, OUT=0              */
} ds1307_sqw_t;

/* ---------------------------------------------------------------
 * Inicializace: vola i2c_init(), zkontroluje bit CH (clock halt).
 * Pokud jsou hodiny zastavene, spusti je.
 * --------------------------------------------------------------- */
void ds1307_init(void);

/* ---------------------------------------------------------------
 * Nastav datum a cas najednou (vsechny registry 0x00-0x06).
 * --------------------------------------------------------------- */
void ds1307_set_datetime(const ds1307_time_t *t);

/* ---------------------------------------------------------------
 * Precti aktualni cas (hodiny, minuty, sekundy).
 * --------------------------------------------------------------- */
void ds1307_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec);

/* ---------------------------------------------------------------
 * Precti aktualni datum (den, mesic, rok, den tydne).
 * --------------------------------------------------------------- */
void ds1307_get_date(uint8_t *date, uint8_t *month, uint8_t *year, uint8_t *day);

/* ---------------------------------------------------------------
 * Nastav rezim SQW vystupu (pin SQ na modulu).
 * Pro 1Hz signal na INT0: ds1307_set_sqw(DS1307_SQW_1HZ)
 * --------------------------------------------------------------- */
void ds1307_set_sqw(ds1307_sqw_t mode);

#endif /* DS1307_H_ */
