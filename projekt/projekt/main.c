/* =============================================================
 * Project:   MPC-POR projekt
 * File:      main.c
 * Author:    Martin Kriz
 * -------------------------------------------------------------
 * Hlavni smycka:
 *   - menu_tick()         kazda iterace (enkoder + LCD)
 *   - uart_cmd_process()  kazda iterace (neblokujici)
 *   - menu_request_redraw() kazdou 1 s (aktualizace hodin na displeji)
 *   - thermostat_tick()   kazdych 5 s (mereni teploty + regulace)
 *
 * Linker flags: -lm  (kvuli logf v thermostat.c)
 * ============================================================= */

#include <avr/io.h>
#include <avr/interrupt.h>

#include "include/timer.h"
#include "include/adc.h"
#include "include/lcd.h"
#include "include/encoder.h"
#include "include/uart.h"
#include "include/relay.h"
#include "include/ds1307.h"
#include "include/settings.h"
#include "include/thermostat.h"
#include "include/menu.h"
#include "include/uart_cmd.h"

/* getTime() vraci tiky po 0.5 us
 * 250 ms =  500 000 tiku  (refresh teploty + displeje, 4 Hz)
 * 1 s    = 2000 000 tiku  (regulace + scheduler) */
#define TICKS_250MS  500000UL
#define TICKS_1S    2000000UL

int main(void)
{
    /* ----------------------------------------------------------
     * Inicializace
     * ---------------------------------------------------------- */
    timerInit();
    sei();

    adc_init();
    lcd_init();
    lcd_config(1, 0, 0);

    encoder_init();
    uart_init();
    relay_init();
    ds1307_init();

    settings_load();
    settings_load_rtc();   /* obnov cas DS1307 z EEPROM po vypnuti */
    thermostat_init();
    menu_init();

    /* ----------------------------------------------------------
     * Hlavni smycka
     * ---------------------------------------------------------- */
    uint32_t last_250ms = getTime();
    uint32_t last_1s    = getTime();

    while (1)
    {
        uint32_t now = getTime();

        /* Cteni teploty + refresh displeje (4 Hz) */
        if ((now - last_250ms) >= TICKS_250MS)
        {
            last_250ms = now;
            thermostat_sample_temp();
            menu_request_redraw();
        }

        /* Regulace + scheduler kazdou 1 s */
        if ((now - last_1s) >= TICKS_1S)
        {
            last_1s = now;
            thermostat_tick();
        }

        menu_tick();
        uart_cmd_process();
    }
}
