#ifndef __FONTS_H__
#define __FONTS_H__

/* Bitstream Vera Sans Mono (English) */
#define FONT_VERAMONO_ENG_WIDTH  8
#define FONT_VERAMONO_ENG_HEIGHT 16

/**
  Bitmap Font Data
  - A bit in bitmap represents a color (16 bits) in video memory or a pixel (16 bits) in screen.
*/

/**
  - font          : Bitstream Vera Sans Mono
  - font style    : normal
  - font size     : 16
  - font type     : bitmap
  - language      : English
  - desciption    : 8 * 16 pixels per a character, total 256 characters (same order as ASCII code)
  - download site : https://www.gnome.org/fonts/
*/
extern unsigned char g_fontVeraMonoEng[];

#endif // __FONTS_H__
