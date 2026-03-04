#include <xenos/xenos.h>
#include <console/console.h>

#include <network/network.h>

#include "isd_tcp.h"
#include "isd1200.h"

#define ISD1200_INIT 0xA0
#define ISD1200_DEINIT 0xA1
#define ISD1200_READ_ID 0xA2
#define ISD1200_READ_FLASH 0xA3
#define ISD1200_ERASE_FLASH 0xA4
#define ISD1200_WRITE_FLASH 0xA5
#define ISD1200_PLAY_VOICE 0xA6
#define ISD1200_EXEC_MACRO 0xA7
#define ISD1200_RESET 0xA8
#define ISD1200_PLAY_SPI 0xA9

int main(void)
{
	xenos_init(VIDEO_MODE_AUTO);
	console_init();
	printf("Hello, world!\n");

	network_init();

	network_print_config();

	isd_tcp_init();

	int byte;

	byte = isd1200_init();

	printf("%d\n", byte);

	while(1) {
		network_poll();

		if (isd_tcp_recv_len != 0){

			switch(isd_tcp_recv_buf[0]) {
				case ISD1200_INIT: {
					uint8_t ret = isd1200_init() ? 0 : 1;
					isd_tcp_send(&ret, sizeof(ret));
					break;
				}
				case ISD1200_DEINIT:
					isd1200_deinit();
					isd_tcp_send((unsigned char[]){0}, 1);
					break;
				case ISD1200_READ_ID: {
					uint8_t dev_id = isd1200_read_id();
					isd_tcp_send(&dev_id, sizeof(dev_id));
					break;
				}
				case ISD1200_READ_FLASH: {
					uint8_t buffer[512];
					uint32_t page;
					memcpy(&page, &isd_tcp_recv_buf[1], sizeof(page));
					isd1200_flash_read(page, buffer);
					isd_tcp_send(buffer, sizeof(buffer));
					break;
				}
				case ISD1200_ERASE_FLASH:
					isd1200_chip_erase();
					isd_tcp_send((unsigned char[]){0}, 1);
					break;
				case ISD1200_WRITE_FLASH: {
					uint32_t page;

					if (isd_tcp_recv_len != (16 + 5)) {
						printf("Received bytes length not 21 %d\n", isd_tcp_recv_len);
						isd_tcp_send((unsigned char[]){1}, 1);
						break;
					}

					memcpy(&page, &isd_tcp_recv_buf[1], sizeof(page));

					if (page == 0 && isd_tcp_recv_buf[5] != 0xCF) {
						printf("ISD_ERR: Protection byte mismatch, may write protect flash!\n");
						isd_tcp_send((unsigned char[]){1}, 1);
						break;
					}

					isd1200_flash_write(page, &isd_tcp_recv_buf[5]);

					isd_tcp_send((unsigned char[]){0}, 1);
					break;
				}
				case ISD1200_PLAY_VOICE: {
					uint16_t index;
					memcpy(&index, &isd_tcp_recv_buf[1], sizeof(index));
					isd1200_play_vp(index);
					isd_tcp_send((unsigned char[]){0}, 1);
					break;
				}
				case ISD1200_EXEC_MACRO: {
					uint16_t index;
					memcpy(&index, &isd_tcp_recv_buf[1], sizeof(index));
					isd1200_exe_vm(index);
					isd_tcp_send((unsigned char[]){0}, 1);
					break;
				}
				case ISD1200_RESET:
					isd1200_reset();
					isd_tcp_send((unsigned char[]){0}, 1);
					break;
				case ISD1200_PLAY_SPI: {
					isd1200_playspi();
					//isd_tcp_send((unsigned char[]){0}, 1);
					break;
				}
			}
		isd_tcp_recv_len = 0;
		}
	}

	return 0;
}
