/* =============================================================
 * Project:   MPC-POR cviceni 7
 * File:      i2c.h
 * Author:    Martin Kriz
 * Created:   2026-03-24
 * -------------------------------------------------------------
 * Description:
 *   I2C (TWI) master knihovna pro ATmega328P.
 *   SDA = PC4, SCL = PC5 (HW TWI).
 *   SCL frekvence: 100 kHz pri F_CPU = 16 MHz.
 * Notes:
 *   Vsechny funkce jsou blokujici (polling na TWINT).
 * ============================================================= */

#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>

/* R/W bit pro i2c_start() */
#define I2C_WRITE  0
#define I2C_READ   1

/* SCL frekvence */
#define I2C_SCL_FREQ  100000UL

/* ---------------------------------------------------------------
 * Inicializace TWI hardware.
 * Nastavi TWBR pro 100 kHz pri F_CPU 16 MHz.
 * --------------------------------------------------------------- */
void i2c_init(void);

/* ---------------------------------------------------------------
 * Posli START podmínku a SLA+R/W.
 * addr_rw = (adresa_7bit << 1) | I2C_READ nebo I2C_WRITE
 * Navratova hodnota: 0 = uspech (ACK), 1 = chyba
 * Funkce zvladne i Repeated START (vola se znovu bez i2c_stop).
 * --------------------------------------------------------------- */
uint8_t i2c_start(uint8_t addr_rw);

/* ---------------------------------------------------------------
 * Posli STOP podmínku. Uvolni sbernici.
 * --------------------------------------------------------------- */
void i2c_stop(void);

/* ---------------------------------------------------------------
 * Zapis jeden bajt.
 * Navratova hodnota: 0 = ACK prijat, 1 = NACK
 * --------------------------------------------------------------- */
uint8_t i2c_write(uint8_t data);

/* ---------------------------------------------------------------
 * Precti bajt a posli ACK (dalsi bajt bude nasledovat).
 * --------------------------------------------------------------- */
uint8_t i2c_read_ack(void);

/* ---------------------------------------------------------------
 * Precti bajt a posli NACK (posledni bajt v sekvenci).
 * --------------------------------------------------------------- */
uint8_t i2c_read_nack(void);

/* ---------------------------------------------------------------
 * Zapis data na slave.
 * addr - 7-bitova adresa slave
 * reg  - pointer na adresu registru, NULL pokud nepotrebujes
 * data - data k odeslani
 * len  - pocet bajtu
 * Navratova hodnota: 0 = OK, 1 = chyba
 * --------------------------------------------------------------- */
uint8_t i2c_master_transmit(uint8_t addr, uint8_t *reg, uint8_t *data, uint8_t len);

/* ---------------------------------------------------------------
 * Precti data ze slave.
 * addr - 7-bitova adresa slave
 * reg  - pointer na adresu registru, NULL pokud nepotrebujes
 * data - buffer pro prijata data
 * len  - pocet bajtu ke cteni
 * Navratova hodnota: 0 = OK, 1 = chyba
 * --------------------------------------------------------------- */
uint8_t i2c_master_read(uint8_t addr, uint8_t *reg, uint8_t *data, uint8_t len);

#endif /* I2C_H_ */
