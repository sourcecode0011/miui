


#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <asm/types.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fbviewer.h"

static int double_buffered;

int openFB(const char *name);
void closeFB(int fh);
void getVarScreenInfo(int fh, struct fb_var_screeninfo *var);
void setVarScreenInfo(int fh, struct fb_var_screeninfo *var);
void getFixScreenInfo(int fh, struct fb_fix_screeninfo *fix);

static int set_displayed_framebuffer(struct framebuffer *fb, unsigned int n)
{
	if (n > 1)
		return -1;
	//fb->var.yres_virtual = fb->var.yres * 2;
	fb->var.yoffset = n * fb->var.yres;
	if (ioctl(fb->fbh, FBIOPAN_DISPLAY, &fb->var) < 0) {
		fprintf(stderr, "active fb swap failed\n");
	}
	fb->displayed_buffer = n;
	return 0;
}

int fb_init(struct framebuffer *fb)
{
	/* get the framebuffer device handle */
	fb->fbh= openFB(NULL);
	if(fb->fbh == -1)
		return -1;

	/* read current video mode */
	getVarScreenInfo(fb->fbh, &fb->var);
	getFixScreenInfo(fb->fbh, &fb->fix);

	printf("fb0 reports (possibly inaccurate):\n"
           "  vi.bits_per_pixel = %d\n"
           "  vi.red.offset   = %3d   .length = %3d\n"
           "  vi.green.offset = %3d   .length = %3d\n"
           "  vi.blue.offset  = %3d   .length = %3d\n"
		"yres=%u\n"
		"yres_virtual=%u\n"
		"yoffset=%u\n",
           fb->var.bits_per_pixel,
           fb->var.red.offset, fb->var.red.length,
           fb->var.green.offset, fb->var.green.length,
           fb->var.blue.offset, fb->var.blue.length,
	   fb->var.yres, fb->var.yres_virtual, fb->var.yoffset);

	fb->screen_x = fb->var.xres;
	fb->screen_y = fb->var.yres;

	fb->x_stride = (fb->fix.line_length * 8) / fb->var.bits_per_pixel;

	switch (fb->var.bits_per_pixel) {
		case 8: fb->cpp = 1;break;
		case 16: fb->cpp = 2;break;
		case 24:
		case 32: fb->cpp = 4;break;
	}
	printf("singal=%u\nsmem_len=%u\n", fb->screen_x * fb->screen_y * fb->cpp, fb->fix.smem_len);
	fb->fb_base = (unsigned char*)mmap(NULL, fb->fix.smem_len, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fbh, 0);

	fb->fb_tmp = (unsigned char *)malloc( fb->var.xres * fb->var.yres * fb->var.bits_per_pixel / 8);

	if (fb->var.yres * fb->fix.line_length * 2 <= fb->fix.smem_len) {
		double_buffered = 1;
		set_displayed_framebuffer(fb, 0);
	} else {
		double_buffered = 0;
		fb->displayed_buffer = 1;
		//fb->fb_tmp = (unsigned char *)malloc(fb->fix.smem_len);
	}

//printf("===framebuff===%x \n",fb->fb_base);
	if(fb->fb_base == MAP_FAILED)
	{
		close(fb->fbh);
		if (fb->fb_tmp != NULL) {
			free(fb->fb_tmp);
			fb->fb_tmp = NULL;
		}
		perror("mmap");
		return -1;
	}

	return 0;
}

void fb_release(struct framebuffer *fb)
{
	if (fb != NULL) {
		close(fb->fbh);
		if (fb->fb_tmp != NULL) {
			free(fb->fb_tmp);
			fb->fb_tmp = NULL;
		}
	}
}
void fb_flush(struct framebuffer *fb)
{

		if (double_buffered != 0)
		{
			memcpy(fb->fb_base + ((1-fb->displayed_buffer)*fb->screen_y *  fb->x_stride ) * fb->cpp, fb->fb_tmp, fb->screen_x * fb->screen_y * fb->cpp);
			set_displayed_framebuffer(fb, 1-fb->displayed_buffer);
		}
		else {
			memcpy(fb->fb_base, fb->fb_tmp, fb->fix.smem_len);
		}

}

int openFB(const char *name)
{
	int fh;
	char *dev;

	if(name == NULL)
	{
		dev = getenv("FRAMEBUFFER");
		if(dev) name = dev;
		else name = DEFAULT_FRAMEBUFFER;
	}

	if((fh = open(name, O_RDWR)) == -1)
	{
		fprintf(stderr, "open %s: %s\n", name, strerror(errno));
		return -1;
	}
	return fh;
}

void closeFB(int fh)
{
	close(fh);
}

void getVarScreenInfo(int fh, struct fb_var_screeninfo *var)
{
	if(ioctl(fh, FBIOGET_VSCREENINFO, var))
	{
		fprintf(stderr, "ioctl FBIOGET_VSCREENINFO: %s\n", strerror(errno));
		exit(1);
	}
}

void setVarScreenInfo(int fh, struct fb_var_screeninfo *var)
{
	if(ioctl(fh, FBIOPUT_VSCREENINFO, var))
	{
		fprintf(stderr, "ioctl FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
		exit(1);
	}
}

void getFixScreenInfo(int fh, struct fb_fix_screeninfo *fix)
{
	if (ioctl(fh, FBIOGET_FSCREENINFO, fix))
	{
		fprintf(stderr, "ioctl FBIOGET_FSCREENINFO: %s\n", strerror(errno));
		exit(1);
	}
}

