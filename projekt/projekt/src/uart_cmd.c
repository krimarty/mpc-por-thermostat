/* =============================================================
 * Project:   MPC-POR projekt
 * File:      uart_cmd.c
 * Author:    Martin Kriz
 * ============================================================= */

#include "../include/uart_cmd.h"
#include "../include/uart.h"
#include "../include/settings.h"
#include "../include/thermostat.h"
#include "../include/relay.h"
#include "../include/ds1307.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define LINE_BUF_SIZE  48U

/* -----------------------------------------------------------------
 * Pozicni parsovani — cte mezeru oddelene hodnoty
 * ----------------------------------------------------------------- */

/* Preskoc mezery, vrat pointer za nimi */
static const char *skip_sp(const char *s)
{
    while (*s == ' ') s++;
    return s;
}

/* Precti uint8 ze stringu, posun pointer za prectene znaky */
static uint8_t read_u8(const char *s, const char **next)
{
    s = skip_sp(s);
    uint8_t v = 0;
    while (*s >= '0' && *s <= '9')
        v = (uint8_t)(v * 10u + (uint8_t)(*s++ - '0'));
    *next = s;
    return v;
}

/* Precti teplotu "XX.X" nebo "-X.X" jako int16 x10 */
static int16_t read_temp(const char *s, const char **next)
{
    s = skip_sp(s);
    int8_t sign = 1;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }

    int16_t v = 0;
    while (*s >= '0' && *s <= '9')
        v = (int16_t)(v * 10 + (*s++ - '0'));

    int16_t frac = 0;
    if (*s == '.' || *s == ',')
    {
        s++;
        if (*s >= '0' && *s <= '9') frac = *s++ - '0';
    }
    *next = s;
    return (int16_t)(sign * (v * 10 + frac));
}

/* Formátuj int16 x10 → "XX.X" nebo "-X.X" */
static void fmt_temp(int16_t val, char *buf)
{
    if (val < 0) { val = -val; snprintf(buf, 7, "-%d.%d", val/10, val%10); }
    else         {             snprintf(buf, 7,  "%d.%d", val/10, val%10); }
}

/* -----------------------------------------------------------------
 * Obsluha prikazu
 * ----------------------------------------------------------------- */

static void cmd_status(void)
{
    char t[8];
    fmt_temp(g_temp, t);          printf("TEMP:%s\n",  t);
    fmt_temp(g_settings.sp, t);   printf("SP:%s\n",    t);

    if      (relay_get_heating()) printf("STATE:HEATING\n");
    else if (relay_get_cooling()) printf("STATE:COOLING\n");
    else                          printf("STATE:IDLE\n");

    uint8_t h, m, s, d, mo, y, day;
    ds1307_get_time(&h, &m, &s);
    ds1307_get_date(&d, &mo, &y, &day);
    printf("TIME:%02u:%02u\n",       h, m);
    printf("DATE:%02u.%02u.%02u\n",  d, mo, y);
    printf("HYST:%u.%u\n",  g_settings.hyst/10, g_settings.hyst%10);
    fmt_temp(g_settings.cal, t);  printf("CAL:%s\n",   t);
    printf("SCHED:EN:%u\n", g_settings.sched_en);
}

static void cmd_get(const char *p)
{
    char t[8];
    p = skip_sp(p);

    if (strncmp(p, "TEMP",  4) == 0) { fmt_temp(g_temp, t);        printf("TEMP:%s\n", t); }
    else if (strncmp(p, "SP",   2) == 0) { fmt_temp(g_settings.sp, t); printf("SP:%s\n",   t); }
    else if (strncmp(p, "STATE",5) == 0)
    {
        if      (relay_get_heating()) printf("STATE:HEATING\n");
        else if (relay_get_cooling()) printf("STATE:COOLING\n");
        else                          printf("STATE:IDLE\n");
    }
    else if (strncmp(p, "TIME", 4) == 0)
    {
        uint8_t h, m, s; ds1307_get_time(&h, &m, &s);
        printf("TIME:%02u:%02u\n", h, m);
    }
    else if (strncmp(p, "DATE", 4) == 0)
    {
        uint8_t d, mo, y, day; ds1307_get_date(&d, &mo, &y, &day);
        printf("DATE:%02u.%02u.%02u\n", d, mo, y);
    }
    else if (strncmp(p, "HYST", 4) == 0) { printf("HYST:%u.%u\n", g_settings.hyst/10, g_settings.hyst%10); }
    else if (strncmp(p, "CAL",  3) == 0) { fmt_temp(g_settings.cal, t); printf("CAL:%s\n", t); }
    else if (strncmp(p, "SCHED",5) == 0)
    {
        printf("SCHED:EN:%u\n", g_settings.sched_en);
        const char *gr[2] = {"WD","WE"};
        for (uint8_t g = 0; g < 2; g++) {
            sched_slot_t *sl = g ? g_settings.we : g_settings.wd;
            for (uint8_t i = 0; i < 4; i++) {
                fmt_temp(sl[i].sp, t);
                printf("SCHED:%s:%u:%u:%02u:%02u:%s\n",
                    gr[g], i+1, sl[i].enabled, sl[i].hour, sl[i].min, t);
            }
        }
    }
    else { printf("ERR:UNKNOWN\n"); }
}

static void cmd_set(const char *p)
{
    p = skip_sp(p);

    /* SET SP XX.X */
    if (strncmp(p, "SP ", 3) == 0)
    {
        int16_t sp = read_temp(p + 3, &p);
        if (sp < THERMOSTAT_SP_MIN || sp > THERMOSTAT_SP_MAX) { printf("ERR:RANGE\n"); return; }
        thermostat_set_manual_sp(sp);
        settings_save();
        printf("OK\n");
    }

    /* SET HYST X.X */
    else if (strncmp(p, "HYST ", 5) == 0)
    {
        int16_t h = read_temp(p + 5, &p);
        if (h < 1 || h > 50) { printf("ERR:RANGE\n"); return; }
        g_settings.hyst = (uint8_t)h;
        settings_save();
        printf("OK\n");
    }

    /* SET CAL +/-X.X */
    else if (strncmp(p, "CAL ", 4) == 0)
    {
        int16_t cal = read_temp(p + 4, &p);
        if (cal < -50 || cal > 50) { printf("ERR:RANGE\n"); return; }
        g_settings.cal = (int8_t)cal;
        settings_save();
        printf("OK\n");
    }

    /* SET TIME HH MM */
    else if (strncmp(p, "TIME ", 5) == 0)
    {
        p += 5;
        uint8_t h  = read_u8(p, &p);
        uint8_t m  = read_u8(p, &p);
        if (h > 23 || m > 59) { printf("ERR:RANGE\n"); return; }
        ds1307_time_t t;
        ds1307_get_time(&t.hour, &t.min, &t.sec);
        ds1307_get_date(&t.date, &t.month, &t.year, &t.day);
        t.hour = h; t.min = m; t.sec = 0;
        t.day  = ds1307_day_of_week((uint16_t)(2000u + t.year), t.month, t.date);
        ds1307_set_datetime(&t);
        printf("OK\n");
    }

    /* SET DATE DD MM YY */
    else if (strncmp(p, "DATE ", 5) == 0)
    {
        p += 5;
        uint8_t d  = read_u8(p, &p);
        uint8_t mo = read_u8(p, &p);
        uint8_t y  = read_u8(p, &p);
        if (d < 1 || d > 31 || mo < 1 || mo > 12 || y > 99) { printf("ERR:RANGE\n"); return; }
        ds1307_time_t t;
        ds1307_get_time(&t.hour, &t.min, &t.sec);
        t.date = d; t.month = mo; t.year = y;
        t.day  = ds1307_day_of_week((uint16_t)(2000u + y), mo, d);
        ds1307_set_datetime(&t);
        printf("OK\n");
    }

    /* SET SCHED EN 0|1 */
    else if (strncmp(p, "SCHED EN ", 9) == 0)
    {
        uint8_t en = read_u8(p + 9, &p);
        if (en > 1) { printf("ERR:RANGE\n"); return; }
        g_settings.sched_en = en;
        settings_save();
        printf("OK\n");
    }

    /* SET SCHED WD|WE N EN HH MM XX.X */
    else if (strncmp(p, "SCHED WD ", 9) == 0 || strncmp(p, "SCHED WE ", 9) == 0)
    {
        uint8_t is_we = (p[6] == 'E');
        p += 9;
        uint8_t  idx = read_u8(p, &p);
        uint8_t  en  = read_u8(p, &p);
        uint8_t  h   = read_u8(p, &p);
        uint8_t  m   = read_u8(p, &p);
        int16_t  sp  = read_temp(p, &p);
        if (idx < 1 || idx > 4 || en > 1 || h > 23 || m > 59
            || sp < THERMOSTAT_SP_MIN || sp > THERMOSTAT_SP_MAX)
            { printf("ERR:RANGE\n"); return; }
        sched_slot_t *sl = is_we ? &g_settings.we[idx-1] : &g_settings.wd[idx-1];
        sl->enabled = en; sl->hour = h; sl->min = m; sl->sp = sp;
        settings_save();
        printf("OK\n");
    }

    else { printf("ERR:UNKNOWN\n"); }
}

static void cmd_help(void)
{
    printf("GET TEMP|SP|STATE|TIME|DATE|HYST|CAL|SCHED\n");
    printf("SET SP <XX.X>\n");
    printf("SET TIME <HH MM>\n");
    printf("SET DATE <DD MM YY>\n");
    printf("SET HYST <X.X>\n");
    printf("SET CAL <+/-X.X>\n");
    printf("SET SCHED EN <0|1>\n");
    printf("SET SCHED <WD|WE> <1-4> <EN> <HH MM> <XX.X>\n");
    printf("STATUS | HELP\n");
}

/* -----------------------------------------------------------------
 * uart_cmd_process — neblokujici, volat v hlavni smycce
 * ----------------------------------------------------------------- */
void uart_cmd_process(void)
{
    static char    buf[LINE_BUF_SIZE];
    static uint8_t pos = 0;

    while (uart_rx_available())
    {
        char c = (char)uart_rx_byte();
        if (c == '\r') continue;

        if (c == '\n')
        {
            buf[pos] = '\0';
            pos = 0;
            if (buf[0] == '\0') return;

            const char *p = buf;
            if      (strncmp(p, "STATUS", 6) == 0) cmd_status();
            else if (strncmp(p, "HELP",   4) == 0) cmd_help();
            else if (strncmp(p, "GET ",   4) == 0) cmd_get(p + 4);
            else if (strncmp(p, "SET ",   4) == 0) cmd_set(p + 4);
            else printf("ERR:UNKNOWN\n");
            return;
        }

        if (pos < LINE_BUF_SIZE - 1u)
            buf[pos++] = c;
    }
}
