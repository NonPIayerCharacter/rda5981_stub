#include <board.h>
#include <rda_api.h>
#include <rda5981_bootrom_api.h>
#include <drv_uart.h>

int uart_putc(char c)
{
	UART_SEND_BYTE(c);
	return 1;
}

void uart_fifo_reset()
{
	((RDA_UART_TypeDef*)UART_BASE)->FCR = 1 << 0 | 1 << 1;
}

int8_t uart_getc(char* ch, uint64_t timeout)
{
	uint64_t loops = 0;
	uint64_t max_loops = 10000 * timeout;
	volatile RDA_UART_TypeDef* UART = (volatile RDA_UART_TypeDef*)UART_BASE;

	while(!(UART->LSR & RXFIFO_EMPTY_MASK))
	{
		if(loops++ >= max_loops)
			return -1;
	}

	*ch = UART_RECV_BYTE();
	return 0;
}

extern uint32_t AHBBusClock;
void uart_set_baud(uint32_t baud)
{
	uint32_t baud_divisor;
	uint32_t baud_mod;
	uint32_t ier_temp;

	baud_divisor = (AHBBusClock / baud) >> 4;
	baud_mod = (AHBBusClock / baud) & 0x0F;

	ier_temp = ((RDA_UART_TypeDef*)UART_BASE)->IER;
	((RDA_UART_TypeDef*)UART_BASE)->LCR |= (1 << 7); //enable load devisor register
	((RDA_UART_TypeDef*)UART_BASE)->DLL = (baud_divisor >> 0) & 0xFF;
	((RDA_UART_TypeDef*)UART_BASE)->DLH = (baud_divisor >> 8) & 0xFF;
	((RDA_UART_TypeDef*)UART_BASE)->LCR &= ~(1 << 7);// after loading, disable load devisor register
	((RDA_UART_TypeDef*)UART_BASE)->IER = ier_temp;
	((RDA_UART_TypeDef*)UART_BASE)->LCR |= (1 << 7); //enable load devisor register
	((RDA_UART_TypeDef*)UART_BASE)->DL2 = (baud_mod >> 1) + ((baud_mod - (baud_mod >> 1)) << 4);
	((RDA_UART_TypeDef*)UART_BASE)->LCR &= ~(1 << 7);// after loading, disable load devisor register
}

void uart_console_output(const char* str, int length)
{
	int index;

	for(index = 0; index < length; index++)
	{
		if(*str == '\n')
			uart_putc('\r');

		uart_putc(*str);
		str++;
	}
}
