#include <stdint.h>

void SystemInit (void) {
#define SCB_VTOR   (*(volatile uint32_t *)0xE000ED08)
    SCB_VTOR = 0x00100000;   /* start of the vector table in RAM */
		//SCB->VTOR = (uint32_t)0x00100000;
}
