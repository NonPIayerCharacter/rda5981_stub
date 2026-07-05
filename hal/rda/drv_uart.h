/*
 * File      : drv_uart.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2013, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 *
 */

#ifndef DRV_UART_H
#define DRV_UART_H

#include "stdint.h"

int uart_putc(char c);
int8_t uart_getc(char* ch, uint64_t timeout);
void uart_fifo_reset(void);
void uart_set_baud(uint32_t baud);

#endif /* end of include guard: DRV_UART_H */
