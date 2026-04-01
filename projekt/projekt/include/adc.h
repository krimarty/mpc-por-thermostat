/* =============================================================
 * File:      adc.h
 * Description:
 *   ADC knihovna pro ATmega328P.
 *   Single conversion mode, reference AVCC, prescaler /128.
 * Notes:
 *   Datasheet ATmega328P: str. 205-220
 * ============================================================= */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void     adc_init(void);
uint16_t adc_read(uint8_t channel);

#endif /* ADC_H */
