#include "../include/timer.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define NOP() asm volatile ("nop")

static volatile uint16_t timer1overflowCount = 0;

void busyDelay(uint32_t us)
{
	NOP();
	NOP();
	NOP();
	NOP();
	NOP();
	NOP();
	NOP();
	if (us <= 2)
		return;
	us -= 2;
    for (; us != 0; --us)
    {
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
		NOP();
    }
}

ISR(TIMER1_OVF_vect)
{
	timer1overflowCount++;
}

void timerInit(void)
{
	TCCR1A = 0;              // Normal mode
	TCCR1B = (1 << CS11);   // Start, preddelicka 8
	TIMSK1 = (1 << TOIE1);  // Povoleni preruseni
}

uint32_t getTime(void)
{
	cli();  // Zakazat preruseni, atomicke cteni 16bitove promenne na 8bit MCU
	uint16_t ovf = timer1overflowCount;
	uint16_t cnt = TCNT1;
	// Preteceni nastalo mezi poslednim pretecenim a volanim cli(),
	// ISR jeste nestihla bezet, opravit ovf rucne
	if (TIFR1 & (1 << TOV1))
		ovf++;
	sei();
	return ((uint32_t)ovf << 16) | cnt;
}

void delay(uint32_t us)
{
	uint32_t start = getTime();
	uint32_t ticks = us << 1;  // us * 2: prevod na tiky (0.5 us/tik)

	while ((getTime() - start) < ticks);
}
