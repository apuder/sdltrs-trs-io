
#pragma once

#include <stdint.h>

void spi_xram_poke_code(uint8_t addr, uint8_t data);
void spi_xram_poke_data(uint8_t addr, uint8_t data);
uint8_t spi_xram_peek_data(uint8_t addr);
void spi_set_breakpoint(uint8_t n, uint16_t addr);
void spi_clear_breakpoint(uint8_t n);

