#include "../include/lcd.h"
#include "../include/lcd_pins.h"
#include "../include/timer.h"

#include <avr/io.h>
#include <stdio.h>

/* DDRAM adresy zacatku radku pro 16x2 display */
#define LCD_ROW0_ADDR   0x00
#define LCD_ROW1_ADDR   0x40

/* HD44780 instrukce */
#define LCD_CMD_CLEAR           0x01
#define LCD_CMD_HOME            0x02
#define LCD_CMD_ENTRY_MODE      0x06    /* increment, no shift */
#define LCD_CMD_DISPLAY_OFF     0x08
#define LCD_CMD_DISPLAY_ON      0x0C    /* display on, cursor off, blink off */
#define LCD_CMD_FUNCTION_4BIT   0x28    /* 4-bit, 2 radky, 5x8 */
#define LCD_CMD_SET_DDRAM       0x80

/* Casovani (us) - RW je GND, necteme BF, pouzivame fixni delay */
#define LCD_DELAY_INIT_1        15000   /* >15 ms po power on */
#define LCD_DELAY_INIT_2        4100    /* >4.1 ms */
#define LCD_DELAY_INIT_3        100     /* >100 us */
#define LCD_DELAY_CMD           50      /* >37 us pro bezne prikazy */
#define LCD_DELAY_CLEAR         2000    /* >1.52 ms pro Clear/Home */
#define LCD_DELAY_ENABLE_US     1       /* >450 ns pulz E */

/* ------------------------------------------------------------------ */
/* Stream                                                              */
/* ------------------------------------------------------------------ */

static uint8_t stream_row = 0;

static int lcd_putchar(char c, FILE *stream)
{
    (void)stream;
    if (c == '\n')
    {
        stream_row = (stream_row + 1) & 1;
        lcd_set_cursor(0, stream_row);
    }
    else
    {
        lcd_write_char(c);
    }
    return 0;
}

FILE lcd_stream = FDEV_SETUP_STREAM(lcd_putchar, NULL, _FDEV_SETUP_WRITE);

/* ------------------------------------------------------------------ */
/* Interni funkce                                                       */
/* ------------------------------------------------------------------ */

static void send_nibble(uint8_t nibble)
{
    /* Nastav D4-D7, zachovej dolni 4 bity PORTD */
    LCD_DATA_PORT = (LCD_DATA_PORT & 0x0F) | (nibble << 4);

    /* Pulz na E: high >= 450ns, cycle time >= 1000ns */
    LCD_E_PORT |=  (1 << LCD_E_PIN);
    busyDelay(LCD_DELAY_ENABLE_US);
    LCD_E_PORT &= ~(1 << LCD_E_PIN);
    busyDelay(LCD_DELAY_ENABLE_US);
}

static void send_byte(uint8_t byte, uint8_t rs)
{
	// 0 = prikaz, 1 = data
    if (rs)
        LCD_RS_PORT |=  (1 << LCD_RS_PIN);
    else
        LCD_RS_PORT &= ~(1 << LCD_RS_PIN);
	
	// Doba mezi RS a enable tas = 60 ns	
	busyDelay(LCD_DELAY_ENABLE_US);

    send_nibble(byte >> 4);     /* horni nibble */
    send_nibble(byte & 0x0F);   /* dolni nibble */

    delay(LCD_DELAY_CMD);
}

static void send_cmd(uint8_t cmd)
{
    send_byte(cmd, 0);

    if (cmd == LCD_CMD_CLEAR || cmd == LCD_CMD_HOME)
        delay(LCD_DELAY_CLEAR);
}

/* ------------------------------------------------------------------ */
/* Inicializace (Figure 24 - HD44780 datasheet, 4-bit interface)       */
/* ------------------------------------------------------------------ */

void lcd_init(void)
{
    /* Nastav piny jako vystupy */
    LCD_RS_DDR   |= (1 << LCD_RS_PIN);
    LCD_E_DDR    |= (1 << LCD_E_PIN);
    LCD_DATA_DDR |= (1 << LCD_D4_PIN) | (1 << LCD_D5_PIN) | (1 << LCD_D6_PIN) | (1 << LCD_D7_PIN);

    /* Vsechny vystupy na 0 */
    LCD_RS_PORT  &= ~(1 << LCD_RS_PIN);
    LCD_E_PORT   &= ~(1 << LCD_E_PIN);
    LCD_DATA_PORT &= 0x0F;

    /* Cekej >15 ms po power on */
    delay(LCD_DELAY_INIT_1);

    /* 3x poslat nibble 0x3 - inicializace do 8-bit rezimu (dle datasheetu) */
    send_nibble(0x3);
    delay(LCD_DELAY_INIT_2);

    send_nibble(0x3);
    delay(LCD_DELAY_INIT_3);

    send_nibble(0x3);
    delay(LCD_DELAY_INIT_3);

    /* Prepni do 4-bit rezimu */
    send_nibble(0x2);
    delay(LCD_DELAY_INIT_3);

    /* Od ted posilame plne bajty (2x nibble) */
    send_cmd(LCD_CMD_FUNCTION_4BIT);    /* 4-bit, 2 radky, 5x8 */
    send_cmd(LCD_CMD_DISPLAY_OFF);      /* display off */
    send_cmd(LCD_CMD_CLEAR);            /* vymaz display */
    send_cmd(LCD_CMD_ENTRY_MODE);       /* increment, no shift */
    send_cmd(LCD_CMD_DISPLAY_ON);       /* display on, cursor off */
}

/* ------------------------------------------------------------------ */
/* Verejne funkce                                                       */
/* ------------------------------------------------------------------ */

void lcd_clear(void)
{
    stream_row = 0;
    send_cmd(LCD_CMD_CLEAR);
}

void lcd_home(void)
{
    stream_row = 0;
    send_cmd(LCD_CMD_HOME);
}

void lcd_config(uint8_t display, uint8_t cursor, uint8_t blink)
{
    send_cmd(0x08 | (display ? (1 << 2) : 0)
                  | (cursor  ? (1 << 1) : 0)
                  | (blink   ? (1 << 0) : 0));
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    stream_row = row & 1;
    uint8_t addr = (row == 0) ? LCD_ROW0_ADDR : LCD_ROW1_ADDR;
    send_cmd(LCD_CMD_SET_DDRAM | (addr + col));
}

void lcd_write_char(char c)
{
    send_byte((uint8_t)c, 1);
}

void lcd_write_string(const char *str)
{
    while (*str)
        lcd_write_char(*str++);
}

void lcd_shift_display(uint8_t right)
{
    /* S/C=1 (display shift), R/L=right */
    uint8_t cmd = 0x18 | (right ? (1 << 2) : 0);
    send_cmd(cmd);
}
