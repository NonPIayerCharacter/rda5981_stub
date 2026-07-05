#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <board.h>
//#include <core_cm4.h>
#include <drv_uart.h>
#include <rda_api.h>
#include <rda5981_bootrom_api.h>
#include "../../OpenBK7231T_App/libraries/miniz/miniz.h"

mz_uint16 m_next[TDEFL_LZ_DICT_SIZE];

#define UART_BASE   ((volatile uint32_t *)0x40012000U)
#define FLASH_CTL_REG_BASE              0x17fff000
#define FLASH_CTL_TX_CMD_ADDR_REG       (FLASH_CTL_REG_BASE + 0x00)
#define FLASH_CTL_TX_BLOCK_SIZE_REG     (FLASH_CTL_REG_BASE + 0x04)
#define FLASH_CTL_RX_FIFO_DATA_REG      (FLASH_CTL_REG_BASE + 0x10)

#define WRITE_REG32(REG, VAL) ((*(volatile unsigned int*)(REG)) = (unsigned int)(VAL))
#define WRITE_REG8(REG, VAL) ((*(volatile unsigned char*)(REG)) = (unsigned char)(VAL))
#define MREAD_WORD(addr)      *((volatile unsigned int *)(addr))

#define MAGIC 						0xA5
#define ACK_MAGIC 					0x5A 

#define STATE_ERR					0xFF
#define STATE_SYN					0x00
#define STATE_RAM_DOWNLOAD			0x01
#define STATE_FLASH_DOWNLOAD		0x02
#define STATE_FLASH_UPLOAD			0x03
#define STATE_FLASH_ERASE			0x04
#define STATE_FLASH_CHIPERASE		0x05
#define STATE_RUN					0x06
#define STATE_BOUND					0x07
#define STATE_MAX					0x08

#define STATUS_SUCCESS				0x00
#define STATUS_ERROR				0x01
#define STATUS_ADDR_ERROR			0x02
#define STATUS_TYPE_ERROR			0x03
#define STATUS_LEN_ERROR			0x04
#define STATUS_CRC_ERROR			0x05

#define RESPONSE_FAIL				0xFF
#define RESPONSE_OK					0x00
#define RESPONSE_SYNC_BOOTROM		0x01
#define RESPONSE_SYNC_SBL			0x02

#define ACK_OK  0x00
#define ACK_ERR 0x01

#define MSG_OK 0x00
#define MSG_ERR -1

#define SYNC_REQUEST_VALUE			0x73796E63
#define SYNC_REQUEST_SIZE			0x04
#define SYNC_REQUEST_TIMEOUT		120

#define HEAD_SIZE					4
#define CFG_SIZE					8
#define ACK_SIZE					6
//#define CMD_DATA_MAX_LEN 			(4 + 1 + 1024*64 + 2)
#define CMD_DATA_MAX_LEN 			(1024*16)
#define RESPONSE_SIZE				0x01
#define LOAD_MAX_SIZE_BIG			0x10000

struct sburner_cmd
{
	unsigned int msg_type;
	unsigned int arg0;
	unsigned int arg1;
};

struct message_rec_head
{
	unsigned char magic;
	unsigned char type;
	unsigned short data_len;
	unsigned int run_addr;
	unsigned char CRC8;
};

#define MESSAGE_REC_SIZE sizeof(struct message_rec_head)

struct message_ack_head
{
	unsigned char magic;
	unsigned char type;
	unsigned short data_len;
	unsigned char status;
	unsigned char CRC8;
};
#define MESSAGE_ACK_SIZE sizeof(struct message_ack_head)

struct load_cfg_msg
{
	unsigned int	addr;
	unsigned int	len;
} load_cfg_msg_t;
#define MESSAGE_LOAD_SIZE sizeof(struct load_cfg_msg)

#pragma pack(1)
typedef struct message_head
{
	uint8_t sof;
	uint8_t type;
	uint32_t data_len;
	uint8_t sub_type;
	uint32_t check_sum;
} message_head_t;
#pragma pack()

struct download_cfg_msg
{
	int msg_type;
	int addr;
	int len;
};

struct download_t
{
	int download_state;
	int download_addr;
	int download_len;
	int download_timeout;
	char* downloader_buf;
};

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define CRC_MODE 'C'
#define XMODEM_BLOCK_SIZE_1K 1024
#define XMODEM_BLOCK_SIZE_128 128

uint32_t g_flash_id = 0;
unsigned int g_flash_size = 0x1000000;

struct message_ack_head ACK_msg =
{
	.magic = ACK_MAGIC,
	.type = 0,
	.data_len = 0,
	.status = STATUS_SUCCESS,
	.CRC8 = 0
};

ALIGN(4)
unsigned char cmd_data_buf[CMD_DATA_MAX_LEN] = { 0 };

static inline volatile uint32_t* uart_stat(void)
{
	return UART_BASE + (0x20 >> 2);
}

void uart_write(uint8_t* buf, uint32_t len)
{
	if(!buf || !len)
	{
		return;
	}

	for(size_t i = 0; i < len; ++i)
	{
		uart_putc(buf[i]);
	}
}

unsigned char uboot_mesage_check(unsigned char* buf, unsigned short length)
{
	unsigned int crc = 0;
	unsigned char ret = 0;
	unsigned int i = 0;

	for(i = 0; i < length; i++)
	{
		crc += buf[i];
	}
	ret = crc % 256;
	return ret;
}

void sburner_flash_init(void)
{
	WRITE_REG32(FLASH_CTL_TX_BLOCK_SIZE_REG, 3 << 8);
	uint32_t status3 = MREAD_WORD(FLASH_CTL_RX_FIFO_DATA_REG);
	uint32_t status4 = MREAD_WORD(FLASH_CTL_RX_FIFO_DATA_REG);
	uint32_t status5 = MREAD_WORD(FLASH_CTL_RX_FIFO_DATA_REG);
	wait_busy_down();
	spi_wip_reset();
	WRITE_REG32(FLASH_CTL_TX_CMD_ADDR_REG, 0x9F);
	wait_busy_down();
	status3 = MREAD_WORD(FLASH_CTL_RX_FIFO_DATA_REG);
	status4 = MREAD_WORD(FLASH_CTL_RX_FIFO_DATA_REG);
	status5 = MREAD_WORD(FLASH_CTL_RX_FIFO_DATA_REG);
	g_flash_id = ((status5 & 0xFF) << 16) | ((status4 & 0xFF) << 8) | (status3 & 0xFF);
	unsigned char id = status5 & 0xff;
	switch(id)
	{
		default:
		case 0x14:
			g_flash_size = 0x100000; break;
		case 0x15:
			g_flash_size = 0x200000; break;
		case 0x16:
			g_flash_size = 0x400000; break;
		case 0x17:
			g_flash_size = 0x800000; break;
		case 0x18:
			g_flash_size = 0x1000000; break;
		case 0x19:
			g_flash_size = 0x2000000; break;
		case 0x20:
			g_flash_size = 0x4000000; break;
	}
}

static uint16_t crc16_ccitt(const uint8_t* data, uint16_t len)
{
	uint16_t crc = 0;
	while(len--)
	{
		crc ^= (uint16_t)(*data++) << 8;
		for(uint8_t i = 0; i < 8; i++)
			crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
	}
	return crc;
}

void uboot_sync(void)
{
	unsigned int i = 0;
	struct message_rec_head* msg = (struct message_rec_head*)cmd_data_buf;

	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.status = STATUS_SUCCESS;

	//SYNC
	for(i = 0; i < SYNC_REQUEST_SIZE; i++)
	{
		if(cmd_data_buf[HEAD_SIZE + i] != (char)((SYNC_REQUEST_VALUE >> (i * 8)) & 0xff))
		{
			ACK_msg.status = STATUS_ERROR;
			ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
			uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
			return;	//eroor, again
		}
	}
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
}

int main(void)
{
	board_init();
	void rda_ccfg_ck(void);
	rda_ccfg_ck();
	RDA_WDT->WDTCFG &= ~(0x01UL << WDT_EN_BIT);
	uart_set_baud(115200);
	sburner_flash_init();
	while(1) uart_cmd_parser();
	return 0;
}

void uboot_flashid(void)
{
	struct message_rec_head* msg = (struct message_rec_head*)cmd_data_buf;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
	ACK_msg.data_len = 4;
	memcpy(cmd_data_buf, &ACK_msg, HEAD_SIZE);
	memcpy(&cmd_data_buf[HEAD_SIZE], &g_flash_id, 4);
	cmd_data_buf[HEAD_SIZE + 4] = STATUS_SUCCESS;
	cmd_data_buf[HEAD_SIZE + 4 + 1] = uboot_mesage_check((unsigned char*)cmd_data_buf, HEAD_SIZE + 4 + 1);
	uart_write((unsigned char*)cmd_data_buf, HEAD_SIZE + 4 + 2);
}

void uboot_buad(void)
{
	struct message_rec_head* msg = (struct message_rec_head*)cmd_data_buf;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
	uart_set_baud(msg->run_addr);
	for(int i = 0; i < 1000000; ++i);
	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
}

void uboot_flash_crc32(void* buf)
{
	struct load_cfg_msg cfg_msg;
	struct message_rec_head* msg = (struct message_rec_head*)buf;

	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 32;

	memcpy(&cfg_msg, &(cmd_data_buf[HEAD_SIZE]), CFG_SIZE);

	if((cfg_msg.addr + cfg_msg.len) > g_flash_size)
	{
		ACK_msg.data_len = 0;
		ACK_msg.status = STATUS_ADDR_ERROR;
		ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
		uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
		return;
	}
	uint32_t crc = CRC32((unsigned char*)0x18000000UL + cfg_msg.addr, cfg_msg.len);
	memcpy(cmd_data_buf, &ACK_msg, HEAD_SIZE);
	memcpy(&cmd_data_buf[HEAD_SIZE], &crc, 4);
	cmd_data_buf[HEAD_SIZE + 4] = STATUS_SUCCESS;
	cmd_data_buf[HEAD_SIZE + 4 + 1] = uboot_mesage_check((unsigned char*)cmd_data_buf, HEAD_SIZE + 4 + 1);
	uart_write((unsigned char*)cmd_data_buf, HEAD_SIZE + 4 + 2);
}

void uboot_flash_xmodem_ul(void* buf)
{
	uint8_t block_num = 1;
	uint8_t resp = 0;
	uint32_t offset = 0;
	int retry;
	int ret;
	bool use_1k = true;
	bool use_crc = true;

	uint8_t* packet = cmd_data_buf;
	//uint8_t packet[3 + XMODEM_BLOCK_SIZE_1K + 2];
	struct load_cfg_msg cfg_msg;
	struct message_rec_head* msg = (struct message_rec_head*)buf;

	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);

	memcpy(&cfg_msg, &(cmd_data_buf[HEAD_SIZE]), CFG_SIZE);
	uint32_t data_len = cfg_msg.len;

	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);

	int timeout = 10000;
	while(timeout > 0)
	{
		if(uart_getc(&resp, 100) == 0)
		{
			if(resp == CRC_MODE)
			{
				use_crc = true;
				use_1k = true;
				break;
			}
			else if(resp == NAK)
			{
				use_crc = false;
				use_1k = false;
				break;
			}
			else if(resp == CAN)
			{
				return;
			}
		}

		timeout -= 100;
	}

	if(timeout <= 0)
	{
		uart_putc(CAN);
		uart_putc(CAN);
		return;
	}

	while(data_len > 0)
	{
		uint32_t block_size;
		uint8_t header;

		if(use_1k)
		{
			block_size = XMODEM_BLOCK_SIZE_1K;
			header = STX;
		}
		else
		{
			block_size = 128;
			header = SOH;
		}

		uint32_t chunk = (data_len >= block_size) ? block_size : data_len;

		retry = 0;
		while(retry < 10)
		{
			memset(packet, 0xFF, sizeof(packet));

			packet[0] = header;
			packet[1] = block_num;
			packet[2] = ~block_num;

			RDA5991H_READ_FLASH(cfg_msg.addr + offset, &packet[3], chunk);

			if(chunk < block_size)
			{
				memset(&packet[3 + chunk], 0xFF, block_size - chunk);
			}

			uint32_t pkt_len = 3 + block_size;
			if(use_crc)
			{
				uint16_t crc = crc16_ccitt(&packet[3], block_size);
				packet[pkt_len++] = (crc >> 8) & 0xff;
				packet[pkt_len++] = crc & 0xff;
			}
			else
			{
				uint8_t sum = 0;
				for(uint32_t i = 0; i < block_size; ++i)
				{
					sum += packet[3 + i];
				}
				packet[pkt_len++] = sum;
			}

			uart_write(packet, pkt_len);

			ret = uart_getc(&resp, 5000);
			if(ret == 0 && resp == ACK)
			{
				break;
			}

			if(ret == 0 && resp == CAN)
			{
				if(uart_getc(&resp, 1000) == 0 && resp == CAN)
				{
					return;
				}
			}

			++retry;
		}

		if(use_1k && retry >= 7)
		{
			use_1k = false;
		}

		if(retry >= 10)
		{
			uart_putc(CAN);
			uart_putc(CAN);
			uart_set_baud(115200);
			return;
		}

		offset += chunk;
		data_len -= chunk;
		++block_num;
	}

	retry = 0;
	while(retry < 10)
	{
		uart_putc(EOT);

		ret = uart_getc(&resp, 5000);
		if(ret == 0 && resp == ACK)
		{
			return;
		}
		++retry;
	}

	uart_putc(CAN);
	uart_putc(CAN);
}

int mz_deflateInit3(mz_streamp pStream, int level, int method, int window_bits, int mem_level, int strategy)
{
	tdefl_compressor* pComp;
	mz_uint comp_flags = tdefl_create_comp_flags_from_zip_params(level, window_bits, strategy);

	if(!pStream)
		return MZ_STREAM_ERROR;
	if((method != MZ_DEFLATED) || ((mem_level < 1) || (mem_level > 9)) || ((window_bits != MZ_DEFAULT_WINDOW_BITS) && (-window_bits != MZ_DEFAULT_WINDOW_BITS)))
		return MZ_PARAM_ERROR;

	pStream->data_type = 0;
	pStream->adler = 0;
	pStream->msg = NULL;
	pStream->reserved = 0;
	pStream->total_in = 0;
	pStream->total_out = 0;
	if(!pStream->zalloc)
		pStream->zalloc = miniz_def_alloc_func;
	if(!pStream->zfree)
		pStream->zfree = miniz_def_free_func;

	pComp = (tdefl_compressor*)pStream->zalloc(pStream->opaque, 1, sizeof(tdefl_compressor));
	pComp->m_next = m_next;
	if(!pComp)
		return MZ_MEM_ERROR;

	pStream->state = (struct mz_internal_state*)pComp;

	if(tdefl_init(pComp, NULL, NULL, comp_flags) != TDEFL_STATUS_OKAY)
	{
		mz_deflateEnd(pStream);
		return MZ_PARAM_ERROR;
	}

	return MZ_OK;
}

void uboot_flash_xmodem_ul_z(void* buf)
{
	uint8_t block_num = 1;
	uint8_t resp = 0;
	int retry;
	int ret;
	bool use_1k = true;
	bool use_crc = true;

	//uint8_t packet[3 + XMODEM_BLOCK_SIZE_1K + 2];
	uint8_t* packet = cmd_data_buf;

	struct load_cfg_msg cfg_msg;
	struct message_rec_head* msg = (struct message_rec_head*)buf;

	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);

	memcpy(&cfg_msg, &(cmd_data_buf[HEAD_SIZE]), CFG_SIZE);

	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);

	int timeout = 10000;

	while(timeout > 0)
	{
		if(uart_getc(&resp, 1000) == 0)
		{
			if(resp == CRC_MODE)
			{
				use_crc = true;
				use_1k = true;
				break;
			}

			if(resp == NAK)
			{
				use_crc = false;
				use_1k = false;
				break;
			}

			if(resp == CAN) return;
		}

		timeout -= 1000;
	}

	if(timeout <= 0)
	{
		uart_putc(CAN);
		uart_putc(CAN);
		return;
	}

	z_stream stream;

	memset(&stream, 0, sizeof(stream));

	const uint8_t* src = (const uint8_t*)(0x18000000 + cfg_msg.addr);

	uint32_t remaining = cfg_msg.len;

	if(mz_deflateInit3(&stream, 5, MZ_DEFLATED, -MZ_DEFAULT_WINDOW_BITS, 9, MZ_DEFAULT_STRATEGY) != Z_OK)
	{
		return;
	}

	bool finished = false;

	while(!finished)
	{
		uint32_t block_size;
		uint8_t header;

		if(use_1k)
		{
			block_size = XMODEM_BLOCK_SIZE_1K;
			header = STX;
		}
		else
		{
			block_size = 128;
			header = SOH;
		}

		memset(packet, 0xFF, sizeof(packet));

		packet[0] = header;
		packet[1] = block_num;
		packet[2] = ~block_num;

		stream.next_out = &packet[3];
		stream.avail_out = block_size;

		while(stream.avail_out)
		{
			if(stream.avail_in == 0 && remaining)
			{
				uint32_t n = remaining;

				if(n > block_size) n = block_size;

				stream.next_in = (unsigned char*)(src + (cfg_msg.len - remaining));

				stream.avail_in = n;

				remaining -= n;
			}

			ret = deflate(&stream, remaining ? Z_NO_FLUSH : Z_FINISH);

			if(ret == Z_STREAM_END)
			{
				finished = true;
				break;
			}

			if(ret != Z_OK)
			{
				deflateEnd(&stream);
				return;
			}
		}

		uint32_t pkt_len = 3 + block_size;

		if(use_crc)
		{
			uint16_t crc = crc16_ccitt(&packet[3], block_size);

			packet[pkt_len++] = crc >> 8;
			packet[pkt_len++] = crc & 0xff;
		}
		else
		{
			uint8_t sum = 0;

			for(uint32_t i = 0; i < block_size; i++)
			{
				sum += packet[3 + i];
			}

			packet[pkt_len++] = sum;
		}

		retry = 0;

		while(retry < 10)
		{
			uart_write(packet, pkt_len);

			ret = uart_getc(&resp, 5000);

			if(ret == 0 && resp == ACK)
			{
				break;
			}

			retry++;
		}

		//if(use_1k && retry >= 7)
		//{
		//	use_1k = false;
		//}

		if(retry >= 10)
		{
			deflateEnd(&stream);

			uart_putc(CAN);
			uart_putc(CAN);

			return;
		}

		block_num++;
	}

	deflateEnd(&stream);

	retry = 0;

	while(retry < 10)
	{
		uart_putc(EOT);

		ret = uart_getc(&resp, 5000);

		if(ret == 0 && resp == ACK)
		{
			return;
		}

		retry++;
	}

	uart_putc(CAN);
	uart_putc(CAN);
}

void uboot_flash_erase_handle(void* buf)
{
	struct load_cfg_msg cfg_msg;
	struct message_rec_head* msg = (struct message_rec_head*)buf;

	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.status = STATUS_SUCCESS;

	memcpy(&cfg_msg, &(cmd_data_buf[HEAD_SIZE]), CFG_SIZE);

	if((cfg_msg.addr + cfg_msg.len) > g_flash_size)
	{
		ACK_msg.status = STATUS_ADDR_ERROR;
		ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
		uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
		return;
	}

	rda5981_spi_erase_partition(cfg_msg.addr, cfg_msg.len);
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
}

void uboot_flash_chiperase_handle(void* buf)
{
	struct message_rec_head* msg = (struct message_rec_head*)buf;

	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.status = STATUS_SUCCESS;

	rda5981_spi_erase_partition(0x1000, g_flash_size - 0x1000);
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
}

void uboot_flash_xmodem_dl(void* buf)
{
	uint8_t header[3] = { 0x00 };
	uint8_t* data = cmd_data_buf;
	uint8_t crc_bytes[2] = { 0x00 };
	uint16_t crc_calc, crc_recv;
	uint32_t offset = 0;
	struct load_cfg_msg cfg_msg;
	struct message_rec_head* msg = (struct message_rec_head*)buf;
	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);

	memcpy(&cfg_msg, &(cmd_data_buf[HEAD_SIZE]), CFG_SIZE);

	if((cfg_msg.addr + cfg_msg.len) > g_flash_size)
	{
		ACK_msg.status = STATUS_ADDR_ERROR;
		ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
		uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
		return;
	}

	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);

	rda5981_spi_erase_partition(cfg_msg.addr, cfg_msg.len);

	uart_putc(CRC_MODE);

	for(;;)
	{
		// header
		if(uart_getc(&header[0], 1000) != 0)
		{
			uart_putc(CRC_MODE);
			continue;
		}

		if(header[0] == EOT)
		{
			uart_putc(ACK);
			break;
		}

#define NAKCONTINUE { uart_putc(NAK); continue; }

		if(header[0] != STX) NAKCONTINUE

		if(uart_getc(&header[1], 2000) == -1) NAKCONTINUE
		if(uart_getc(&header[2], 2000) == -1) NAKCONTINUE

		if((header[1] + header[2]) != 0xFF) NAKCONTINUE

		// recv data + crc
		for(int i = 0; i < XMODEM_BLOCK_SIZE_1K; i++)
			if(uart_getc(&data[i], 2000) == -1) NAKCONTINUE
		if(uart_getc(&crc_bytes[0], 2000) == -1) NAKCONTINUE
		if(uart_getc(&crc_bytes[1], 2000) == -1) NAKCONTINUE

		crc_recv = ((uint16_t)crc_bytes[0] << 8) | crc_bytes[1];
		crc_calc = crc16_ccitt(data, XMODEM_BLOCK_SIZE_1K);

		if(crc_recv != crc_calc) NAKCONTINUE

		RDA5991H_WRITE_FLASH(cfg_msg.addr + offset, (char *)data, XMODEM_BLOCK_SIZE_1K);
		offset += XMODEM_BLOCK_SIZE_1K;

		uart_putc(ACK);
	}
}

void uboot_flash_xmodem_dl_z(void* buf)
{
	uint8_t header[3] = { 0x00 };
	uint8_t data[XMODEM_BLOCK_SIZE_1K] = { 0xFF };
	uint8_t crc_bytes[2] = { 0x00 };
	uint16_t crc_calc, crc_recv;
	uint32_t flash_offset = 0;
	struct load_cfg_msg cfg_msg;
	struct message_rec_head* msg = (struct message_rec_head*)buf;

	ACK_msg.status = STATUS_SUCCESS;
	ACK_msg.magic = ACK_MAGIC;
	ACK_msg.type = msg->type;
	ACK_msg.data_len = 0x0000;
	ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);

	memcpy(&cfg_msg, &(cmd_data_buf[HEAD_SIZE]), CFG_SIZE);

	if((cfg_msg.addr + cfg_msg.len) > g_flash_size)
	{
		ACK_msg.status = STATUS_ADDR_ERROR;
		ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
		uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
		return;
	}

	uart_write((unsigned char*)&ACK_msg, ACK_SIZE);

	rda5981_spi_erase_partition(cfg_msg.addr, cfg_msg.len);

	mz_stream stream;
	memset(&stream, 0, sizeof(stream));
	if(mz_inflateInit2(&stream, -MZ_DEFAULT_WINDOW_BITS) != MZ_OK)
	{
		ACK_msg.status = STATUS_ERROR;
		uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
		return;
	}

	uart_putc(CRC_MODE);

	flash_offset = cfg_msg.addr;

	for(;;)
	{
		uint32_t data_size = 0;

		if(uart_getc(&header[0], 3333) != 0)
		{
			uart_putc(CRC_MODE);
			continue;
		}

		if(header[0] == EOT)
		{
			stream.next_in = NULL;
			stream.avail_in = 0;

			while(1)
			{
				stream.next_out = cmd_data_buf;
				stream.avail_out = sizeof(cmd_data_buf);

				int status = mz_inflate(&stream, MZ_NO_FLUSH);
				uint32_t produced = sizeof(cmd_data_buf) - stream.avail_out;

				if(produced > 0)
				{
					if((flash_offset + produced) > (cfg_msg.addr + cfg_msg.len))
					{
						goto abort_decompression;
					}
					HAL_FlashWrite(cmd_data_buf, produced, flash_offset);
					flash_offset += produced;
				}

				if(status == MZ_STREAM_END) break;

				if(status != MZ_OK && status != MZ_BUF_ERROR)
					goto abort_decompression;
			}

			mz_inflateEnd(&stream);

			uart_putc(ACK);
			return;
		}

		if(header[0] != STX && header[0] != SOH)
		{
			uart_putc(NAK);
			continue;
		}

		data_size = (header[0] == STX) ? XMODEM_BLOCK_SIZE_1K : XMODEM_BLOCK_SIZE_128;

		uart_getc(&header[1], 10000);
		uart_getc(&header[2], 10000);

		if((header[1] + header[2]) != 0xFF)
		{
			uart_putc(NAK);
			continue;
		}

		for(uint32_t i = 0; i < data_size; i++)
			uart_getc(&data[i], 20000);

		uart_getc(&crc_bytes[0], 10000);
		uart_getc(&crc_bytes[1], 10000);

		crc_recv = ((uint16_t)crc_bytes[0] << 8) | crc_bytes[1];
		crc_calc = crc16_ccitt(data, data_size);

		if(crc_recv != crc_calc)
		{
			uart_putc(NAK);
			continue;
		}

		stream.next_in = data;
		stream.avail_in = data_size;

		while(stream.avail_in > 0)
		{
			stream.next_out = cmd_data_buf;
			stream.avail_out = sizeof(cmd_data_buf);

			int status = mz_inflate(&stream, MZ_NO_FLUSH);
			uint32_t produced = sizeof(cmd_data_buf) - stream.avail_out;

			if(produced > 0)
			{
				if((flash_offset + produced) > (cfg_msg.addr + cfg_msg.len))
				{
					goto abort_decompression;
				}

				HAL_FlashWrite(cmd_data_buf, produced, flash_offset);
				flash_offset += produced;
			}

			if(status == MZ_STREAM_END) break;

			if(status != MZ_OK && status != MZ_BUF_ERROR)
				goto abort_decompression;
		}

		uart_putc(ACK);
	}

abort_decompression:
	mz_inflateEnd(&stream);

	uart_putc(CAN);
	uart_putc(CAN);
}

int uart_cmd_parser(void)
{
	unsigned int i = 0;
	unsigned char resp = 0;
	unsigned char CRC8 = 0;
	unsigned char buf[HEAD_SIZE] = { 0 };
	struct message_rec_head rec_head;

	unsigned char cfgbuf[CFG_SIZE] = { 0 };

	uart_fifo_reset();

	do
	{
		memset(buf, 0, HEAD_SIZE);
		for(i = 0; i < HEAD_SIZE; ++i)
		{
			volatile uint32_t* stat = uart_stat();
			while(!((*stat >> 20U) & 1U));
			resp = uart_getc((unsigned char*)&buf[i], 0xFFFFFFFF);
			if(resp < 0)
			{
				resp = MSG_ERR;
				return resp;
			}
			if((buf[0]) != MAGIC)
			{
				i = 0xFFFFFFFF;
			}
		}

		//MEMSET(cmd_data_buf,0,CMD_DATA_MAX_LEN);
		memcpy(cmd_data_buf, buf, HEAD_SIZE);  // last buf, save 4 bytes head data
		memcpy(&rec_head, buf, HEAD_SIZE);  // data head, save 4 bytes to rec_head, and parser

		// check HEAD STATE
		if(buf[1] <= 0x9F)
		{
			for(i = 0; i < rec_head.data_len; i++)  // parser buf 4 bytes data, can get data_len.
			{
				// receive data_len data to cfgbuf
				resp = uart_getc((unsigned char*)&cfgbuf[i], 0xFFFFFFFF);
				if(resp < 0)
				{
					resp = MSG_ERR;
					return resp;
				}
			}
			memcpy(&(cmd_data_buf[HEAD_SIZE]), cfgbuf, rec_head.data_len);  // last buf, save 8 bytes data

			// CRC8
			resp = uart_getc((unsigned char*)&cmd_data_buf[HEAD_SIZE + rec_head.data_len], 0xFFFFFFFF);
			if(resp < 0)
			{
				resp = MSG_ERR;
				return resp;
			}
			CRC8 = uboot_mesage_check((unsigned char*)cmd_data_buf, HEAD_SIZE + rec_head.data_len);
			if(cmd_data_buf[HEAD_SIZE + rec_head.data_len] != CRC8)
			{
				rec_head.type = STATE_ERR;  // crc8 error
			}
		}
		else
		{
			rec_head.type = STATE_ERR;
		}


		switch(rec_head.type)
		{
			case STATE_SYN:
				uboot_sync();
				break;
			case STATE_FLASH_ERASE:  // 0x04
				uboot_flash_erase_handle(&cmd_data_buf);
				break;
			case STATE_FLASH_CHIPERASE:  // 0x05
				uboot_flash_chiperase_handle(&cmd_data_buf);
				break;
			case STATE_BOUND:  // 0x07
				uboot_buad();
				break;
				//case 0x09:
				//	uboot_flash_sha256(&cmd_data_buf);
				//	break;
			case 0x8F:
				uboot_flash_crc32(&cmd_data_buf);
				break;
			case 0x90:
				uboot_flashid();
				break;
			case 0x91:
				uboot_flash_xmodem_dl(&cmd_data_buf);
				break;
			case 0x92:
				uboot_flash_xmodem_ul(&cmd_data_buf);
				break;
			case 0x96:
				uboot_flash_xmodem_ul_z(&cmd_data_buf);
				break;
			case 0x97:
				uboot_flash_xmodem_dl_z(&cmd_data_buf);
				break;

			default:
				ACK_msg.type = STATE_ERR;
				ACK_msg.status = STATUS_TYPE_ERROR;
				ACK_msg.CRC8 = uboot_mesage_check((unsigned char*)&ACK_msg, ACK_SIZE - 1);
				uart_write((unsigned char*)&ACK_msg, ACK_SIZE);
				break;
		}
	} while(1);
}

void* $Sub$$malloc(size_t size)
{
	return (void*)(0x40101000);
}

#if defined (__GNUC__)
__attribute__((used)) void* __wrap_malloc(size_t size)
{
	return (void*)(0x40101000);
}
__attribute__((used)) void* memset(void* dest, int c, size_t n)
{
	unsigned char* p = dest;
	while(n--)
	{
		*p++ = (unsigned char)c;
	}
	return dest;
}
__attribute__((used)) void* memcpy(void* dest, const void* src, size_t n)
{
	unsigned char* d = (unsigned char*)dest;
	const unsigned char* s = (const unsigned char*)src;

	while(n--)
	{
		*d++ = *s++;
	}

	return dest;
}
void __assert_func(const char* file, int line, const char* func, const char* expr) {}
void free(void* ptr) {}

void* _sbrk(int incr)
{
	return NULL;
}
#endif