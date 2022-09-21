
#ifndef __XRAY_H__
#define __XRAY_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
bool xray_mem_read(uint16_t addr, uint8_t* byte);
bool xray_mem_write(uint16_t addr, uint8_t byte);
#ifdef __cplusplus
}
#endif

#endif
