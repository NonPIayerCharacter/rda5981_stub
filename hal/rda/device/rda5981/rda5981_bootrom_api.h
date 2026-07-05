#ifndef RDA5991H_BOOTROM_API_H_
#define RDA5991H_BOOTROM_API_H_

#include "stdbool.h"

typedef signed char             r_s8;           /* 8bit integer type */
typedef short            r_s16;          /* 16bit integer type */
typedef int              r_s32;          /* 32bit integer type */
typedef long long        r_s64;          /* 64bit integer type */
typedef unsigned char           r_u8;           /* 8bit unsigned integer type */
typedef unsigned short          r_u16;          /* 16bit unsigned integer type */
typedef unsigned int            r_u32;          /* 32bit unsigned integer type */
typedef unsigned long long      r_u64;          /* 64bit unsigned integer type */
typedef bool                    r_bool;         /* boolean type */
typedef void                    r_void;         /* void */

#define UART_SEND_BYTE_ADDR 0x1bb5//uart_send_byte
#define UART_RECV_BYTE_ADDR 0x1c77//uart_recv_byte
#define CRC32_ADDR 0x8dff//u32 crc32(const u8 *p, size_t len)
#define FLASH_ERASE_FUN_ADDR 0x21f1//smartlink_erase_for_mbed
#define FLASH_WRITE_FUN_ADDR 0x2241//smartlink_write_for_mbed
#define FLASH_READ_FUN_ADDR 0x2243//smartlink_read_for_mbed
#define FLASH_ERASE_PARTITION_FUN_ADDR 0x2139//spi_flash_erase_partition
#define SPI_FLASH_READ_DATA_FOR_MBED_ADDR 0x2007//spi_flash_read_data_for_mbed
#define spi_flash_disable_cache_addr 0x1eb7//spi_flash_disable_cache
#define spi_flash_flush_cache_addr 0x1ecd//spi_flash_flush_cache
#define spi_flash_cfg_cache_addr 0x1e9f//spi_flash_cfg_cache
#define spi_flash_erase_4KB_sector_addr 0x23a3
#define spi_wip_reset_addr 0x1d8b
#define spi_write_reset_addr 0x1d9f
#define wait_busy_down_addr 0x1d81

#define UART_SEND_BYTE_ADDR_4 0x1be5//uart_send_byte
#define UART_RECV_BYTE_ADDR_4 0x1ca7//uart_recv_byte
#define CRC32_ADDR_4 0x8e33//u32 crc32(const u8 *p, size_t len)
#define FLASH_ERASE_FUN_ADDR_4 0x2221//smartlink_erase_for_mbed
#define FLASH_WRITE_FUN_ADDR_4 0x2271//smartlink_write_for_mbed
#define FLASH_READ_FUN_ADDR_4 0x2273//smartlink_read_for_mbed
#define FLASH_ERASE_PARTITION_FUN_ADDR_4 0x2169//spi_flash_erase_partition
#define SPI_FLASH_READ_DATA_FOR_MBED_ADDR_4 0x2037//spi_flash_read_data_for_mbed
#define spi_flash_disable_cache_addr_4 0x1ee7//spi_flash_disable_cache
#define spi_flash_flush_cache_addr_4 0x1efd//spi_flash_flush_cache
#define spi_flash_cfg_cache_addr_4 0x1ecf//spi_flash_cfg_cache
#define spi_flash_erase_4KB_sector_addr_4 0x23d3
#define spi_wip_reset_addr_4 0x1dbb
#define spi_write_reset_addr_4 0x1dcf
#define wait_busy_down_addr_4 0x1db1

int rda_ccfg_hwver(void);

static inline void wait_busy_down_2(void)
{
    ((void(*)(void))wait_busy_down_addr)();
}

static inline void spi_write_reset_2(void)
{
    ((void(*)(void))spi_write_reset_addr)();
}

static inline void spi_wip_reset_2(void)
{
    ((void(*)(void))spi_wip_reset_addr)();
}

static inline void wait_busy_down_4(void)
{
    ((void(*)(void))wait_busy_down_addr_4)();
}

static inline void spi_write_reset_4(void)
{
    ((void(*)(void))spi_write_reset_addr_4)();
}

static inline void spi_wip_reset_4(void)
{
    ((void(*)(void))spi_wip_reset_addr_4)();
}

static void wait_busy_down(void)
{
    if (rda_ccfg_hwver() >= 4)
        wait_busy_down_4();
    else
        wait_busy_down_2();
}

static void spi_write_reset(void)
{
    if (rda_ccfg_hwver() >= 4)
        spi_write_reset_4();
    else
        spi_write_reset_2();
}

static void spi_wip_reset(void)
{
    if (rda_ccfg_hwver() >= 4)
        spi_wip_reset_4();
    else
        spi_wip_reset_2();
}

static inline void spi_flash_enable_cache(void)
{
    r_u32 func = spi_flash_cfg_cache_addr;
    if (rda_ccfg_hwver() >= 4) {
        func = spi_flash_cfg_cache_addr_4;
    }
    ((void(*)(void))func)();
}

static inline void spi_flash_disable_cache(void)
{
    r_u32 func = spi_flash_disable_cache_addr;
    if (rda_ccfg_hwver() >= 4) {
        func = spi_flash_disable_cache_addr_4;
    }
    ((void(*)(void))func)();
}

static inline void spi_flash_flush_cache(void)
{
    r_u32 func = spi_flash_flush_cache_addr;
    if (rda_ccfg_hwver() >= 4) {
        func = spi_flash_flush_cache_addr_4;
    }
    ((void(*)(void))func)();
}

static inline void rda5981_spi_flash_erase_4KB_sector(r_u32 addr)
{
    r_u32 func = spi_flash_erase_4KB_sector_addr;
    if (rda_ccfg_hwver() >= 4) {
        func = spi_flash_erase_4KB_sector_addr_4;
    }
    ((void(*)(r_u32))func)(addr);
}

static inline void RDA5991H_ERASE_FLASH(void*addr, r_u32 len)
{
    r_u32 func = FLASH_ERASE_FUN_ADDR;
    if (rda_ccfg_hwver() >= 4) {
        func = FLASH_ERASE_FUN_ADDR_4;
    }
    ((void(*)(void*, r_u32))func)(addr, len);
}

static inline void RDA5991H_WRITE_FLASH(r_u32 addr, r_u8 *data, r_u32 len)
{
    r_u32 func = FLASH_WRITE_FUN_ADDR;
    if (rda_ccfg_hwver() >= 4) {
        func = FLASH_WRITE_FUN_ADDR_4;
    }
    ((void(*)(r_u32, r_u8 *, r_u32))func)(addr, data, len);
}

static inline void RDA5991H_READ_FLASH(r_u32 addr, r_u8 *buf, r_u32 len)
{
    r_u32 func = FLASH_READ_FUN_ADDR;
    if (rda_ccfg_hwver() >= 4) {
        func = FLASH_READ_FUN_ADDR_4;
    }
    ((void(*)(r_u32, r_u8 *, r_u32))func)(addr, buf, len);
}

static inline void SPI_FLASH_READ_DATA_FOR_MBED(void*addr, void*buf, r_u32 len)
{
    r_u32 func = SPI_FLASH_READ_DATA_FOR_MBED_ADDR;
    if (rda_ccfg_hwver() >= 4) {
        func = SPI_FLASH_READ_DATA_FOR_MBED_ADDR_4;
    }
    ((void(*)(void*, void*, r_u32))func)(buf, addr, len);
}

static inline void UART_SEND_BYTE(const unsigned char c)
{
    r_u32 func = UART_SEND_BYTE_ADDR;
    if (rda_ccfg_hwver() >= 4) {
        func = UART_SEND_BYTE_ADDR_4;
    }
    ((void(*)(const unsigned char))func)(c);
}

static inline unsigned char UART_RECV_BYTE(void)
{
    r_u32 func = UART_RECV_BYTE_ADDR;
    if (rda_ccfg_hwver() >= 4) {
        func = UART_RECV_BYTE_ADDR_4;
    }
    ((unsigned char(*)(void))func)();
}

static inline unsigned char CRC32(unsigned char* p, unsigned int len)
{
    r_u32 func = CRC32_ADDR;
    if (rda_ccfg_hwver() >= 4) {
        func = CRC32_ADDR_4;
    }
    ((unsigned int(*)(unsigned char *, unsigned int))func)(p, len);
}

#endif
