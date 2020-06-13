#include "bootanimation.h"
#include "fbviewer.h"

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

/* Public Use Functions:
 *
 * extern int fb_display(unsigned char *rgbbuff,
 *	 int x_size, int y_size,
 *	 int x_pan, int y_pan,
 *	 int x_offs, int y_offs);
 *
 * extern int getCurrentRes(int *x,int *y);
 *
 */
#if 0
__u16 red[256], green[256], blue[256];
struct fb_cmap map332 = {0, 256, red, green, blue, NULL};
__u16 red_b[256], green_b[256], blue_b[256];
struct fb_cmap map_back = {0, 256, red_b, green_b, blue_b, NULL};
#endif
static int double_buffered;

int openFB(const char *name);
void closeFB(int fh);
void getVarScreenInfo(int fh, struct fb_var_screeninfo *var);
void setVarScreenInfo(int fh, struct fb_var_screeninfo *var);
void getFixScreenInfo(int fh, struct fb_fix_screeninfo *fix);
void set332map(int fh);
void* convertRGB2FB(int fh, unsigned char *rgbbuff, unsigned long count, int bpp, int *cpp);
void blit2FB(int fh, unsigned char *fbbuff, unsigned char *alpha,
	unsigned int pic_xs, unsigned int pic_ys,
	unsigned int scr_xs, unsigned int scr_ys,
	unsigned int xp, unsigned int yp,
	unsigned int xoffs, unsigned int yoffs,
	int cpp);
void fill2FB(int fh, unsigned char *fb_base, unsigned char *fbbuff, unsigned char *alpha,
	unsigned int pic_xs, unsigned int pic_ys,
	unsigned int scr_xs, unsigned int scr_ys,
	unsigned int xp, unsigned int yp,
	unsigned int xoffs, unsigned int yoffs,
	int cpp);
#if 1

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

#ifdef SPENT_TIME
typedef long long int  nsecs_t;
#include <sys/time.h>
#endif
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
int fb_drawfull(struct Image *i, struct framebuffer *fb)
{
	printf("%s =%d %x width %d height %d double_buffered %d \n",__FILE__,__LINE__,i,i->width, i->height, double_buffered);
#if 0
	fill2FB(fb->fbh, double_buffered?fb->fb_base:fb->fb_tmp, i->fbbuffer,
		i->alpha, i->width, i->height, fb->x_stride,
		fb->var.yres_virtual, 0, 0, i->x_offs,
		i->y_offs + (1-fb->displayed_buffer)*fb->screen_y, fb->cpp);
printf("%s =%d \n",__FILE__,__LINE__);
if (double_buffered != 0)
		set_displayed_framebuffer(fb, 1-fb->displayed_buffer);
	else {
		memcpy(fb->fb_base, fb->fb_tmp, fb->fix.smem_len);
	}

#else
memcpy(fb->fb_tmp, i->fbbuffer, i->width*i->height*4);
#endif

	//printf("%s =%d \n",__FILE__,__LINE__);

	return 0;
}



void fb_drawrect(struct Image *i,struct framebuffer *fb)
{
	unsigned char* framebuf = fb->fb_tmp + i->y_offs*fb->screen_x*4;
	unsigned char*p;
	int ret = 0;
	int di,dj;
	unsigned char alpha;
	unsigned char r,g,b;
	unsigned char r0,g0,b0;
	int x = i->x_offs;
	unsigned char *alp = i->alpha;

	p=i->fbbuffer;
	printf("%d==%d alp %x\n",i->width, i->height,alp);
	for(dj=0;dj<i->height;dj++)
	{
		
		for(di=0;di<i->width;di++)
		{
			b=p[di*3];
			g =p[di*3+1];
			r =p[di*3+2];
			alpha=alp[di];
			//if(alpha !=0)alpha=0xff;
			//printf("%x,",alpha);
			b0= framebuf[x*4+di*4];
			g0= framebuf[x*4+di*4+1];
			r0= framebuf[x*4+di*4+2];
			
			framebuf[x*4+di*4]=(((unsigned int)(alpha) * b) + (unsigned int)(255 - alpha) * b0) / 255;//b
			framebuf[x*4+4*di+1]=(((unsigned int)(alpha) * g) + (unsigned int)(255 - alpha) * g0) / 255;//g
			framebuf[x*4+4*di+2]=(((unsigned int)(alpha) * r) + (unsigned int)(255 - alpha) * r0) / 255;//r

			//framebuf[x*4+4*di+3]=0x80;//
			//if(0x30 > *p)
			//memcpy(framebuf+x*4+di*4,p, 4);
			//p+=4;
			
		}
	
		//memcpy(framebuf+x*4,p,i.width*4);
		p += i->width*3;
		alp +=i->width;
		framebuf += fb->screen_x*4;
	}

}


#endif
#if 0
int fb_display(unsigned char *rgbbuff, unsigned char * alpha,
               unsigned int x_size, unsigned int y_size,
               unsigned int x_pan, unsigned int y_pan,
               unsigned int x_offs, unsigned int y_offs)
{
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	unsigned char *fbbuff = NULL;
	int fh = -1, bp = 0;
	unsigned int x_stride;

	/* get the framebuffer device handle */
	fh = openFB(NULL);
	if(fh == -1)
		return -1;

	/* read current video mode */
	getVarScreenInfo(fh, &var);
	getFixScreenInfo(fh, &fix);

	x_stride = (fix.line_length * 8) / var.bits_per_pixel;

	/* correct panning */
	if(x_pan > x_size - x_stride) x_pan = 0;
	if(y_pan > y_size - var.yres) y_pan = 0;
	/* correct offset */
	if(x_offs + x_size > x_stride) x_offs = 0;
	if(y_offs + y_size > var.yres) y_offs = 0;

	/* blit buffer 2 fb */
	fbbuff = (unsigned char*)convertRGB2FB(fh, rgbbuff, x_size * y_size, var.bits_per_pixel, &bp);
#if 0
	blit2FB(fh, fbbuff, alpha, x_size, y_size, x_stride, var.yres, x_pan, y_pan, x_offs, y_offs, bp);
#else
	blit2FB(fh, fbbuff, alpha, x_size, y_size, x_stride, var.yres_virtual, x_pan, y_pan, x_offs, y_offs + var.yoffset, bp);
#endif
	free(fbbuff);

	/* close device */
	closeFB(fh);
	return 0;
}
#endif
int getCurrentRes(int *x, int *y)
{
	struct fb_var_screeninfo var;
	int fh;
	fh = openFB(NULL);
	if(fh == -1)
		return -1;
	getVarScreenInfo(fh, &var);
	*x = var.xres;
	*y = var.yres;
	closeFB(fh);
	return 0;
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
#if 0
static void make332map(struct fb_cmap *map)
{
	int rs, gs, bs, i;
	int r = 8, g = 8, b = 4;

	map->red = red;
	map->green = green;
	map->blue = blue;

	rs = 256 / (r - 1);
	gs = 256 / (g - 1);
	bs = 256 / (b - 1);

	for (i = 0; i < 256; i++)
	{
		map->red[i]   = (rs * ((i / (g * b)) % r)) * 255;
		map->green[i] = (gs * ((i / b) % g)) * 255;
		map->blue[i]  = (bs * ((i) % b)) * 255;
	}
}

static void set8map(int fh, struct fb_cmap *map)
{
	if (ioctl(fh, FBIOPUTCMAP, map) < 0)
	{
		fprintf(stderr, "Error putting colormap");
		exit(1);
	}
}

static void get8map(int fh, struct fb_cmap *map)
{
	if (ioctl(fh, FBIOGETCMAP, map) < 0)
	{
		fprintf(stderr, "Error getting colormap");
		exit(1);
	}
}

void set332map(int fh)
{
	make332map(&map332);
	set8map(fh, &map332);
}
void fill2FB(int fh, unsigned char *fb_base, unsigned char *fbbuff, unsigned char *alpha,
	unsigned int pic_xs, unsigned int pic_ys,
	unsigned int scr_xs, unsigned int scr_ys,
	unsigned int xp, unsigned int yp,
	unsigned int xoffs, unsigned int yoffs,
	int cpp)
{
	int i, xc, yc;

	unsigned char *fbptr;
	unsigned char *imptr;

#ifdef DEBUG
	printf("pic x:%u,y:%u\n", pic_xs, pic_ys);
	printf("screen x:%u,y:%u\n", scr_xs, scr_ys);
	printf("pen x:%u,y:%u\n", xp, yp);
	printf("offset x:%u,y:%u\n", xoffs, yoffs);
#endif

	xc = (pic_xs > scr_xs) ? scr_xs : pic_xs;
	yc = (pic_ys > scr_ys) ? scr_ys : pic_ys;

	if(cpp == 1)
	{
		get8map(fh, &map_back);
		set332map(fh);
	}

	fbptr = fb_base + (yoffs * scr_xs + xoffs) * cpp;
	imptr = fbbuff + (yp * pic_xs + xp) * cpp;

	if(alpha)
	{
		unsigned char * alphaptr;
		int from, to, x;

		alphaptr = alpha + (yp  * pic_xs + xp);

		for(i = 0; i < yc; i++, fbptr += scr_xs * cpp, imptr += pic_xs * cpp, alphaptr += pic_xs)
		{
			for(x = 0; x<xc; x++)
			{
				int v;

				from = to = -1;
				for(v = x; v<xc; v++)
				{
					if(from == -1)
					{
						if(alphaptr[v] > 0x80) from = v;
					}
					else
					{
						if(alphaptr[v] < 0x80)
						{
							to = v;
							break;
						}
					}
				}
				if(from == -1)
					break;

				if(to == -1) to = xc;

				memcpy(fbptr + (from * cpp), imptr + (from * cpp), (to - from - 1) * cpp);
				x += to - from - 1;
			}
		}
	}
	else {
		for(i = 0; i < yc; i++, fbptr += scr_xs * cpp, imptr += pic_xs * cpp)
			memcpy(fbptr, imptr, xc * cpp);
	}

	if(cpp == 1)
		set8map(fh, &map_back);

}
void blit2FB(int fh, unsigned char *fbbuff, unsigned char *alpha,
	unsigned int pic_xs, unsigned int pic_ys,
	unsigned int scr_xs, unsigned int scr_ys,
	unsigned int xp, unsigned int yp,
	unsigned int xoffs, unsigned int yoffs,
	int cpp)
{
	int i, xc, yc;
	unsigned char *fb;

	unsigned char *fbptr;
	unsigned char *imptr;

	xc = (pic_xs > scr_xs) ? scr_xs : pic_xs;
	yc = (pic_ys > scr_ys) ? scr_ys : pic_ys;

	fb = (unsigned char*)mmap(NULL, scr_xs * scr_ys * cpp, PROT_WRITE | PROT_READ, MAP_SHARED, fh, 0);

	if(fb == MAP_FAILED)
	{
		perror("mmap");
		return;
	}

	if(cpp == 1)
	{
		get8map(fh, &map_back);
		set332map(fh);
	}

	fbptr = fb + (yoffs * scr_xs + xoffs) * cpp;
	imptr = fbbuff + (yp * pic_xs + xp) * cpp;

	if(alpha)
	{
		unsigned char * alphaptr;
		int from, to, x;

		alphaptr = alpha + (yp  * pic_xs + xp);

		for(i = 0; i < yc; i++, fbptr += scr_xs * cpp, imptr += pic_xs * cpp, alphaptr += pic_xs)
		{
			for(x = 0; x<xc; x++)
			{
				int v;

				from = to = -1;
				for(v = x; v<xc; v++)
				{
					if(from == -1)
					{
						if(alphaptr[v] > 0x80) from = v;
					}
					else
					{
						if(alphaptr[v] < 0x80)
						{
							to = v;
							break;
						}
					}
				}
				if(from == -1)
					break;

				if(to == -1) to = xc;

				memcpy(fbptr + (from * cpp), imptr + (from * cpp), (to - from - 1) * cpp);
				x += to - from - 1;
			}
		}
	}
	else
		for(i = 0; i < yc; i++, fbptr += scr_xs * cpp, imptr += pic_xs * cpp)
			memcpy(fbptr, imptr, xc * cpp);

	if(cpp == 1)
		set8map(fh, &map_back);

	munmap(fb, scr_xs * scr_ys * cpp);
}
#endif
inline static unsigned char make8color(unsigned char r, unsigned char g, unsigned char b)
{
	return (
	(((r >> 5) & 7) << 5) |
	(((g >> 5) & 7) << 2) |
	 ((b >> 6) & 3)	   );
}

inline static unsigned short make15color(unsigned char r, unsigned char g, unsigned char b)
{
	return (
	(((r >> 3) & 31) << 10) |
	(((g >> 3) & 31) << 5)  |
	 ((b >> 3) & 31)		);
}

inline static unsigned short make16color(unsigned char r, unsigned char g, unsigned char b)
{
	return (
	(((r >> 3) & 31) << 11) |
	(((g >> 2) & 63) << 5)  |
	 ((b >> 3) & 31)		);
}

void* convertRGB2FB(int fh, unsigned char *rgbbuff, unsigned long count, int bpp, int *cpp)
{
	unsigned long i;
	void *fbbuff = NULL;
	u_int8_t  *c_fbbuff;
	u_int16_t *s_fbbuff;
	u_int32_t *i_fbbuff;

	switch(bpp)
	{
	case 8:
		*cpp = 1;
		c_fbbuff = (unsigned char *) malloc(count * sizeof(unsigned char));
		for(i = 0; i < count; i++)
		c_fbbuff[i] = make8color(rgbbuff[i*3], rgbbuff[i*3+1], rgbbuff[i*3+2]);
		fbbuff = (void *) c_fbbuff;
		break;
	case 15:
		*cpp = 2;
		s_fbbuff = (unsigned short *) malloc(count * sizeof(unsigned short));
		for(i = 0; i < count ; i++)
		s_fbbuff[i] = make15color(rgbbuff[i*3], rgbbuff[i*3+1], rgbbuff[i*3+2]);
		fbbuff = (void *) s_fbbuff;
		break;
	case 16:
		*cpp = 2;
		s_fbbuff = (unsigned short *) malloc(count * sizeof(unsigned short));
		for(i = 0; i < count ; i++)
		s_fbbuff[i] = make16color(rgbbuff[i*3], rgbbuff[i*3+1], rgbbuff[i*3+2]);
		fbbuff = (void *) s_fbbuff;
		break;
	case 24:
		*cpp = 4;
		i_fbbuff = (unsigned int *) malloc(count * sizeof(unsigned int));
		for(i = 0; i < count ; i++)
		i_fbbuff[i] = ((0xFF << 24) & 0xFF000000) |
				((rgbbuff[i*3] << 16) & 0xFF0000) |
				((rgbbuff[i*3+1] << 8) & 0xFF00) |
				(rgbbuff[i*3+2] & 0xFF);
		fbbuff = (void *) i_fbbuff;
		break;
	case 32:
		*cpp = 4;
		i_fbbuff = (unsigned int *) malloc(count * sizeof(unsigned int));
		for(i = 0; i < count ; i++)
		i_fbbuff[i] = ((rgbbuff[i*3+3] << 24) & 0xFF000000) |
				((rgbbuff[i*3] << 16) & 0xFF0000) |
				((rgbbuff[i*3+1] << 8) & 0xFF00) |
				(rgbbuff[i*3+2] & 0xFF);
		fbbuff = (void *) i_fbbuff;
		break;
	default:
		fprintf(stderr, "Unsupported video mode! You've got: %dbpp\n", bpp);
		exit(1);
	}
	return fbbuff;
}

