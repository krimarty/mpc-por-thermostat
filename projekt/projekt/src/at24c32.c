/* =============================================================
 * Project:   MPC-POR cviceni 7
 * File:      at24c32.c
 * Author:    Martin Kriz
 * Created:   2026-03-25
 * -------------------------------------------------------------
 * Description:
 *   Implementace knihovny AT24C32N.
 *
 * Mapa operaci AT24C32N:
 *   Zapis (Page Write, max 32B):
 *     START -> SLA+W -> AddrHigh -> AddrLow -> Data[0..n] -> STOP
 *     Pak cekej 5 ms (tWR).
 *
 *   Cteni (Random Read):
 *     START -> SLA+W -> AddrHigh -> AddrLow -> STOP
 *     -> START -> SLA+R -> Data[0..n-1] ACK -> Data[n] NACK -> STOP
 *
 * Datasheet AT24C32N: str. 7 (Write), str. 9 (Read)
 * ============================================================= */

#include "../include/at24c32.h"
#include "../include/i2c.h"
#include "../include/timer.h"
#include <stddef.h>

/* Write cycle time dle datasheetu: max 5 ms */
#define AT24C32_TWR_US  5000UL

/* Max page size AT24C32 + 1 byte AddrLow */
#define AT24C32_BUF_SIZE  (32u + 1u)

/* ------------------------------------------------------------------ */

void at24c32_write(uint16_t addr, const uint8_t *data, uint8_t len)
{
    /* i2c_master_transmit posila: START -> SLA+W -> AddrHigh -> buf[0..len] -> STOP
     * buf[0] = AddrLow, buf[1..] = data */
    if (len > 32u)
        len = 32u;

    uint8_t addr_high = (uint8_t)(addr >> 8);
    uint8_t buf[AT24C32_BUF_SIZE];

    buf[0] = (uint8_t)(addr & 0xFFu);      /* AddrLow */
    for (uint8_t i = 0; i < len; i++)
        buf[i + 1u] = data[i];

    i2c_master_transmit(AT24C32_ADDR, &addr_high, buf, (uint8_t)(len + 1u));

    /* Cekej na dokonceni interniho zapisu EEPROM */
    delay(AT24C32_TWR_US);
}

void at24c32_read(uint16_t addr, uint8_t *data, uint8_t len)
{
    /* Nastav interni adresovy ukazatel EEPROM (dummy write) */
    uint8_t addr_high = (uint8_t)(addr >> 8);
    uint8_t addr_low  = (uint8_t)(addr & 0xFFu);
    i2c_master_transmit(AT24C32_ADDR, &addr_high, &addr_low, 1u);

    /* Cteni od nastavene adresy (reg = NULL = bez dalsiho zapisu adresy) */
    i2c_master_read(AT24C32_ADDR, NULL, data, len);
}
