#include <stdlib.h>

#include <xenos/xenos.h>
#include <console/console.h>

#include <input/input.h>
#include <usb/usbmain.h>

#include <network/network.h>

#include <diskio/ata.h>
#include <libfat/fat.h>

int bdev_enum(int handle, const char **name);

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
#define ISD1200_READ_SPI 0xB0
#define ISD1200_READ_CFG 0xB1
#define ISD1200_SET_CFG 0xB2
#define ISD1200_CMD_FIN 0xB3

int main(void)
{
	const char * s;
	int handle;
	struct controller_data_s pad;
	struct controller_data_s prev = {0};

	xenos_init(VIDEO_MODE_AUTO);
	console_init();
	printf("Hello, world!\n");

	usb_init();
	usb_do_poll();

	xenon_ata_init();
	xenon_atapi_init();

	fatInitDefault();

	network_init();

	console_set_colors(0,-1);
	console_clrscr();

	network_print_config();

	isd_tcp_init();

	printf("\n");

	int ret = isd1200_init();
	if(!ret)
		printf("isd1200_init failed!\n");

	printf("\nSoftSonus\n\n");

	printf("A: Play power\nB: Play eject\nX: Read ISD flash to uda:/isddump.bin\nY: Write uda:/isdflash.bin to ISD flash\nStart: Start spi audio stream\nLB: Dump power PCM to uda:/power.raw\nRB: Dump eject PCM to uda:/eject.raw\nBack: exit\n\n");

	int fileops = 1;
	handle=-1;
	handle=bdev_enum(handle,&s);
	if(handle<0) {
		printf("No fat drives detected, file operations disabled\n");
		fileops = 0;
	}

	while(1) {
		network_poll();

		usb_do_poll();
		get_controller_data(&pad, 0);

		if (pad.back) return 0;

		if (pad.a && !prev.a) {
			isd1200_play_vp(5);
		}
		if (pad.b && !prev.b) {
			isd1200_play_vp(6);
		}
		if (pad.x && !prev.x) {
			if(fileops){
				uint8_t buffer[512];
				printf("Dumping flash....\n");

				FILE *fp = fopen("uda:/isddump.bin", "wb");
				if (!fp) {
					printf("Can't open uda:/isddump.bin'\n");
				} else {
					for (uint32_t i = 0; i < 0xB000 / 512; i++) {
						isd1200_flash_read(i, buffer);
						size_t written = fwrite(buffer, 1, 512, fp);
						if (written != 512) {
							printf("Written bytes not 512\n");
							break;
						}
					}
					fclose(fp);
					printf("Dumped flash to uda:/isddump.bin\n");
				}

			} else {
				printf("No drives to write to!\n");
			}
		}
		if (pad.y && !prev.y) {
			if(fileops){
				uint8_t buffer[16];
				printf("Writing flash....\n");
				FILE *fp = fopen("uda:/isdflash.bin", "rb");
				if (!fp) {
					printf("uda:/isdflash.bin not found\n");
				} else {
					fseek(fp, 0, SEEK_END);
					int size = ftell(fp);
					fseek(fp, 0, SEEK_SET);

					uint8_t first_byte;
					if (fread(&first_byte, 1, 1, fp) != 1) {
						printf("Failed to read header\n");
						goto fclose;
					}
					if (first_byte != 0xCF) {
						printf("ISD_ERR: Protection byte mismatch, may write protect flash!\n");
						goto fclose;
					}

					fseek(fp, 0, SEEK_SET);

					if (size < 0xB000) {
						printf("File too small: expected at least 0xB000 bytes, got %d\n", size);
						goto fclose;
					}

					isd1200_chip_erase();

					for (uint32_t i = 0; i < 0xB000 / 16; i++) {
						if (fread(buffer, 16, 1, fp) != 1) {
							printf("fread failed at block %u\n", i);
							goto fclose;
						}
						isd1200_flash_write(i, buffer);
					}

					printf("Write finished!\n");
fclose:
					fclose(fp);

				}
			} else {
				printf("No drives to read from!\n");
			}
		}
		if(pad.start && !prev.start) {
			isd1200_playspi();
		}

		if ((pad.rb && !prev.rb) || (pad.lb && !prev.lb)) {
			if(fileops){
				uint8_t buffer[512];
				size_t total_written = 0;

				printf("Dumping %s PCM....\n", pad.rb ? "eject" : "power");

				FILE *fp = fopen(pad.rb ? "uda:/eject.raw" : "uda:/power.raw", "wb");
				if (!fp) {
					printf("Can't open file'\n");
				} else {

					isd1200_set_cfg(0x02, 0x41);

					isd1200_play_vp(pad.rb ? 6 : 5);

					while (!(isd1200_read_interrupt_status() & INTERRUPT_STATUS_CMD_FIN))
					; //Clear vp irq

					while(!(isd1200_read_interrupt_status() & INTERRUPT_STATUS_CMD_FIN)) {
						isd1200_spi_read(buffer);
						size_t written = fwrite(buffer, 1, 512, fp);
						if (written != 512) {
							printf("Written bytes not 512\n");
							break;
						}
						total_written += written;
					}

					fclose(fp);
					isd1200_reset();
					isd1200_init();
					printf("Dumped %d bytes\n", total_written);
				}

			} else {
				printf("No drives to write to!\n");
			}
		}

		prev = pad;

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
				case ISD1200_READ_SPI: {
					uint8_t buffer[512];
					if((isd1200_read_interrupt_status() & INTERRUPT_STATUS_CMD_FIN)){
						uint8_t header = 0xFF;
						isd_tcp_send(&header, 1);
					} else {
						uint8_t header = 0x01;
						isd_tcp_send(&header, 1);
						isd1200_spi_read(buffer);
						isd_tcp_send(buffer, sizeof(buffer));
					}
					break;
				}
				case ISD1200_READ_CFG: {
					uint8_t reg;
					memcpy(&reg, &isd_tcp_recv_buf[1], sizeof(reg));
					uint8_t ret = isd1200_read_cfg(reg);
					isd_tcp_send(&ret, sizeof(ret));
					break;
				}
				case ISD1200_SET_CFG: {
					uint8_t reg;
					uint8_t val;
					memcpy(&reg, &isd_tcp_recv_buf[1], sizeof(reg));
					memcpy(&val, &isd_tcp_recv_buf[2], sizeof(val));
					isd1200_set_cfg(reg, val);
					isd_tcp_send((unsigned char[]){0}, 1);
					break;
				}
				case ISD1200_CMD_FIN: {
					while (!(isd1200_read_interrupt_status() & INTERRUPT_STATUS_CMD_FIN))
					;
					isd_tcp_send((unsigned char[]){0}, 1);
					break;
				}
			}
		isd_tcp_recv_len = 0;
		}
	}

	return 0;
}
