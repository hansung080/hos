#ifndef __DEFINES_H__
#define __DEFINES_H__

// Unix-based OS such as linux, macOS has no difference between binary file and text file.
// Thus, O_BINARY option can be ignored on those OS.
#ifndef O_BINARY
#define O_BINARY 0
#endif

#define BYTES_OF_SECTOR 512

#endif // __DEFINES_H__