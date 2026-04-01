/* =============================================================
 * Project:   MPC-POR projekt
 * File:      menu.c
 * Author:    Martin Kriz
 * ============================================================= */

#include "../include/menu.h"
#include "../include/lcd.h"
#include "../include/encoder.h"
#include "../include/settings.h"
#include "../include/thermostat.h"
#include "../include/relay.h"
#include "../include/ds1307.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* -----------------------------------------------------------------
 * Stavy
 * ----------------------------------------------------------------- */
typedef enum {
    MS_MAIN,
    MS_ROOT,
    MS_EDIT_HYST,
    MS_EDIT_CAL,
    MS_DATETIME,
    MS_EDIT_DT,           /* sdileny edit stav pro datum/cas, viz dt_field */
    MS_SCHED,
    MS_EDIT_SCHED_EN,
    MS_SCHED_GROUP,       /* seznam slotu WD nebo WE, viz sched_is_we */
    MS_SCHED_SLOT,        /* polozky jednoho slotu */
    MS_EDIT_SLOT,         /* sdileny edit stav pro slot, viz slot_field */
    MS_SAVE
} ms_t;

/* -----------------------------------------------------------------
 * Stavove promenne
 * ----------------------------------------------------------------- */
static ms_t    state       = MS_MAIN;
static uint8_t cursor      = 0;
static uint8_t redraw      = 1;
static uint8_t save_yes    = 1;   /* 1=ANO vybrano v MS_SAVE */
static uint8_t dt_field    = 0;   /* 0=hod,1=min,2=den,3=mes,4=rok */
static uint8_t slot_field  = 0;   /* 0=en,1=hod,2=min,3=sp */
static uint8_t sched_is_we = 0;   /* 0=pracovni dny, 1=vikend */
static uint8_t sched_slot  = 0;   /* index slotu 0-3 */

static ds1307_time_t dt_edit;     /* lokalni buffer pro editaci casu */

/* -----------------------------------------------------------------
 * Pomocne funkce
 * ----------------------------------------------------------------- */

static void set_state(ms_t s) { state = s; cursor = 0; redraw = 1; }

static int16_t clamp16(int16_t v, int16_t lo, int16_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void move_cursor(int16_t delta, uint8_t n)
{
    if (!delta) return;
    cursor = (uint8_t)clamp16((int16_t)cursor + delta, 0, (int16_t)(n - 1u));
    redraw = 1;
}

/* Zapise string s, leve-zarovnany, doplneny mezerami na 16 znaku */
static void draw_row(uint8_t r, const char *s)
{
    char buf[17];
    snprintf(buf, sizeof(buf), "%-16s", s);
    lcd_set_cursor(0, r);
    lcd_write_string(buf);
}

/* Formátuje int16 x10 na "XX.X" nebo "-X.X" */
static void fmt_t(int16_t v, char *buf, uint8_t len)
{
    if (v < 0) snprintf(buf, len, "-%d.%d", (-v) / 10, (-v) % 10);
    else        snprintf(buf, len,  "%d.%d",   v  / 10,   v  % 10);
}

/* -----------------------------------------------------------------
 * Texty polozek menu
 * ----------------------------------------------------------------- */
static const char * const root_items[]  = { "HYSTEREZE", "DATUM & CAS", "KALIBRACE", "SCHEDULER" };
static const char * const dt_items[]    = { "HODINA", "MINUTA", "DEN", "MESIC", "ROK" };
static const char * const sched_items[] = { "POVOLEN", "PRAC.DNY", "VIKEND" };
static const char * const slot_items[]  = { "POVOLEN", "HODINA", "MINUTA", "TEPLOTA" };

/* -----------------------------------------------------------------
 * Draw funkce
 * ----------------------------------------------------------------- */

static void draw_main(void)
{
    char ta[7], ts[7], buf[17];
    fmt_t(g_temp, ta, sizeof(ta));
    fmt_t(g_settings.sp, ts, sizeof(ts));
    char sc = relay_get_heating() ? 'H' : relay_get_cooling() ? 'C' : ' ';
    snprintf(buf, sizeof(buf), "A:%-5sS:%-5s%c", ta, ts, sc);
    draw_row(0, buf);

    uint8_t h, m, s, d, mo, y, day;
    ds1307_get_time(&h, &m, &s);
    ds1307_get_date(&d, &mo, &y, &day);
    snprintf(buf, sizeof(buf), "%s %02u.%02u. %02u:%02u",
             DS1307_DAY_STR[day], d, mo, h, m);
    draw_row(1, buf);
}

static void draw_list(const char *title, const char * const *items, uint8_t cur)
{
    char buf[17];
    draw_row(0, title);
    snprintf(buf, sizeof(buf), ">%s", items[cur]);
    draw_row(1, buf);
}

static void draw_edit(const char *title, const char *val)
{
    char buf[17];
    draw_row(0, title);
    snprintf(buf, sizeof(buf), "> %s", val);
    draw_row(1, buf);
}

static void draw_edit_hyst(void)
{
    char val[10];
    snprintf(val, sizeof(val), "%u.%u C", g_settings.hyst / 10, g_settings.hyst % 10);
    draw_edit("HYSTEREZE", val);
}

static void draw_edit_cal(void)
{
    char val[10];
    fmt_t(g_settings.cal, val, sizeof(val));
    uint8_t l = (uint8_t)strlen(val);
    snprintf(val + l, (uint8_t)(sizeof(val) - l), " C");
    draw_edit("KALIBRACE", val);
}

static void draw_edit_dt(void)
{
    char val[8];
    switch (dt_field) {
        case 0: snprintf(val, sizeof(val), "%02u", dt_edit.hour);  break;
        case 1: snprintf(val, sizeof(val), "%02u", dt_edit.min);   break;
        case 2: snprintf(val, sizeof(val), "%02u", dt_edit.date);  break;
        case 3: snprintf(val, sizeof(val), "%02u", dt_edit.month); break;
        case 4: snprintf(val, sizeof(val), "%02u", dt_edit.year);  break;
        default: break;
    }
    draw_edit(dt_items[dt_field], val);
}

static void draw_sched_group(void)
{
    sched_slot_t *slots = sched_is_we ? g_settings.we : g_settings.wd;
    char buf[17];
    draw_row(0, sched_is_we ? "VIKEND" : "PRAC.DNY");
    sched_slot_t *sl = &slots[cursor];
    if (sl->enabled)
        snprintf(buf, sizeof(buf), ">SLOT %u: %02u:%02u", cursor + 1u, sl->hour, sl->min);
    else
        snprintf(buf, sizeof(buf), ">SLOT %u: --:--", cursor + 1u);
    draw_row(1, buf);
}

static void draw_sched_slot(void)
{
    sched_slot_t *sl = sched_is_we
                       ? &g_settings.we[sched_slot]
                       : &g_settings.wd[sched_slot];
    char r0[17], r1[17], val[8];
    snprintf(r0, sizeof(r0), "SLOT %u %s", sched_slot + 1u, sched_is_we ? "VIK" : "PD");
    draw_row(0, r0);
    switch (cursor) {
        case 0: snprintf(r1, sizeof(r1), ">POVOLEN: %s", sl->enabled ? "ANO" : "NE"); break;
        case 1: snprintf(r1, sizeof(r1), ">HODINA:  %02u",  sl->hour); break;
        case 2: snprintf(r1, sizeof(r1), ">MINUTA:  %02u",  sl->min);  break;
        case 3: fmt_t(sl->sp, val, sizeof(val));
                snprintf(r1, sizeof(r1), ">TEPLOTA: %s",    val);       break;
        default: break;
    }
    draw_row(1, r1);
}

static void draw_edit_slot(void)
{
    sched_slot_t *sl = sched_is_we
                       ? &g_settings.we[sched_slot]
                       : &g_settings.wd[sched_slot];
    char val[8];
    switch (slot_field) {
        case 0: snprintf(val, sizeof(val), "%s",   sl->enabled ? "ANO" : "NE"); break;
        case 1: snprintf(val, sizeof(val), "%02u", sl->hour); break;
        case 2: snprintf(val, sizeof(val), "%02u", sl->min);  break;
        case 3: fmt_t(sl->sp, val, sizeof(val)); break;
        default: break;
    }
    draw_edit(slot_items[slot_field], val);
}

static void draw_save(void)
{
    char buf[17];
    draw_row(0, "Ulozit zmeny?");
    snprintf(buf, sizeof(buf), "%sANO    %sNE",
             save_yes ? ">" : " ", save_yes ? " " : ">");
    draw_row(1, buf);
}

/* -----------------------------------------------------------------
 * State handlery
 * ----------------------------------------------------------------- */

static void do_main(int16_t delta, enc_btn_event_t evt)
{
    if (delta)
    {
        thermostat_set_manual_sp(g_settings.sp + delta * 5);
        redraw = 1;
    }
    if (evt == ENC_BTN_SHORT) set_state(MS_ROOT);
}

static void do_root(int16_t delta, enc_btn_event_t evt)
{
    move_cursor(delta, 4);
    if (evt == ENC_BTN_SHORT)
    {
        switch (cursor)
        {
            case 0: set_state(MS_EDIT_HYST); break;
            case 1:
                ds1307_get_time(&dt_edit.hour, &dt_edit.min, &dt_edit.sec);
                ds1307_get_date(&dt_edit.date, &dt_edit.month, &dt_edit.year, &dt_edit.day);
                set_state(MS_DATETIME);
                break;
            case 2: set_state(MS_EDIT_CAL); break;
            case 3: set_state(MS_SCHED); break;
        }
    }
    if (evt == ENC_BTN_LONG) { save_yes = 1; set_state(MS_SAVE); }
}

static void do_edit_hyst(int16_t delta, enc_btn_event_t evt)
{
    if (delta)
    {
        g_settings.hyst = (uint8_t)clamp16((int16_t)g_settings.hyst + delta, 1, 50);
        redraw = 1;
    }
    if (evt == ENC_BTN_SHORT || evt == ENC_BTN_LONG) set_state(MS_ROOT);
}

static void do_edit_cal(int16_t delta, enc_btn_event_t evt)
{
    if (delta)
    {
        g_settings.cal = (int8_t)clamp16((int16_t)g_settings.cal + delta, -50, 50);
        redraw = 1;
    }
    if (evt == ENC_BTN_SHORT || evt == ENC_BTN_LONG) set_state(MS_ROOT);
}

static void do_datetime(int16_t delta, enc_btn_event_t evt)
{
    move_cursor(delta, 5);
    if (evt == ENC_BTN_SHORT) { dt_field = cursor; set_state(MS_EDIT_DT); }
    if (evt == ENC_BTN_LONG)
    {
        /* Prepocitej den tydne a zapis do DS1307 */
        dt_edit.day = ds1307_day_of_week(
            (uint16_t)(2000u + dt_edit.year), dt_edit.month, dt_edit.date);
        ds1307_set_datetime(&dt_edit);
        set_state(MS_ROOT);
    }
}

static void do_edit_dt(int16_t delta, enc_btn_event_t evt)
{
    if (delta)
    {
        switch (dt_field)
        {
            case 0: dt_edit.hour  = (uint8_t)clamp16((int16_t)dt_edit.hour  + delta, 0, 23); break;
            case 1: dt_edit.min   = (uint8_t)clamp16((int16_t)dt_edit.min   + delta, 0, 59); break;
            case 2: dt_edit.date  = (uint8_t)clamp16((int16_t)dt_edit.date  + delta, 1, 31); break;
            case 3: dt_edit.month = (uint8_t)clamp16((int16_t)dt_edit.month + delta, 1, 12); break;
            case 4: dt_edit.year  = (uint8_t)clamp16((int16_t)dt_edit.year  + delta, 0, 99); break;
        }
        redraw = 1;
    }
    if (evt == ENC_BTN_SHORT || evt == ENC_BTN_LONG) set_state(MS_DATETIME);
}

static void do_sched(int16_t delta, enc_btn_event_t evt)
{
    move_cursor(delta, 3);
    if (evt == ENC_BTN_SHORT)
    {
        switch (cursor)
        {
            case 0: set_state(MS_EDIT_SCHED_EN); break;
            case 1: sched_is_we = 0; set_state(MS_SCHED_GROUP); break;
            case 2: sched_is_we = 1; set_state(MS_SCHED_GROUP); break;
        }
    }
    if (evt == ENC_BTN_LONG) set_state(MS_ROOT);
}

static void do_edit_sched_en(int16_t delta, enc_btn_event_t evt)
{
    if (delta > 0) { g_settings.sched_en = 1; redraw = 1; }
    if (delta < 0) { g_settings.sched_en = 0; redraw = 1; }
    if (evt == ENC_BTN_SHORT || evt == ENC_BTN_LONG) set_state(MS_SCHED);
}

static void do_sched_group(int16_t delta, enc_btn_event_t evt)
{
    move_cursor(delta, 4);
    if (evt == ENC_BTN_SHORT) { sched_slot = cursor; set_state(MS_SCHED_SLOT); }
    if (evt == ENC_BTN_LONG)  set_state(MS_SCHED);
}

static void do_sched_slot(int16_t delta, enc_btn_event_t evt)
{
    move_cursor(delta, 4);
    if (evt == ENC_BTN_SHORT) { slot_field = cursor; set_state(MS_EDIT_SLOT); }
    if (evt == ENC_BTN_LONG)  set_state(MS_SCHED_GROUP);
}

static void do_edit_slot(int16_t delta, enc_btn_event_t evt)
{
    sched_slot_t *sl = sched_is_we
                       ? &g_settings.we[sched_slot]
                       : &g_settings.wd[sched_slot];
    if (delta)
    {
        switch (slot_field)
        {
            case 0: sl->enabled = (delta > 0) ? 1u : 0u; break;
            case 1: sl->hour = (uint8_t)clamp16((int16_t)sl->hour + delta, 0, 23); break;
            case 2: sl->min  = (uint8_t)clamp16((int16_t)sl->min  + delta, 0, 59); break;
            case 3: sl->sp   = clamp16(sl->sp + delta * 5, THERMOSTAT_SP_MIN, THERMOSTAT_SP_MAX); break;
        }
        redraw = 1;
    }
    if (evt == ENC_BTN_SHORT || evt == ENC_BTN_LONG) set_state(MS_SCHED_SLOT);
}

static void do_save(int16_t delta, enc_btn_event_t evt)
{
    if (delta) { save_yes = (delta > 0) ? 1u : 0u; redraw = 1; }
    if (evt == ENC_BTN_SHORT)
    {
        if (save_yes) settings_save();
        set_state(MS_MAIN);
    }
    if (evt == ENC_BTN_LONG) set_state(MS_ROOT);
}

/* -----------------------------------------------------------------
 * menu_init / menu_tick / menu_request_redraw
 * ----------------------------------------------------------------- */

void menu_init(void)
{
    state  = MS_MAIN;
    cursor = 0;
    redraw = 1;
}

void menu_request_redraw(void)
{
    if (state == MS_MAIN) redraw = 1;
}

void menu_tick(void)
{
    int16_t         delta = encoder_get_count();
    enc_btn_event_t evt   = encoder_get_btn_event();
    if (delta) encoder_clear_count();

    switch (state)
    {
        case MS_MAIN:          do_main(delta, evt);         break;
        case MS_ROOT:          do_root(delta, evt);         break;
        case MS_EDIT_HYST:     do_edit_hyst(delta, evt);    break;
        case MS_EDIT_CAL:      do_edit_cal(delta, evt);     break;
        case MS_DATETIME:      do_datetime(delta, evt);     break;
        case MS_EDIT_DT:       do_edit_dt(delta, evt);      break;
        case MS_SCHED:         do_sched(delta, evt);        break;
        case MS_EDIT_SCHED_EN: do_edit_sched_en(delta, evt);break;
        case MS_SCHED_GROUP:   do_sched_group(delta, evt);  break;
        case MS_SCHED_SLOT:    do_sched_slot(delta, evt);   break;
        case MS_EDIT_SLOT:     do_edit_slot(delta, evt);    break;
        case MS_SAVE:          do_save(delta, evt);         break;
    }

    if (!redraw) return;
    redraw = 0;

    switch (state)
    {
        case MS_MAIN:          draw_main();                                          break;
        case MS_ROOT:          draw_list("NASTAVENI",   root_items,  cursor);        break;
        case MS_EDIT_HYST:     draw_edit_hyst();                                     break;
        case MS_EDIT_CAL:      draw_edit_cal();                                      break;
        case MS_DATETIME:      draw_list("DATUM & CAS", dt_items,    cursor);        break;
        case MS_EDIT_DT:       draw_edit_dt();                                       break;
        case MS_SCHED:         draw_list("SCHEDULER",   sched_items, cursor);        break;
        case MS_EDIT_SCHED_EN: draw_edit("SCHED POVOLEN",
                                   g_settings.sched_en ? "ANO" : "NE");             break;
        case MS_SCHED_GROUP:   draw_sched_group();                                   break;
        case MS_SCHED_SLOT:    draw_sched_slot();                                    break;
        case MS_EDIT_SLOT:     draw_edit_slot();                                     break;
        case MS_SAVE:          draw_save();                                          break;
    }
}
