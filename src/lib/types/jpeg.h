#ifndef __TYPES_JPEG_H__
#define __TYPES_JPEG_H__

#pragma pack(push, 1)

// huffman table
typedef struct __Huff {
	int elem; // element count
	unsigned short code[256];
	unsigned char size[256];
	unsigned char value[256];
} Huff;

// JPEG struct
typedef struct __Jpeg {
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

#endif // __TYPES_JPEG_H__