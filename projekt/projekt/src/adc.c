#include <avr/io.h>
#include "../include/adc.h"

/* REFS1:0 = 01 -> reference AVCC (5V)
 * ADPS2:0 = 111 -> prescaler /128 -> 16MHz/128 = 125kHz ADC clock */
void adc_init(void)
{
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel)
{
    ADMUX  = (ADMUX & 0xF0) | (channel & 0x0F);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}
