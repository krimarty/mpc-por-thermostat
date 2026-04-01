/* =============================================================
 * Project:   MPC-POR cviceni 4
 * File:      uart.h
 * Author:    Martin Kriz
 * Created:   2026-02-27
 * -------------------------------------------------------------
 * Description:
 *   UART knihovna pro ATmega328P.
 *   Inicializace: 38400 bps, 8N1.
 * Notes:
 *   Po uart_init() je nutne zavolat sei().
 * ============================================================= */

#ifndef UART_H
#define UART_H

#include <stdint.h>

/* ---------------------------------------------------------------
 * Konfigurace
 * --------------------------------------------------------------- */
#define UART_BAUD         38400
#define UART_RX_BUF_SIZE  32

/* ---------------------------------------------------------------
 * Inicializace
 * Nastavi 38400 8N1, povoli TX, RX a RX interrupt.
 * --------------------------------------------------------------- */
void uart_init(void);

/* ---------------------------------------------------------------
 * TX — odesilani dat
 * --------------------------------------------------------------- */
void uart_tx_byte(uint8_t data);	// Odeslani je bytu
void uart_tx_string(const char *str);	// Odeslani celeho stringu

/* ---------------------------------------------------------------
 * RX — interrupt + ring buffer
 * --------------------------------------------------------------- */
uint8_t uart_rx_available(void);    // vrati pocet bytu cekajicich v bufferu
uint8_t uart_rx_byte(void);         // blocking — ceka dokud neprijde bajt

#endif /* UART_H */
