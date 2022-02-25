
#include "trs-io.h"
#include "trsio-wrapper.h"
#include <stdio.h>

extern "C" {

bool trsio_z80_out(uint8_t byte) {
  return TrsIO::outZ80(byte);
}

uint8_t trsio_z80_in() {
  return TrsIO::inZ80();
}

void trsio_process_in_background() {
  TrsIO::processInBackground();
}

}


