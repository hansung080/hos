#ifndef __TYPES_UTIL_H__
#define __TYPES_UTIL_H__

/* Macro Functions */
#define MIN(x, y)     (((x) < (y)) ? (x) : (y))
#define MAX(x, y)     (((x) > (y)) ? (x) : (y))
#define ABS(x)        (((x) >= 0) ? (x) : -(x))
#define SWAP(x, y, t) ((t) = (x)), ((x) = (y)), ((y) = (t))

#endif // __TYPES_UTIL_H__
