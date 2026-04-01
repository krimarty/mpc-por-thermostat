/* =============================================================
 * Project:   MPC-POR projekt
 * File:      uart_cmd.h
 * Author:    Martin Kriz
 * -------------------------------------------------------------
 * Description:
 *   Parsovani a obsluha UART prikazu termostatu.
 *   Protokol: ASCII, zakonceni LF (\n), \r se ignoruje.
 *   Zadny echo — MCU odpovida jen vysledkem.
 *
 * Pouziti:
 *   uart_cmd_process() volat v hlavni smycce.
 *   Funkce je neblokujici — nacita znaky z uart ring bufferu,
 *   sestavuje radek a zpracuje ho pri prijmu \n.
 * ============================================================= */

#ifndef UART_CMD_H_
#define UART_CMD_H_

/* Neblokujici obsluha — volat v hlavni smycce */
void uart_cmd_process(void);

#endif /* UART_CMD_H_ */
