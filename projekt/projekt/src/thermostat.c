/* =============================================================
 * Project:   MPC-POR projekt
 * File:      thermostat.c
 * Author:    Martin Kriz
 * ============================================================= */

#include "../include/thermostat.h"
#include "../include/settings.h"
#include "../include/relay.h"
#include "../include/adc.h"
#include "../include/ds1307.h"
#include "../include/timer.h"

#include <math.h>

/* -----------------------------------------------------------------
 * Konfigurace teplomeru (NTC termistor)
 * ----------------------------------------------------------------- */
#define TEMP_ADC_CH   1
#define TEMP_R1       20000.0f
#define TEMP_R2       1500.0f
#define TEMP_BETA     3895.0f
#define TEMP_R0       10000.0f
#define TEMP_T0       298.15f

/* -----------------------------------------------------------------
 * Globalni promenne
 * ----------------------------------------------------------------- */
volatile int16_t g_temp           = 0;
volatile uint8_t g_manual_override = 0;

static uint8_t last_min = 0xFF;   /* 0xFF = neplatna hodnota, vynutí prvni kontrolu */

/* -----------------------------------------------------------------
 * Pomocna: zmer teplotu, vrat x10 (po kalibraci)
 * Klouzavy prumer z posledních ADC_FILTER_SIZE vzorku ADC.
 * ----------------------------------------------------------------- */
#define ADC_FILTER_SIZE  20U

static int16_t read_temp(void)
{
    static uint16_t adc_buf[ADC_FILTER_SIZE];
    static uint8_t  adc_idx   = 0;
    static uint8_t  adc_count = 0;

    adc_buf[adc_idx] = adc_read(TEMP_ADC_CH);
    adc_idx = (uint8_t)((adc_idx + 1u) % ADC_FILTER_SIZE);
    if (adc_count < ADC_FILTER_SIZE) adc_count++;

    uint32_t sum = 0;
    for (uint8_t i = 0; i < adc_count; i++) sum += adc_buf[i];
    uint16_t adc = (uint16_t)(sum / adc_count);

    float    vadc = adc * 5.0f / 1024.0f;
    float    rth  = TEMP_R1 * vadc / (5.0f - vadc) - TEMP_R2;
    float    tk   = 1.0f / (logf(rth / TEMP_R0) / TEMP_BETA + 1.0f / TEMP_T0);
    float    tc   = tk - 273.15f;
    return (int16_t)(tc * 10.0f) + (int16_t)g_settings.cal;
}

/* -----------------------------------------------------------------
 * Pomocna: hysterezni regulator
 *
 * Topeni zapne kdyz: temp < sp - hyst/2
 * Topeni vypne kdyz: temp > sp + hyst/2
 * Chlazeni opacne.
 * Jednotky: vse x10 (0.1 C)
 * ----------------------------------------------------------------- */
static void regulate(void)
{
    int16_t sp   = g_settings.sp;
    int16_t hyst = (int16_t)g_settings.hyst;
    int16_t temp = g_temp;

    int16_t lo = sp - hyst / 2;
    int16_t hi = sp + hyst / 2;

    if (temp < lo)
    {
        relay_set_cooling(0);
        relay_set_heating(1);
    }
    else if (temp > hi)
    {
        relay_set_heating(0);
        relay_set_cooling(1);
    }
    else
    {
        /* V histereznim pasmu — nemen stav rele */
    }
}

/* -----------------------------------------------------------------
 * Pomocna: zkontroluj scheduler, pripadne prepni SP
 *
 * Logika:
 *   1. Zjisti aktualni cas a den tydne z RTC.
 *   2. Vyber spravnou sadu slotu (wd nebo we).
 *   3. Najdi posledni aktivni slot ktery uz probehl
 *      (slot.hour:min <= now.hour:min).
 *   4. Pokud se tento slot lisi od posledniho aplikovaneho,
 *      nastav SP a vynuluj manual_override.
 * ----------------------------------------------------------------- */
static void scheduler_check(void)
{
    if (!g_settings.sched_en)
        return;

    uint8_t hour, min, sec, date, month, year, day;
    ds1307_get_time(&hour, &min, &sec);
    ds1307_get_date(&date, &month, &year, &day);

    /* Vyber sadu slotu: Po-Pa (day 2-6) = wd, So-Ne (day 7,1) = we */
    sched_slot_t *slots = (day >= 2u && day <= 6u)
                          ? g_settings.wd
                          : g_settings.we;

    /* Aktualni cas v minutach od pulnoci */
    uint16_t now_min = (uint16_t)hour * 60u + min;

    /* Najdi posledni slot ktery uz probehl */
    int8_t   best_idx      = -1;
    uint16_t best_slot_min = 0;

    for (uint8_t i = 0; i < 4; i++)
    {
        if (!slots[i].enabled)
            continue;

        uint16_t slot_min = (uint16_t)slots[i].hour * 60u + slots[i].min;

        if (slot_min <= now_min && slot_min >= best_slot_min)
        {
            best_slot_min = slot_min;
            best_idx      = (int8_t)i;
        }
    }

    if (best_idx < 0)
        return;    /* zadny slot jeste dnes neprobehl */

    int16_t slot_sp = slots[best_idx].sp;

    /* Pokud SP scheduleru nesedi s aktualnim g_settings.sp
     * a uzivatel ho rucne nezmenil, aplikuj slot */
    if (!g_manual_override && g_settings.sp != slot_sp)
    {
        g_settings.sp = slot_sp;
    }

    /* Pokud SP odpovidalo tomu co chtel scheduler,
     * vynuluj override — uzivatel uz "dohral" */
    if (g_settings.sp == slot_sp)
    {
        g_manual_override = 0;
    }
}

/* -----------------------------------------------------------------
 * thermostat_init
 * ----------------------------------------------------------------- */
void thermostat_init(void)
{
    g_temp            = read_temp();
    g_manual_override = 0;
    last_min          = 0xFF;
}

/* -----------------------------------------------------------------
 * thermostat_tick — volat kazdych 5 s
 * ----------------------------------------------------------------- */
void thermostat_tick(void)
{
    g_temp = read_temp();
    regulate();

    uint8_t h, m, s;
    ds1307_get_time(&h, &m, &s);
    if (m != last_min)
    {
        last_min = m;
        scheduler_check();
    }
}

/* -----------------------------------------------------------------
 * thermostat_sample_temp — jen ADC, bez regulace
 * ----------------------------------------------------------------- */
void thermostat_sample_temp(void)
{
    g_temp = read_temp();
}

/* -----------------------------------------------------------------
 * thermostat_set_manual_sp
 * ----------------------------------------------------------------- */
void thermostat_set_manual_sp(int16_t sp_tenths)
{
    if (sp_tenths < THERMOSTAT_SP_MIN) sp_tenths = THERMOSTAT_SP_MIN;
    if (sp_tenths > THERMOSTAT_SP_MAX) sp_tenths = THERMOSTAT_SP_MAX;

    g_settings.sp     = sp_tenths;
    g_manual_override = 1;
}
