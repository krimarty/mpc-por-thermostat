/* =============================================================
 * Project:   MPC-POR cviceni 7
 * File:      i2c.c
 * Author:    Martin Kriz
 * Created:   2026-03-24
 * -------------------------------------------------------------
 * Description:
 *   Implementace I2C (TWI) master pro ATmega328P.
 *   Datasheet ATmega328P: sekce 21 (Two-Wire Serial Interface)
 * ============================================================= */

#include "../include/i2c.h"

#include <avr/io.h>
#include <stddef.h>     /* NULL */
#include <util/twi.h>   /* TW_STATUS, TW_START, TW_REP_START atd. */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* ------------------------------------------------------------------ */

void i2c_init(void)
{
    //Prescaler = 1 (TWPS1:0 = 00)
    TWSR = 0;

    // Datasheet ATmega328P: rovnice 21-1 */
    TWBR = (uint8_t)((F_CPU / I2C_SCL_FREQ - 16UL) / 2UL);
}

uint8_t i2c_start(uint8_t addr_rw)
{
    /* [1] Send START condition */
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    /* [2] Wait for TWINT flag set - START condition has been transmitted */
    while (!(TWCR & (1 << TWINT)));

    /* [3] Check status - ocekavame START (0x08) nebo REP_START (0x10) */
    if (TW_STATUS != TW_START && TW_STATUS != TW_REP_START)
        return 1;

    /* [4] Load SLA+R/W into TWDR, clear TWINT to start transmission of address */
    TWDR = addr_rw;
    TWCR = (1 << TWINT) | (1 << TWEN);

    /* [4] Wait for TWINT flag set - SLA+W/R has been transmitted, ACK/NACK received */
    while (!(TWCR & (1 << TWINT)));

    /* [5] Check status - ocekavame ACK od slave:
     * 0x18 = SLA+W ACK (master transmitter)
     * 0x40 = SLA+R ACK (master receiver) */
    if (TW_STATUS != TW_MT_SLA_ACK && TW_STATUS != TW_MR_SLA_ACK)
        return 1;

    return 0;
}

void i2c_stop(void)
{
    /* [8] Transmit STOP condition */
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    /* Cekej na dokonceni STOP - bit TWSTO je hardwarove vynulovan */
    while (TWCR & (1 << TWSTO));
}

uint8_t i2c_write(uint8_t data)
{
    /* [5] Load DATA into TWDR, clear TWINT to start transmission */
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);

    /* [6] Wait for TWINT flag set - DATA has been transmitted, ACK/NACK received */
    while (!(TWCR & (1 << TWINT)));

    /* [7] Check status - 0x28 = data odeslana, ACK prijat */
    return (TW_STATUS != TW_MT_DATA_ACK) ? 1 : 0;
}

uint8_t i2c_read_ack(void)
{
    /* TWEA = 1: po prijeti bajtu posli ACK (ocekavame dalsi bajt) */
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t i2c_read_nack(void)
{
    /* TWEA = 0: po prijeti bajtu posli NACK (posledni bajt) */
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t i2c_master_transmit(uint8_t addr, uint8_t *reg, uint8_t *data, uint8_t len)
{
    if (i2c_start((addr << 1) | I2C_WRITE))
        return 1;

    if (reg != NULL) {
        if (i2c_write(*reg))
            { i2c_stop(); return 1; }
    }

    for (uint8_t i = 0; i < len; i++) {
        if (i2c_write(data[i]))
            { i2c_stop(); return 1; }
    }

    i2c_stop();
    return 0;
}

uint8_t i2c_master_read(uint8_t addr, uint8_t *reg, uint8_t *data, uint8_t len)
{
    if (reg != NULL) {
        if (i2c_start((addr << 1) | I2C_WRITE))
            return 1;
        if (i2c_write(*reg))
            { i2c_stop(); return 1; }
    }

    if (i2c_start((addr << 1) | I2C_READ))
        { i2c_stop(); return 1; }

    for (uint8_t i = 0; i < len; i++) {
        data[i] = (i < len - 1) ? i2c_read_ack() : i2c_read_nack();
    }

    i2c_stop();
    return 0;
}
