#include "rda_api.h"
#include <rda5981_bootrom_api.h>

void rda5981_spi_flash_erase_64KB_block(uint32_t addr)
{
    spi_wip_reset();
    spi_write_reset();
    WRITE_REG32(FLASH_CTL_TX_CMD_ADDR_REG, CMD_64KB_BLOCK_ERASE | (addr<<8));
    wait_busy_down();
    spi_wip_reset();
}
void rda5981_spi_erase_partition(void *src, uint32_t counts)
{
    uint32_t a4k, a64k, a64kend, a4kend, atmp;
    if (counts > 0x00) {

        a4k  = ((uint32_t)src                          ) & (~((0x01UL << 12) - 0x01UL));
        a64k = ((uint32_t)src + (0x01UL << 16) - 0x01UL) & (~((0x01UL << 16) - 0x01UL));
        a64kend = ((uint32_t)src + counts                          ) & (~((0x01UL << 16) - 0x01UL));
        a4kend  = ((uint32_t)src + counts + (0x01UL << 12) - 0x01UL) & (~((0x01UL << 12) - 0x01UL));

        for (atmp = a4k; atmp < a4kend; atmp += (0x01UL << 12)) {
            if (a64kend > a64k) {
                if (atmp == a64k) {
                    for (; atmp < a64kend; atmp += (0x01UL << 16)) {
                        rda5981_spi_flash_erase_64KB_block(atmp);
                    }
                    if (atmp == a4kend)
                        break;
                }
            }
            rda5981_spi_flash_erase_4KB_sector(atmp);
        }
    }
}
static int ChipHwVersion = 0;
uint32_t SystemCoreClock = RDA_SYS_CLK_FREQUENCY_160M; /*!< System Clock Frequency (Core Clock)*/
uint32_t AHBBusClock     = RDA_BUS_CLK_FREQUENCY_80M; /*!< AHB Bus Clock Frequency (Bus Clock)*/

int rda_ccfg_hwver(void)
{
    if(0 == ChipHwVersion) {
        ChipHwVersion = (int)((RDA_GPIO->REVID >> 16) & 0xFFUL) + 1;
    }
    return ChipHwVersion;

}

static inline void wr_rf_usb_reg(unsigned char a, unsigned short d, int isusb)
{
    while(RF_SPI_REG & (0x01UL << 31));
    while(RF_SPI_REG & (0x01UL << 31));
    RF_SPI_REG = (unsigned int)d | ((unsigned int)a << 16) | (0x01UL << 25) | ((isusb) ? (0x01UL << 27) : 0x00UL);
}

static inline void rd_rf_usb_reg(unsigned char a, unsigned short *d, int isusb)
{
    while(RF_SPI_REG & (0x01UL << 31));
    while(RF_SPI_REG & (0x01UL << 31));
    RF_SPI_REG = ((unsigned int)a << 16) | (0x01UL << 24) | (0x01UL << 25) | ((isusb) ? (0x01UL << 27) : 0x00UL);
    __asm volatile ("nop");
    while(RF_SPI_REG & (0x01UL << 31));
    while(RF_SPI_REG & (0x01UL << 31));
    *d = (unsigned short)(RF_SPI_REG & 0xFFFFUL);
}
void rda_ccfg_ck(void)
{
    unsigned short val = 0U, cfg = 0U;
#if ((SYS_CPU_CLK == CLK_FREQ_160M) && (AHB_BUS_CLK == CLK_FREQ_80M))
#if 0
    const unsigned int trap_ram_code[] = {
        /* addr_ofst, val */
        /* 0x0000 */  0xB8A6F0FEUL,
        /* 0x0004 */  0xBF006008UL,
        /* 0x0008 */  0xBF00BF00UL,
        /* 0x000C */  0xBF00BF00UL,
        /* 0x0010 */  0xBF00BF00UL,
        /* 0x0014 */  0x6848BF00UL,
        /* 0x0018 */  0xD1FC07C0UL,
        /* 0x001C */  0xF000F8DFUL,
        /* 0x0020 */  0x00001EBCUL
    };
#endif
#endif /* CLK_FREQ_160M && CLK_FREQ_80M */

    cfg = (RDA_SCU->CORECFG >> 11) & 0x07U;
    rd_rf_usb_reg(0xA4, &val, 0);
#if ((SYS_CPU_CLK == CLK_FREQ_160M) && (AHB_BUS_CLK == CLK_FREQ_80M))
    /* HCLK inv */
    if(((CLK_FREQ_40M << 1) | CLK_FREQ_40M) == cfg) {
        val |=  (0x01U << 12);
    }
#endif /* CLK_FREQ_160M && CLK_FREQ_80M */
    /* Config CPU & BUS clock */
    cfg ^=  (((SYS_CPU_CLK << 1) | AHB_BUS_CLK) & 0x07U);
    val &= ~(0x07U << 9);   /* bit[11:10] = 2'b00:40M, 2'b01:80M, 2'b1x:160M */
    val |=  (cfg   << 9);   /* bit[9] = 1'b0:40M, 1'b1:80M */
    val &= ~(0x01U);        /* i2c_wakeup_en */
    wr_rf_usb_reg(0xA4, val, 0);

#if ((SYS_CPU_CLK == CLK_FREQ_160M) && (AHB_BUS_CLK == CLK_FREQ_80M))
#if 0
    /* Load RAM code */
    for(val = 0; val < sizeof(trap_ram_code); val += 4) {
        ADDR2REG(TRAP_RAM_BASE + val) = trap_ram_code[val >> 2];
    }
#endif
#endif /* CLK_FREQ_160M && CLK_FREQ_80M */

}