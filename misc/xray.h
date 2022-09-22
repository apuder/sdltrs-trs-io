
#ifndef __XRAY_H__
#define __XRAY_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint16_t af;
  uint16_t bc;
  uint16_t de;
  uint16_t hl;
  uint16_t af_p;
  uint16_t bc_p;
  uint16_t de_p;
  uint16_t hl_p;
  uint16_t ix;
  uint16_t iy;
  uint16_t pc;
  uint16_t sp;
} XRAY_Z80_REGS;


#ifdef __cplusplus
extern "C" {
#endif
bool xray_mem_read(uint16_t addr, uint8_t* byte);
bool xray_mem_write(uint16_t addr, uint8_t byte);
#ifdef __cplusplus
}
#endif

#endif
