/* =============================================================
 * Project:   MPC-POR projekt
 * File:      thermostat.h
 * Author:    Martin Kriz
 * -------------------------------------------------------------
 * Description:
 *   Regulacni logika termostatu.
 *   - Hysterezni on/off regulator (topeni / chlazeni)
 *   - Scheduler: prepina SP dle casovych slotu (60s kontrola)
 *   - Manual override: rucni zmena SP potlaci scheduler
 *     dokud neprijde dalsi aktivni slot
 *
 * Pouziti:
 *   thermostat_init() pri startu (po settings_load, relay_init).
 *   thermostat_tick() volat kazdych 5 s z hlavni smycky.
 *   Pri rucni zmene SP zavolat thermostat_set_manual_sp().
 *
 * Teplotni rozsah SP:
 *   THERMOSTAT_SP_MIN / THERMOSTAT_SP_MAX (jednotky 0.1 C)
 *   Vychozi 50 (5.0 C) az 350 (35.0 C), prepisatelne.
 * ============================================================= */

#ifndef THERMOSTAT_H_
#define THERMOSTAT_H_

#include <stdint.h>

/* Rozsah zadane teploty v jednotkach 0.1 C */
#ifndef THERMOSTAT_SP_MIN
#define THERMOSTAT_SP_MIN   50    /*  5.0 C */
#endif
#ifndef THERMOSTAT_SP_MAX
#define THERMOSTAT_SP_MAX  350    /* 35.0 C */
#endif

/* Inicializace — nastavi vnitrni stav, spocita prvni den tydne */
void thermostat_init(void);

/* Volat kazdych 5 s:
 *  - precte teplotu (ADC + kalibrace)
 *  - spusti / zastavi rele dle hystereze
 *  - kazdych 60 s zkontroluje scheduler */
void thermostat_tick(void);

/* Rucni zmena SP z menu nebo UART.
 * Nastavi g_settings.sp, zapne manual_override. */
void thermostat_set_manual_sp(int16_t sp_tenths);

/* Aktualni namerena teplota x10 (po kalibraci) */
extern volatile int16_t g_temp;

/* 1 = uzivatel zmenil SP rucne, scheduler ho neprepsal */
extern volatile uint8_t g_manual_override;

#endif /* THERMOSTAT_H_ */
