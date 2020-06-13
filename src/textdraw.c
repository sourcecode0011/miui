#include "miui.h"
#include "fbviewer.h"
static int char_width;
static int char_height;
static char inited = 0;

int draw_text(const char *str, int x, int y,bool bold)
{
	if(inited)
	{
	    int str_len_px = gr_measure(gr_sys_font(), str);

	    if (x < 0)
	        x = (gr_fb_width() - str_len_px) / 2;
	    if (y < 0)
	        y = (gr_fb_height() - char_height) / 2;
	    gr_text(gr_sys_font(), x, y, str, bold);

	    return y + char_height;
	}else
		return 0;
}
/*
static void android_green(void)
{
    gr_color(0xa4, 0xc6, 0x39, 255);
}
*/

void clear_screen(void)
{
    gr_color(0, 0, 0, 255);
    gr_clear();
}


int init_text(struct framebuffer *fb)
{
printf("%s %d \n", __FILE__,__LINE__);
	gr_init(fb->var.xres,fb->var.yres,fb->fix.line_length,fb->var.bits_per_pixel,fb->fb_tmp);
	gr_font_size(gr_sys_font(), &char_width, &char_height);
	
//	clear_screen();
	inited = 1;
//	android_green();
  //  draw_text("Hello World!", -1, -1);
//	gr_flip();
	


	
	return 0;
}

int deinit_text()
{
	gr_exit();
}

