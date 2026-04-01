#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

void busyDelay(uint32_t us);
void timerInit(void);
uint32_t getTime(void);
void delay(uint32_t us);

#endif /* TIMER_H_ */
