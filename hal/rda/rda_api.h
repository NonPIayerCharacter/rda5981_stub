#ifndef RDA_API_H
#define RDA_API_H

#include <board.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define FLASH_CTL_REG_BASE 0x17fff000
#define FLASH_CTL_TX_CMD_ADDR_REG (FLASH_CTL_REG_BASE + 0x00)
#define CMD_64KB_BLOCK_ERASE (0x000000d8UL)
#define WRITE_REG32(REG, VAL)    ((*(volatile unsigned int*)(REG)) = (unsigned int)(VAL))

#define ADDR2REG(addr)          (*((volatile unsigned int *)(addr)))

#define RF_SPI_REG              ADDR2REG(0x4001301CUL)
#define RDA_SYS_CLK_FREQUENCY_40M                            ( 40000000UL)
#define RDA_SYS_CLK_FREQUENCY_80M                            ( 80000000UL)
#define RDA_SYS_CLK_FREQUENCY_160M                           (160000000UL)
#define RDA_BUS_CLK_FREQUENCY_40M                            ( 40000000UL)
#define RDA_BUS_CLK_FREQUENCY_80M                            ( 80000000UL)
#define CLK_FREQ_40M            (0x00U)
#define CLK_FREQ_80M            (0x01U)
#define CLK_FREQ_160M           (0x02U)
#define SYS_CPU_CLK             CLK_FREQ_160M
#define AHB_BUS_CLK             CLK_FREQ_80M
#define __I     volatile const       /*!< Defines 'read only' permissions                 */
#define __O     volatile             /*!< Defines 'write only' permissions                */
#define __IO    volatile             /*!< Defines 'read / write' permissions              */
typedef struct
{
	__IO uint32_t CLKGATE0;               /* 0x00 : Clock Gating 0              */
	__IO uint32_t PWRCTRL;                /* 0x04 : Power Control               */
	__IO uint32_t CLKGATE1;               /* 0x08 : Clock Gating 1              */
	__IO uint32_t CLKGATE2;               /* 0x0C : Clock Gating 2              */
	__IO uint32_t RESETCTRL;              /* 0x10 : Power Control               */
	__IO uint32_t CLKGATE3;               /* 0x14 : Clock Gating 3              */
	__IO uint32_t CORECFG;                /* 0x18 : Core Config                 */
	__IO uint32_t CPUCFG;                 /* 0x1C : CPU Config                  */
	__IO uint32_t FTMRINITVAL;            /* 0x20 : Free Timer Initial Value    */
	__IO uint32_t FTMRTS;                 /* 0x24 : Free Timer Timestamp        */
	__IO uint32_t CLKGATEBP;              /* 0x28 : Clock Gating Bypass         */
	uint32_t RESERVED0[2];
	__IO uint32_t PWMCFG;                 /* 0x34 : PWM Config                  */
	__IO uint32_t FUN0WAKEVAL;            /* 0x38 : SDIO Func0 Wake Val         */
	__IO uint32_t FUN1WAKEVAL;            /* 0x3C : SDIO Func1 Wake Val         */
	__IO uint32_t BOOTJUMPADDR;           /* 0x40 : Boot Jump Addr              */
	__IO uint32_t SDIOINTVAL;             /* 0x44 : SDIO Int Val                */
	__IO uint32_t I2SCLKDIV;              /* 0x48 : I2S Clock Divider           */
	__IO uint32_t BOOTJUMPADDRCFG;        /* 0x4C : Boot Jump Addr Config       */
	__IO uint32_t FTMRPREVAL;             /* 0x50 : Free Timer Prescale Init Val*/
	__IO uint32_t PWROPENCFG;             /* 0x54 : Power Open Config           */
	__IO uint32_t PWRCLOSECFG;            /* 0x58 : Power Close Config          */
} RDA_SCU_TypeDef;

typedef struct
{
	__IO uint32_t CTRL;                   /* 0x00 : GPIO Control                */
	uint32_t RESERVED0;
	__IO uint32_t DOUT;                   /* 0x08 : GPIO Data Output            */
	__IO uint32_t DIN;                    /* 0x0C : GPIO Data Input             */
	__IO uint32_t DIR;                    /* 0x10 : GPIO Direction              */
	__IO uint32_t SLEW0;                  /* 0x14 : GPIO Slew Config 0          */
	__IO uint32_t SLEWIOMUX;              /* 0x18 : GPIO IOMUX Slew Config      */
	__IO uint32_t INTCTRL;                /* 0x1C : GPIO Interrupt Control      */
	__IO uint32_t IFCTRL;                 /* 0x20 : Interface Control           */
	__IO uint32_t SLEW1;                  /* 0x24 : GPIO Slew Config 1          */
	__IO uint32_t REVID;                  /* 0x28 : ASIC Reversion ID           */
	__IO uint32_t LPOSEL;                 /* 0x2C : LPO Select                  */
	uint32_t RESERVED1;
	__IO uint32_t INTSEL;                 /* 0x34 : GPIO Interrupt Select       */
	uint32_t RESERVED2;
	__IO uint32_t SDIOCFG;                /* 0x3C : SDIO Config                 */
	__IO uint32_t MEMCFG;                 /* 0x40 : Memory Config               */
	__IO uint32_t IOMUXCTRL[8];           /* 0x44 - 0x60 : IOMUX Control        */
	__IO uint32_t PCCTRL;                 /* 0x64 : Pulse Counter Control       */
} RDA_GPIO_TypeDef;

typedef struct
{
	union
	{
		__I  uint32_t RBR;            /* 0x00 : UART Receive buffer register      */
		__O  uint32_t THR;            /* 0x00 : UART Transmit holding register    */
		__IO uint32_t DLL;            /* 0x00 : UART Divisor latch(low)           */
	};
	union
	{
		__IO uint32_t DLH;            /* 0x04 : UART Divisor latch(high)          */
		__IO uint32_t IER;            /* 0x04 : UART Interrupt enable register    */
	};
	union
	{
		__I uint32_t IIR;             /* 0x08 : UART Interrupt id register        */
		__O uint32_t FCR;             /* 0x08 : UART Fifo control register        */
	};
	__IO uint32_t LCR;              /* 0x0C : UART Line control register        */
	__IO uint32_t MCR;              /* 0x10 : UART Moderm control register      */
	__I  uint32_t LSR;              /* 0x14 : UART Line status register         */
	__I  uint32_t MSR;              /* 0x18 : UART Moderm status register       */
	__IO uint32_t SCR;              /* 0x1C : UART Scratchpad register          */
	__I  uint32_t FSR;              /* 0x20 : UART FIFO status register         */
	__IO uint32_t FRR;              /* 0x24 : UART FIFO tx/rx trigger resiger   */
	__IO uint32_t DL2;              /* 0x28 : UART Baud rate adjust register    */
	__I  uint32_t RESERVED0[4];
	__I  uint32_t BAUD;             /* 0x3C : UART Auto baud counter            */
	__I  uint32_t DL_SLOW;          /* 0x40 : UART Divisor Adjust when slow clk */
	__I  uint32_t DL_FAST;          /* 0x44 : UART Divisor Adjust when fast clk */
} RDA_UART_TypeDef;

typedef struct
{
  __IO uint32_t WDTCFG;
} RDA_WDT_TypeDef;

#define RDA_AHB0_BASE         (0x40000000UL)
#define RDA_APB_BASE          (RDA_AHB0_BASE)
#define RDA_SCU_BASE          (RDA_APB_BASE  + 0x00000)
#define RDA_GPIO_BASE         (RDA_APB_BASE  + 0x01000)
#define RDA_WDT_BASE          (RDA_SCU_BASE  + 0x0000C)
#define RDA_SCU               ((RDA_SCU_TypeDef       *) RDA_SCU_BASE      )
#define RDA_GPIO              ((RDA_GPIO_TypeDef      *) RDA_GPIO_BASE     )
#define RDA_WDT               ((RDA_WDT_TypeDef       *) RDA_WDT_BASE      )

#define UART_BASE ((volatile uint32_t *)0x40012000U)
#define RXFIFO_EMPTY_MASK (0x01UL << 0)

#define WDT_EN_BIT                  (9)
#define WDT_CLR_BIT                 (10)
#define WDT_TMRCNT_OFST             (11)
#define WDT_TMRCNT_WIDTH            (4)

#endif /*RDA_API_H*/
