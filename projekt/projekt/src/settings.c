/* =============================================================
 * Project:   MPC-POR projekt
 * File:      settings.c
 * Author:    Martin Kriz
 * ============================================================= */

#include "../include/settings.h"
#include "../include/at24c32.h"

/* -----------------------------------------------------------------
 * EEPROM mapa
 *
 * 0x0000  2 B  magic number: 0xBE, 0xEF
 * 0x0002  2 B  SP x10 (int16, little-endian)
 * 0x0004  1 B  hystereze x10 (uint8)
 * 0x0005  1 B  kalibrace: ulozena jako (cal_tenths + 50), uint8, rozsah 0-100
 * 0x0006  1 B  scheduler enabled (0/1)
 * 0x0007  1 B  rezerva
 * 0x0008 20 B  pracovni dny: 4x slot (5 B kazdy)  [page 0, 0x0008-0x001B]
 * 0x0020 20 B  vikend:       4x slot (5 B kazdy)  [page 1, 0x0020-0x0033]
 *
 * Slot (5 B):
 *   [0] enabled (0/1)
 *   [1] hodina  (0-23)
 *   [2] minuta  (0-59)
 *   [3] SP cela cast (uint8, napr. 21 pro 21.x C)
 *   [4] SP desetinna cast x10 (0 nebo 5)
 * ----------------------------------------------------------------- */
#define EEPROM_ADDR_MAGIC    0x0000
#define EEPROM_ADDR_SP       0x0002
#define EEPROM_ADDR_HYST     0x0004
#define EEPROM_ADDR_CAL      0x0005
#define EEPROM_ADDR_SCHED_EN 0x0006
#define EEPROM_ADDR_WD       0x0008
#define EEPROM_ADDR_WE       0x0020

#define MAGIC_0  0xBE
#define MAGIC_1  0xEF

#define CAL_OFFSET  50   /* ulozena hodnota = cal_tenths + CAL_OFFSET */

/* -----------------------------------------------------------------
 * Vychozi hodnoty
 * ----------------------------------------------------------------- */
#define DEFAULT_SP    200   /* 20.0 C */
#define DEFAULT_HYST    5   /*  0.5 C */
#define DEFAULT_CAL     0   /*  0.0 C */

/* -----------------------------------------------------------------
 * Globalni instance
 * ----------------------------------------------------------------- */
settings_t g_settings;

/* -----------------------------------------------------------------
 * Pomocne funkce pro serializaci slotu
 * ----------------------------------------------------------------- */
static void slot_to_bytes(const sched_slot_t *s, uint8_t *buf)
{
    buf[0] = s->enabled;
    buf[1] = s->hour;
    buf[2] = s->min;
    /* SP x10 rozlozit na celou a desetinnou cast */
    int16_t sp = s->sp;
    if (sp < 0) sp = 0;            /* sloty nepodporuji zapornou teplotu */
    buf[3] = (uint8_t)(sp / 10);
    buf[4] = (uint8_t)(sp % 10);   /* 0 nebo 5 */
}

static void bytes_to_slot(const uint8_t *buf, sched_slot_t *s)
{
    s->enabled = buf[0] ? 1u : 0u;
    s->hour    = (buf[1] <= 23u) ? buf[1] : 0u;
    s->min     = (buf[2] <= 59u) ? buf[2] : 0u;
    s->sp      = (int16_t)buf[3] * 10 + (int16_t)(buf[4] <= 9u ? buf[4] : 0u);
}

/* -----------------------------------------------------------------
 * Zavede vychozi hodnoty do g_settings
 * ----------------------------------------------------------------- */
static void load_defaults(void)
{
    g_settings.sp       = DEFAULT_SP;
    g_settings.hyst     = DEFAULT_HYST;
    g_settings.cal      = DEFAULT_CAL;
    g_settings.sched_en = 0;

    for (uint8_t i = 0; i < 4; i++)
    {
        g_settings.wd[i].enabled = 0;
        g_settings.wd[i].hour    = 6;
        g_settings.wd[i].min     = 0;
        g_settings.wd[i].sp      = DEFAULT_SP;

        g_settings.we[i].enabled = 0;
        g_settings.we[i].hour    = 8;
        g_settings.we[i].min     = 0;
        g_settings.we[i].sp      = DEFAULT_SP;
    }
}

/* -----------------------------------------------------------------
 * settings_load
 * ----------------------------------------------------------------- */
void settings_load(void)
{
    uint8_t magic[2];
    at24c32_read(EEPROM_ADDR_MAGIC, magic, 2);

    if (magic[0] != MAGIC_0 || magic[1] != MAGIC_1)
    {
        /* EEPROM neobsahuje platna data — pouzij vychozi a ihned uloz */
        load_defaults();
        settings_save();
        return;
    }

    /* Nacteni skalarnich hodnot */
    uint8_t buf[2];

    at24c32_read(EEPROM_ADDR_SP, buf, 2);
    g_settings.sp = (int16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));

    at24c32_read(EEPROM_ADDR_HYST, buf, 1);
    g_settings.hyst = buf[0];

    at24c32_read(EEPROM_ADDR_CAL, buf, 1);
    g_settings.cal = (int8_t)((int16_t)buf[0] - CAL_OFFSET);

    at24c32_read(EEPROM_ADDR_SCHED_EN, buf, 1);
    g_settings.sched_en = buf[0] ? 1u : 0u;

    /* Nacteni slotu — pracovni dny */
    uint8_t slot_buf[5];
    for (uint8_t i = 0; i < 4; i++)
    {
        at24c32_read((uint16_t)(EEPROM_ADDR_WD + i * 5u), slot_buf, 5);
        bytes_to_slot(slot_buf, &g_settings.wd[i]);
    }

    /* Nacteni slotu — vikend */
    for (uint8_t i = 0; i < 4; i++)
    {
        at24c32_read((uint16_t)(EEPROM_ADDR_WE + i * 5u), slot_buf, 5);
        bytes_to_slot(slot_buf, &g_settings.we[i]);
    }
}

/* -----------------------------------------------------------------
 * settings_save
 * ----------------------------------------------------------------- */
void settings_save(void)
{
    uint8_t buf[2];

    /* Magic number */
    buf[0] = MAGIC_0; buf[1] = MAGIC_1;
    at24c32_write(EEPROM_ADDR_MAGIC, buf, 2);

    /* SP (little-endian) */
    buf[0] = (uint8_t)(g_settings.sp & 0xFF);
    buf[1] = (uint8_t)((g_settings.sp >> 8) & 0xFF);
    at24c32_write(EEPROM_ADDR_SP, buf, 2);

    /* Hystereze */
    buf[0] = g_settings.hyst;
    at24c32_write(EEPROM_ADDR_HYST, buf, 1);

    /* Kalibrace */
    buf[0] = (uint8_t)((int16_t)g_settings.cal + CAL_OFFSET);
    at24c32_write(EEPROM_ADDR_CAL, buf, 1);

    /* Scheduler enabled */
    buf[0] = g_settings.sched_en;
    at24c32_write(EEPROM_ADDR_SCHED_EN, buf, 1);

    /* Sloty — pracovni dny */
    uint8_t slot_buf[5];
    for (uint8_t i = 0; i < 4; i++)
    {
        slot_to_bytes(&g_settings.wd[i], slot_buf);
        at24c32_write((uint16_t)(EEPROM_ADDR_WD + i * 5u), slot_buf, 5);
    }

    /* Sloty — vikend */
    for (uint8_t i = 0; i < 4; i++)
    {
        slot_to_bytes(&g_settings.we[i], slot_buf);
        at24c32_write((uint16_t)(EEPROM_ADDR_WE + i * 5u), slot_buf, 5);
    }
}
