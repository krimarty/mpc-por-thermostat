/* =============================================================
 * Project:   MPC-POR projekt
 * File:      settings.h
 * Author:    Martin Kriz
 * -------------------------------------------------------------
 * Description:
 *   Sprava nastaveni termostatu — ulozeni / nacteni z EEPROM
 *   AT24C32N pres at24c32 knihovnu.
 *
 * Pouziti:
 *   settings_load() pri startu — nacte EEPROM nebo zavede vychozi
 *   hodnoty pokud EEPROM neobsahuje platna data (magic number).
 *   settings_save() ulozi aktualni stav do EEPROM.
 *   Zbytek kodu pristupuje primo ke globalni promenne `g_settings`.
 *
 * Interni jednotky (zamezuji float aritmetice):
 *   teplota    : int16_t,  jednotky 0.1 °C  (210 = 21.0 °C)
 *   hystereze  : uint8_t,  jednotky 0.1 °C  (5   =  0.5 °C)
 *   kalibrace  : int8_t,   jednotky 0.1 °C  (3   = +0.3 °C, rozsah -50..+50)
 * ============================================================= */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdint.h>

/* -----------------------------------------------------------------
 * Jeden casovy slot scheduleru
 * ----------------------------------------------------------------- */
typedef struct
{
    uint8_t  enabled;   /* 0 = slot ignorovan */
    uint8_t  hour;      /* 0-23 */
    uint8_t  min;       /* 0-59 */
    int16_t  sp;        /* zadana teplota x10 (210 = 21.0 C) */
} sched_slot_t;

/* -----------------------------------------------------------------
 * Hlavni struktura nastaveni
 * ----------------------------------------------------------------- */
typedef struct
{
    int16_t      sp;            /* aktualni zadana teplota x10 */
    uint8_t      hyst;          /* hystereze x10 */
    int8_t       cal;           /* kalibraci offset x10 */
    uint8_t      sched_en;      /* scheduler zapnut (0/1) */
    sched_slot_t wd[4];         /* pracovni dny (Po-Pa), sloty 0-3 */
    sched_slot_t we[4];         /* vikend (So-Ne), sloty 0-3 */
} settings_t;

/* Globalni instance — cti a pis primo */
extern settings_t g_settings;

/* -----------------------------------------------------------------
 * Nacti nastaveni z EEPROM.
 * Pokud magic number nesedi, zavede vychozi hodnoty a ulozi je.
 * ----------------------------------------------------------------- */
void settings_load(void);

/* Uloz aktualni g_settings do EEPROM */
void settings_save(void);

#endif /* SETTINGS_H_ */
