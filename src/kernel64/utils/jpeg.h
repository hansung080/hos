/**
  JPEG decoding engine for DCT-baseline
  JPEGLS - Copyright(c) 2004 by Hajime Uchimura (nikutama@gmail.com)
  
  @ history
    2003/04/28 : added OSASK-GUI by H.Kawai
    2003/05/12 : optimized DCT (20-bits fixed point, etc...) -> line 407-464 by I.Tak.
    2009/11/21 : optimized to RGB565 by kkamagui
  
  @ web pages
    http://ko.sourceforge.jp/cvs/view/osask/osask/
    https://ko.osdn.net/cvs/view/osask/osask/
*/

#ifndef __UTILS_JPEG_H__
#define __UTILS_JPEG_H__

#include "../core/types.h"
#include "../core/2d_graphics.h"

/**
  < JPEG Image Encoding/Decoding >
  
  @ JPEG Image Encoding
  ------------     ------------------------------------------     -------------------------------------     ------------------     ----------------------     --------------
  | original | --> | color space conversion & down sampling | --> | discrete cosine transform         | --> | quantization   | --> | entropy encoding   | --> | JPEG image |
  | image    |     | (RGB -> YCbCr)                         |     | (color space -> frequency domain) |     |                |     | (huffman encoding) |     |            |
  ------------     ------------------------------------------     -------------------------------------     ------------------     ----------------------     --------------
  
  @ JPEG Image Decoding
  ------------     ------------------------------------------     -------------------------------------     ------------------     ----------------------     --------------
  | restored | <-- | color space conversion                 | <-- | inverse discrete cosine transform | <-- | dequantization | <-- | entropy decoding   | <-- | JPEG image |
  | image    |     | (RGB <- YCbCr)                         |     | (color space <- frequency domain) |     |                |     | (huffman decoding) |     |            |
  ------------     ------------------------------------------     -------------------------------------     ------------------     ----------------------     --------------
*/

#pragma pack(push, 1)

// huffman table
typedef struct k_Huff {
	int elem; // element count
	unsigned short code[256];
	unsigned char size[256];
	unsigned char value[256];
} Huff;

// JPEG struct
typedef struct k_Jpeg {
	// SOF: start of frame
	int width;
	int height;
	
	// MCU: minimum coded unit
	int mcu_width;
	int mcu_height;
	
	int max_h, max_v;
	int compo_count;
	int compo_id[3];
	int compo_sample[3];
	int compo_h[3];
	int compo_v[3];
	int compo_qt[3];
	
	// SOS: start of scan
	int scan_count;
	int scan_id[3];
	int scan_ac[3];
	int scan_dc[3];
	int scan_h[3];  // sampling element count
	int scan_v[3];  // sampling element count
	int scan_qt[3]; // quantization table index
	
	// DRI: data restart interval
	int interval;
	
	int mcu_buf[32 * 32 * 4]; // buffer
	int *mcu_yuv[4];
	int mcu_preDC[3];
	
	// DQT: define quantization table
	int dqt[3][64];
	int n_dqt;
	
	// DHT: define huffman table
	Huff huff[2][3];
	
	// I/O: input/output
	const unsigned char* data;
	int data_index;
	int data_size;
	
	unsigned long bit_buff;
	int bit_remain;
} Jpeg;

#pragma pack(pop)

bool k_initJpeg(Jpeg* jpeg, const byte* fileBuffer, dword fileSize);
bool k_decodeJpeg(Jpeg* jpeg, Color* imageBuffer);

/* Buffer Read Functions */
static unsigned char get_byte(Jpeg* jpeg);
static int get_word(Jpeg* jpeg);
static unsigned short get_bits(Jpeg* jpeg, int bit);

/* JPEG Segment Functions */
static void jpeg_skip(Jpeg* jpeg);
static int jpeg_sof(Jpeg* jpeg);
static void jpeg_dri(Jpeg* jpeg);
static int jpeg_dqt(Jpeg* jpeg);
static int jpeg_dht(Jpeg* jpeg);
static void jpeg_sos(Jpeg* jpeg);

/* MCU Decoding Functions */
static int jpeg_decode_init(Jpeg* jpeg);
static int jpeg_huff_decode(Jpeg* jpeg, int tc, int th);

/* IDCT Functions */
static void jpeg_idct_init(void);
static void jpeg_idct(int* block, int* dest);
static int jpeg_get_value(Jpeg* jpeg, int size);

/* Block Decoding Function */
static int jpeg_decode_huff(Jpeg* jpeg, int scan, int* block);

/* Block Restoring Functions */
static void jpeg_mcu_bitblt(int* src, int* dest, int width, int x0, int y0, int x1, int y1);
static int jpeg_decode_mcu(Jpeg* jpeg);
static int jpeg_decode_yuv(Jpeg* jpeg, int h, int v, Color* imageBuffer);

#endif // __UTILS_JPEG_H__
