#ifndef __DEFINES_H__
#define __DEFINES_H__

#define byte  unsigned char
#define dword unsigned int

#define SERIAL_FIFOMAXSIZE 16 // FIFO max size (16 bytes)

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 7984

/* macro function */
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#endif // __DEFINES_H__