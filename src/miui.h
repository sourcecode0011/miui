
#ifndef _MINUI_H_
#define _MINUI_H_

#include <sys/types.h>

//#include <functional>

#ifdef __cplusplus 
extern "C" {
#endif

#ifndef bool
#define bool unsigned char 
#endif

#ifndef nullptr
#define nullptr NULL
#endif

typedef  unsigned int uint32_t;
typedef  unsigned char uint8_t;
 
//
// Graphics.
//

typedef struct  {
  int width;
  int height;
  int row_bytes;
  int pixel_bytes;
  unsigned char* data;
}GRSurface;

typedef struct  {
  GRSurface* texture;
  int char_width;
  int char_height;
}GRFont;

typedef enum  {
  ROTATION_NONE = 0,
  ROTATION_RIGHT = 1,
  ROTATION_DOWN = 2,
  ROTATION_LEFT = 3,
}GRRotation;

int gr_init(int _width,int _height,int _linelength,int _bitsperpixel, unsigned char* _bits);

void gr_exit();

int gr_fb_width();
int gr_fb_height();

//void gr_flip();
//void gr_fb_blank(bool blank);

void gr_clear();  // clear entire surface to current color
void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void gr_fill(int x1, int y1, int x2, int y2);

void gr_texticon(int x, int y, GRSurface* icon);

const GRFont* gr_sys_font();
int gr_init_font(const char* name, GRFont** dest);
void gr_text(const GRFont* font, int x, int y, const char* s, bool bold);
int gr_measure(const GRFont* font, const char* s);
void gr_font_size(const GRFont* font, int* x, int* y);

void gr_blit(GRSurface* source, int sx, int sy, int w, int h, int dx, int dy);
unsigned int gr_get_width(GRSurface* surface);
unsigned int gr_get_height(GRSurface* surface);

// Set rotation, flips gr_fb_width/height if 90 degree rotation difference
void gr_rotate(GRRotation rotation);


// Load a single alpha surface from a grayscale PNG image.
int res_create_alpha_surface(const char* name, GRSurface** pSurface);


// Return a list of locale strings embedded in |png_name|. Return a empty list in case of failure.


// Free a surface allocated by any of the res_create_*_surface()
// functions.
void res_free_surface(GRSurface* surface);


#ifdef __cplusplus 
};
#endif

#endif
