#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "../include/uart.h"

/* ---------------------------------------------------------------
 * Baud rate
 * UBRR = F_CPU / (16 * BAUD) - 1
 * Pri 16MHz a 38400 bps → UBRR = 25, chyba 0.16%
 * --------------------------------------------------------------- */
#define F_CPU_HZ  16000000UL
#define UBRR_VAL  (F_CPU_HZ / (16UL * UART_BAUD) - 1)

/* ---------------------------------------------------------------
 * Ring buffer pro RX
 * head — zapisuje ISR
 * tail — cte hlavni program
 * prazdny: head == tail
 * plny:    (head + 1) & mask == tail
 * --------------------------------------------------------------- */
#define RX_BUF_MASK  (UART_RX_BUF_SIZE - 1)

static volatile uint8_t rx_buffer[UART_RX_BUF_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

/* ---------------------------------------------------------------
 * printf podpora — presmerovani stdout na UART
 * \n se prevede na \r\n pro spravne zobrazeni v terminalu
 * presmerovani na uart je ve funkci uart_init()
 * --------------------------------------------------------------- */
static int uart_putchar(char c, FILE *stream)
{
    if (c == '\n')
        uart_tx_byte('\r');
    uart_tx_byte((uint8_t)c);
    return 0;
}
static FILE uart_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

/* ---------------------------------------------------------------
 * ISR — prijde bajt, ulozi se do ring bufferu
 * Pokud je buffer plny, bajt se zahodi
 * --------------------------------------------------------------- */
ISR(USART_RX_vect)
{
    uint8_t data = UDR0;
    uint8_t next_head = (rx_head + 1) & RX_BUF_MASK;

    if (next_head != rx_tail) {
        rx_buffer[rx_head] = data;
        rx_head = next_head;
    }
}

/* ---------------------------------------------------------------
 * Inicializace: 38400 8N1, RX interrupt povolen
 * Po volani zavolej sei() v main()
 * Datasheet atmega: str. 149
 * --------------------------------------------------------------- */
void uart_init(void)
{
	// Volba baudrate
    UBRR0H = (uint8_t)(UBRR_VAL >> 8);
    UBRR0L = (uint8_t)(UBRR_VAL);

    // povoleni RX, TX a preruseni
    UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);

    // 8N1, zadna parita
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    // presmerovani stdout na UART
    stdout = &uart_stdout;
}

/* ---------------------------------------------------------------
 * TX — blocking polling
 * Datasheet atmega: str. 150
 * --------------------------------------------------------------- */
void uart_tx_byte(uint8_t data)
{
	// Cekani na prazdny transmit buffer
    while (!(UCSR0A & (1 << UDRE0)));
	
	// Vlozeni dat do bufferu pro odeslani
    UDR0 = data;
}

void uart_tx_string(const char *str)
{
    while (*str) {
        uart_tx_byte((uint8_t)*str++);
    }
}

/* ---------------------------------------------------------------
 * RX — cte z ring bufferu
 * --------------------------------------------------------------- */
uint8_t uart_rx_available(void)
{
    return (UART_RX_BUF_SIZE + rx_head - rx_tail) & RX_BUF_MASK;
}

uint8_t uart_rx_byte(void)
{
    while (rx_head == rx_tail);

    uint8_t data = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) & RX_BUF_MASK;
    return data;
}
