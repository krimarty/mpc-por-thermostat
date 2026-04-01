/* =============================================================
 * Project:   MPC-POR cviceni 7
 * File:      ds1307.c
 * Author:    Martin Kriz
 * Created:   2026-03-24
 * -------------------------------------------------------------
 * Description:
 *   Implementace knihovny DS1307.
 *   Datasheet DS1307: sekce "Registers" a "I2C Data Transfer"
 *
 * Mapa registru DS1307:
 *   0x00: sekundy  BCD [7]=CH (clock halt, 0=bezi)
 *   0x01: minuty   BCD
 *   0x02: hodiny   BCD [6]=12/24 (0=24h rezim)
 *   0x03: den tydne (1-7, neni BCD)
 *   0x04: datum    BCD
 *   0x05: mesic    BCD
 *   0x06: rok      BCD (00-99)
 *   0x07: control  [7]=OUT [4]=SQWE [1:0]=RS
 * ============================================================= */

#include "../include/ds1307.h"
#include "../include/i2c.h"

/* BCD <-> desitkovka prevod */
static inline uint8_t bcd2dec(uint8_t bcd) { return (uint8_t)((bcd >> 4) * 10u + (bcd & 0x0Fu)); }
static inline uint8_t dec2bcd(uint8_t dec) { return (uint8_t)(((dec / 10u) << 4) | (dec % 10u)); }

/* ------------------------------------------------------------------ */

void ds1307_init(void)
{
    i2c_init();

    /* Precti reg 0 a zkontroluj bit CH (bit 7) */
    uint8_t reg = 0x00;
    uint8_t sec_reg;
    i2c_master_read(DS1307_ADDR, &reg, &sec_reg, 1);

    if (sec_reg & 0x80)
    {
        /* Hodiny jsou zastavene - vymaz bit CH, zachovej sekundy */
        uint8_t val = sec_reg & 0x7F;
        i2c_master_transmit(DS1307_ADDR, &reg, &val, 1);
    }
}

void ds1307_set_datetime(const ds1307_time_t *t)
{
    /* Sekvenční zapis vsech 7 casovych registru najednou (burst write).
     * Adresovy ukazatel DS1307 se po kazdem bajtu automaticky inkrementuje. */
    uint8_t reg = 0x00;
    uint8_t buf[7];
    buf[0] = dec2bcd(t->sec)  & 0x7Fu;     /* reg 0: sekundy, CH=0 */
    buf[1] = dec2bcd(t->min);              /* reg 1: minuty */
    buf[2] = dec2bcd(t->hour) & 0x3Fu;     /* reg 2: hodiny, 24h (bit6=0) */
    buf[3] = t->day;                       /* reg 3: den tydne (neni BCD) */
    buf[4] = dec2bcd(t->date);             /* reg 4: datum */
    buf[5] = dec2bcd(t->month);            /* reg 5: mesic */
    buf[6] = dec2bcd(t->year);             /* reg 6: rok */
    i2c_master_transmit(DS1307_ADDR, &reg, buf, 7);
}

void ds1307_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    /* Precti registry 0 (sec), 1 (min), 2 (hour) */
    uint8_t reg = 0x00;
    uint8_t buf[3];
    i2c_master_read(DS1307_ADDR, &reg, buf, 3);

    *sec  = bcd2dec(buf[0] & 0x7Fu);   /* mask CH bit */
    *min  = bcd2dec(buf[1]);
    *hour = bcd2dec(buf[2] & 0x3Fu);   /* mask 12/24 bit */
}

void ds1307_get_date(uint8_t *date, uint8_t *month, uint8_t *year, uint8_t *day)
{
    /* Precti registry 3 (day), 4 (date), 5 (month), 6 (year) */
    uint8_t reg = 0x03;
    uint8_t buf[4];
    i2c_master_read(DS1307_ADDR, &reg, buf, 4);

    *day   = buf[0];            /* den tydne - neni BCD */
    *date  = bcd2dec(buf[1]);
    *month = bcd2dec(buf[2]);
    *year  = bcd2dec(buf[3]);
}

void ds1307_set_sqw(ds1307_sqw_t mode)
{
    uint8_t reg = 0x07;         /* reg 7: control */
    uint8_t val = (uint8_t)mode;
    i2c_master_transmit(DS1307_ADDR, &reg, &val, 1);
}

/* Zkratky dni tydne, index 1-7 (Ne, Po, Ut, St, Ct, Pa, So) */
const char * const DS1307_DAY_STR[8] = {
    "", "Ne", "Po", "Ut", "St", "Ct", "Pa", "So"
};

uint8_t ds1307_day_of_week(uint16_t year, uint8_t month, uint8_t date)
{
    /* Sakamotov algoritmus — vraci 0=Ne, 1=Po, ..., 6=So
     * Prevedeme na DS1307 format: 1=Ne, 2=Po, ..., 7=So */
    static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3)
        year--;
    uint8_t dow = (uint8_t)((year + year/4u - year/100u + year/400u
                             + t[month - 1u] + date) % 7u);
    /* dow: 0=Ne -> DS1307: 1=Ne, takze +1, wraparound 7->7 */
    return (dow == 0u) ? 1u : (uint8_t)(dow + 1u);
}
