#include "interface/console.h"

#include "tock_sys.h"

// XXX Suppress missing prototype warnings for this file as the headers should
// be in newlib internals, but first stab at including things didn't quite work
// and the warnings are just noise
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wstrict-prototypes"

// ------------------------------
// SYNCHRONOUS LIBC SUPPORT STUBS
// ------------------------------

int _write(int fd, const void* buf, uint32_t count) {
  return _tock_write(fd, buf, count);
}
