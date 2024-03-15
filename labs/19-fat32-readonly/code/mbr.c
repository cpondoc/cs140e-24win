#include "rpi.h"
#include "pi-sd.h"
#include "mbr.h"

mbr_t *mbr_read() {
  // Be sure to call pi_sd_init() before calling this function!

  // Read the MBR into a heap-allocated buffer.  Use `pi_sd_read` or
  // `pi_sec_read` to read 1 sector from LBA 0 into memory.
  void *data = 0;
  pi_sd_read(data, 0, 1);

  // Verify that the MBR is valid. (see mbr_check)
  mbr_check(data);

  // Return the MBR.
  return data;
}
