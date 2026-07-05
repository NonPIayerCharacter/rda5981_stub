#include "board.h"
#include "drv_uart.h"
#include <rda5981_bootrom_api.h>

/**
 * This function will initialize board.
 */
void board_init()
{
#if defined (__CC_ARM)
    __disable_irq();
#elif defined (__GNUC__)
    __asm__ __volatile__ ("cpsid i");
#elif
#error unknown complier
#endif
}

int HAL_FlashWrite(char* buf, unsigned int len, unsigned int addr)
{
	if(len == 0) return 0;
	//addr += 0x18000000;
	int ret;
	int left;
	unsigned int addr_t, len_t;
	uint8_t temp_buf[256] = {0};
	addr_t = addr & 0xffffff00;
	len_t = addr - addr_t + len;
	if(len_t % 256)
		len_t += 256 - len_t % 256;
	//printf("addr %x addr_t %x\r\n", addr, addr_t);
	//printf("len %d len_t %d\r\n", len, len_t);

	RDA5991H_READ_FLASH(addr_t, temp_buf, 256);
	left = 256 - (addr - addr_t);
	if(len < left)
		memcpy(temp_buf + addr - addr_t, buf, len);//256-(addr-addr_t));
	else
		memcpy(temp_buf + addr - addr_t, buf, left);
	RDA5991H_WRITE_FLASH(addr_t, temp_buf, 256);

	len_t -= 256;
	buf += 256 - (addr - addr_t);
	len -= 256 - (addr - addr_t);
	addr_t += 256;

	while(len_t != 0)
	{
		//printf("len_t %d buf %x len %d addr_t %x\r\n", len_t, buf, len, addr_t);
		if(len >= 256)
		{
			memcpy(temp_buf, buf, 256);
		}
		else
		{
			RDA5991H_READ_FLASH(addr_t, temp_buf, 256);
			memcpy(temp_buf, buf, len);
		}
		RDA5991H_WRITE_FLASH(addr_t, temp_buf, 256);
		len_t -= 256;
		buf += 256;
		len -= 256;
		addr_t += 256;
	}
	return 0;
}
