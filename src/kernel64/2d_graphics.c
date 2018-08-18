#include "2d_graphics.h"
#include "vbe.h"
#include "fonts.h"
#include "util.h"

inline void k_drawPixel(int x, int y, Color color) {
	VbeModeInfoBlock* vbeMode;
	
	vbeMode = k_getVbeModeInfoBlock();
	
	*(((Color*)((qword)vbeMode->physicalBaseAddr)) + y * vbeMode->xResolution + x) = color;
}

/**
  < Bresenham Line Algorithm >
  - select the next pixel which is the closest one to the line,
    using the way that checks if the accumulated error is more than 0.5 pixel.
  - optimize operations by replacing float, multiplication, division operation with addition, subtraction, shift operation.
      --------------------------------------------------
      error = (dy/dx)*n >= 0.5
              dy*n >= 0.5*dx
              2*dy*n >= dx
      2*dx*error = error' = 2*dy*n >= dx
      --------------------------------------------------
      error = error - 1.0 = (dy/dx)*n - 1.0
      dx*error = dy*n - dx
      2*dx*error = 2*dy*n - 2*dx
      error' = error' - 2*dx
      --------------------------------------------------
*/
void k_drawLine(int x1, int y1, int x2, int y2, Color color) {
	int deltaX, deltaY;
	int error = 0;
	int deltaError;
	int x, y; // x, y to draw line.
	int stepX, stepY;
	
	deltaX = x2 - x1;
	deltaY = y2 - y1;
	
	if (deltaX < 0) {
		deltaX = -deltaX;
		stepX = -1;
		
	} else {
		stepX = 1;
	}
	
	if (deltaY < 0) {
		deltaY = -deltaY;
		stepY = -1;
		
	} else {
		stepY = 1;
	}
	
	// If deltaX > deltaY, draw line based on x-axis.
	if (deltaX > deltaY) {
		deltaError = deltaY << 1; // 2*dy
		y = y1;
		for (x = x1; x != x2; x += stepX) {
			k_drawPixel(x, y, color);
			
			error += deltaError;
			if (error >= deltaX) { // error' = 2*dy*n >= dx
				y += stepY;
				error -= deltaX << 1; // error' = error' - 2*dx
			}
		}
		
		k_drawPixel(x, y, color);
		
	// If deltaX <= deltaY, draw line based on y-axis.
	} else {
		deltaError = deltaX << 1; // 2*dx
		x = x1;
		for (y = y1; y != y2; y += stepY) {
			k_drawPixel(x, y, color);
			
			error += deltaError;
			if (error >= deltaY) { // error' = 2*dx*n >= dy
				x += stepX;
				error -= deltaY << 1; // error' = error' - 2*dy
			}
		}
		
		k_drawPixel(x, y, color);
	}
}

void k_drawRect(int x1, int y1, int x2, int y2, Color color, bool fill) {
	int width;
	int temp;
	int y; // y to draw rectangle.
	int stepY;
	VbeModeInfoBlock* vbeMode;
	Color* videoMemAddr;
	
	if (fill == false) {
		// draw 4 lines (edges) of rectangle.
		k_drawLine(x1, y1, x2, y1, color);
		k_drawLine(x1, y1, x1, y2, color);
		k_drawLine(x2, y1, x2, y2, color);
		k_drawLine(x1, y2, x2, y2, color);
		
	} else {
		vbeMode = k_getVbeModeInfoBlock();
		videoMemAddr = (Color*)((qword)vbeMode->physicalBaseAddr);
		
		// If x2 < x1, swap the two vertex, because k_memsetWord sets memory from low x to high x.
		if (x2 < x1) {
			SWAP(x1, x2, temp);
			SWAP(y1, y2, temp);
		}
		
		width = x2 - x1 + 1;
		
		if (y1 <= y2) {
			stepY = 1;
			
		} else {
			stepY = -1;
		}
		
		videoMemAddr += y1 * vbeMode->xResolution + x1;
		
		for (y = y1; y != y2; y += stepY) {
			k_memsetWord(videoMemAddr, color, width);
			
			if (stepY >= 0) {
				videoMemAddr += vbeMode->xResolution;
				
			} else {
				videoMemAddr -= vbeMode->xResolution;
			}
		}
		
		k_memsetWord(videoMemAddr, color, width);
	}
}

/**
  < Midpoint Circle Algorithm >
  - select the next pixel which is the closest one to the circle,
    using the way that checks if the midpoint of two candidate pixels is inside circle or outside circle.
  - calculate only 1/8 (45 degree) of circle, and draw remaining 7/8 of circle using symmetry of circle.
  - calculate circle with origin (0, 0), and move it to origin (x, y).
  - optimize operations by replacing float, multiplication, division operation with addition, subtraction, shift operation.
      --------------------------------------------------
      first pixel:     (0, r)     first distance:     d_first
      last pixel:      (x-1, y),  last distance:      d_old
      current pixel 1: (x, y),    current distance 1: d_new1
      current pixel 2: (x, y-1),  current distance 2: d_new2
      --------------------------------------------------
      d_first = 0^2 + (r-0.5)^2 - r^2
              = 0^2 + r^2 - 2*r*0.5 + 0.25 - r^2
              = -r + 0.25
              = -r  // discard 0.25 in integer operation
      
      d_old = (x-1)^2 + (y-0.5)^2 - r^2
      d_new1 = x^2 + (y-0.5)^2 - r^2
      d_new2 = x^2 + (y-1.5)^2 - r^2
      --------------------------------------------------
      d_new1 = x^2 + (y-0.5)^2 - r^2
             = ((x-1)+1)^2 + (y-0.5)^2 - r^2
             = (x-1)^2 + 2*(x-1) + 1 + (y-0.5)^2 - r^2
             = (x-1)^2 + (y-0.5)^2 - r^2 + 2*(x-1) + 1
             = d_old + 2*x - 1
      --------------------------------------------------
      d_new2 = x^2 + (y-1.5)^2 - r^2
             = x^2 + ((y-0.5)-1)^2 - r^2
             = x^2 + (y-0.5)^2 - 2*(y-0.5) + 1 - r^2
             = x^2 + (y-0.5)^2 - r^2 - 2*(y-0.5) + 1
             = d_new1 - 2*y + 2
      --------------------------------------------------
*/

void k_drawCircle(int x, int y, int radius, Color color, bool fill) {
	int circleX, circleY; // x, y to draw circle.
	int distance; // distance difference
	
	if (radius < 0) {
		return;
	}
	
	// start drawing circle from (0, r)
	circleY = radius;
	
	if (fill == false) {
		// draw 4 points contacting with x-axis and y-axis.
		k_drawPixel(0 + x, radius + y, color);
		k_drawPixel(0 + x, -radius + y, color);
		k_drawPixel(radius + x, 0 + y, color);
		k_drawPixel(-radius + x, 0 + y, color);
		
	} else {
		// draw 2 lines matching x-axis and y-axis.
		k_drawLine(0 + x, radius + y, 0 + x, -radius + y, color);
		k_drawLine(radius + x, 0 + y, -radius + x, 0 + y, color);
	}
	
	distance = -radius; // d_first = -r  // discard 0.25 in integer operation.
	
	/**
	  calculate only 1/8 (45 degree) of circle, and draw remaining 7/8 of circle using symmetry of circle.
	  calculate circle with origin (0, 0), and move it to origin (x, y).
	*/
	for (circleX = 1; circleX <= circleY; circleX++) {
		distance += (circleX << 1) - 1; // d_new1 = d_old + 2*x - 1
		
		// If the midpoint is outside circle, select the lower pixel out of two candidate pixels.
		// add equal(=) to the condition in oder to cover discarding 0.25.
		if (distance >= 0) {
			circleY--;
			distance += - (circleY << 1) + 2; // d_new2 = d_new1 - 2*y + 2
		}
		
		if (fill == false) {
			// draw 8 points in the symmetric position of (circleX, circleY).
			k_drawPixel(circleX + x, circleY + y, color);
			k_drawPixel(circleX + x, -circleY + y, color);
			k_drawPixel(-circleX + x, circleY + y, color);
			k_drawPixel(-circleX + x, -circleY + y, color);
			k_drawPixel(circleY + x, circleX + y, color);
			k_drawPixel(circleY + x, -circleX + y, color);
			k_drawPixel(-circleY + x, circleX + y, color);
			k_drawPixel(-circleY + x, -circleX + y, color);
			
		} else {
			// draw 4 parallel lines of x-axis in the symmetric position.
			// (use k_drawRect instead of k_drawLine, because it's faster when drawing a parallel line of x-axis.)
			k_drawRect(-circleX + x, circleY + y, circleX + x, circleY + y, color, true);
			k_drawRect(-circleX + x, -circleY + y, circleX + x, -circleY + y, color, true);
			k_drawRect(-circleY + x, circleX + y, circleY + x, circleX + y, color, true);
			k_drawRect(-circleY + x, -circleX + y, circleY + x, -circleX + y, color, true);
		}
	}
}

void k_drawText(int x, int y, Color textColor, Color backgroundColor, const char* str) {
	int currentX, currentY; // x, y to draw text.
	int i, j, k;
	byte bitmask;
	int bitmaskIndex;
	VbeModeInfoBlock* vbeMode;
	Color* videoMemAddr;
	int len;
	
	vbeMode = k_getVbeModeInfoBlock();
	videoMemAddr = (Color*)((qword)vbeMode->physicalBaseAddr);
	
	currentX = x;
	len = k_strlen(str);
	
	// draw text (string).
	for (i = 0; i < len; i++) {
		currentY = y * vbeMode->xResolution;
		
		// Bitstream Vera Sans Mono (English) font data has 8 * 16 bits per a character,
		// and the font data has same order as ASCII code.
		// Thus, bitmask index indicates a byte (8 bits) of current character in the font data.
		bitmaskIndex = str[i] * FONT_VERAMONO_ENG_HEIGHT;
		
		// draw a character.
		for (j = 0; j < FONT_VERAMONO_ENG_HEIGHT; j++) {
			bitmask = g_fontVeraMonoEng[bitmaskIndex++];
			
			// draw a line in a character.
			for (k = 0; k < FONT_VERAMONO_ENG_WIDTH; k++) {
				
				// If a bit in bitmap == 1, draw a pixel with text color.
				if (bitmask & (0x01 << (FONT_VERAMONO_ENG_WIDTH - k - 1))) {
					videoMemAddr[currentY + currentX + k] = textColor;
					
				// If a bit in bitmap == 0, draw a pixel with backgroud color.
				} else {
					videoMemAddr[currentY + currentX + k] = backgroundColor;
				}
			}
			
			// move to the next line in a character.
			currentY += vbeMode->xResolution;
		}
		
		// move to the next character.
		currentX += FONT_VERAMONO_ENG_WIDTH;
	}
}

