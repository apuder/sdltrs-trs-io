
#pragma once

#include <stdint.h>

uint8_t spi_get_cookie();
uint8_t spi_get_fpga_version();
uint8_t spi_get_printer_byte();
void spi_bram_poke(uint16_t addr, uint8_t data);
uint8_t spi_bram_peek(uint16_t addr);
void spi_xram_poke_code(uint8_t addr, uint8_t data);
void spi_xram_poke_data(uint8_t addr, uint8_t data);
uint8_t spi_xram_peek_data(uint8_t addr);
uint8_t spi_dbus_read();
void spi_dbus_write(uint8_t d);
uint8_t spi_abus_read();
void spi_set_breakpoint(uint8_t n, uint16_t addr);
void spi_clear_breakpoint(uint8_t n);
void spi_xray_resume();
void spi_trs_io_done();
void spi_set_full_addr(bool flag);
void spi_set_screen_color(uint8_t color);

void init_spi();

