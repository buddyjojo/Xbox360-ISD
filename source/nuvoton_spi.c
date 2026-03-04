/*
 * Copyright (c) 2022 Balázs Triszka <balika011@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <xenon_smc/xenon_gpio.h>
#include <pci/io.h>

#define SMC_BASE    0xea001000

#define GPIO_PINDR  (SMC_BASE + 0x20)
#define GPIO_HIGH   (SMC_BASE + 0x34)
#define GPIO_LOW    (SMC_BASE + 0x38)
#define GPIO_READ   (SMC_BASE + 0x40)

#define GPIO_CLK   (1 << 6)
#define GPIO_MOSI  (1 << 7)
#define GPIO_BSBY  (1 << 11)
#define GPIO_SSB   (1 << 14)
#define GPIO_MISO  (1 << 15)

void nuvoton_spi_init()
{
	write32(GPIO_HIGH, GPIO_CLK | GPIO_MOSI | GPIO_SSB); //Set inital pin state before direction change
	xenon_gpio_set_oe(GPIO_BSBY | GPIO_MISO, GPIO_CLK | GPIO_MOSI | GPIO_SSB); //Set pins 11 and 15 to input, 6, 7 and 14 to output
}

void nuvoton_spi_transfer(uint8_t *buffer, uint32_t length)
{
	write32(GPIO_LOW, GPIO_SSB);
	for (; length; length--, buffer++) {
		uint8_t tx = *buffer;
		uint8_t rx = 0;
		while (!(read32(GPIO_READ) & GPIO_BSBY));
		for (uint8_t i = 0; i < 8; i++) {
			write32(GPIO_LOW, GPIO_CLK);
			write32((tx & 0x80) ? GPIO_HIGH : GPIO_LOW, GPIO_MOSI);
			tx <<= 1;
			write32(GPIO_HIGH, GPIO_CLK);
			rx <<= 1;
			if (read32(GPIO_READ) & GPIO_MISO)
				rx |= 1;
		}
		*buffer = rx;
	}
	write32(GPIO_HIGH, GPIO_SSB);
}
