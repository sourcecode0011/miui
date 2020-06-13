#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#endif

#include "fbviewer.h"


void* convertRGB2FB(int fh, unsigned char *rgbbuff, unsigned long count, int bpp, int *cpp);


struct framebuffer g_fb;

int initfb()
{
	if (fb_init(&g_fb) < 0) {
		printf( "fb_init failed.\n");
		return -1;
	}
	//memset(g_fb.fb_base, 0x00, g_fb.fix.smem_len);
	init_text(&g_fb);
return 0;
}
void deinitfb()
{
	fb_release(&g_fb);
	deinit_text();
}
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
	unsigned char  *c_fbbuff;
	unsigned short *s_fbbuff;
	unsigned int *i_fbbuff;

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

static int loadimage(char *filename, struct Image *i)
{
	int (*load)(char *, unsigned char *, unsigned char **, int, int);
	unsigned char * image = NULL;
	unsigned char * alpha = NULL;

	int x_size, y_size;

#ifdef FBV_SUPPORT_PNG
	if(fh_png_id(filename))
	if(fh_png_getsize(filename, &x_size, &y_size) == FH_ERROR_OK)
	{
		load = fh_png_load;
		goto identified;
	}
#endif

#ifdef FBV_SUPPORT_JPEG
	if(fh_jpeg_id(filename))
	if(fh_jpeg_getsize(filename, &x_size, &y_size) == FH_ERROR_OK)
	{
		load = fh_jpeg_load;
		goto identified;
	}
#endif

#ifdef FBV_SUPPORT_BMP
	if(fh_bmp_id(filename))
	if(fh_bmp_getsize(filename, &x_size, &y_size) == FH_ERROR_OK)
	{
		load = fh_bmp_load;
		goto identified;
	}
#endif
	fprintf(stderr, "%s: Unable to access file or file format unknown.\n", filename);
	return(1);

identified:
	if(!(image = (unsigned char*)malloc(x_size * y_size * 3)))
	{
		fprintf(stderr, "%s: Out of memory.\n", filename);
		return -1;
	}
	if(load(filename, image, &alpha, x_size, y_size) != FH_ERROR_OK)
	{
		fprintf(stderr, "%s: Image data is corrupt?\n", filename);
		free(image);
		return -1;
	}
	//if(!opt_alpha)
	//{
		//free(alpha);
		//alpha = NULL;
	//}


	i->width = x_size;
	i->height = y_size;
	i->rgb = image;
	i->alpha = alpha;
	i->do_free = 0;

	return 0;
}

int load_image(char *filename, struct Image *i, int dispx,int dispy, char iscov)
{
	int ret = 0;

	if (!i->fbbuffer) {
		if (!i->rgb) {
			ret = loadimage(filename, i);
			if (ret != 0) {
				fprintf(stderr, "load_image [%s] failed\n", filename);
				return ret;
			}
		}
		if(iscov){
		i->fbbuffer = (unsigned char*)convertRGB2FB(0, i->rgb, i->width * i->height, g_fb.var.bits_per_pixel, &g_fb.cpp);
		free(i->rgb);
		i->rgb = NULL;
			}
		else{
			i->fbbuffer = i->rgb;}
		
		i->x_offs = dispx;

		i->y_offs = dispy;
	}
	return ret;
}

void drawfullscreen(struct Image *i)
{
	memcpy(g_fb.fb_tmp, i->fbbuffer, i->width*i->height*4);
}

void drawrect(struct Image *i)
{
	struct framebuffer *fb = &g_fb;
	unsigned char* framebuf = fb->fb_tmp + (i->y_offs*fb->screen_x)*4;
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
void drawcolor(unsigned int argb)
{
	int i;
	unsigned int *p = g_fb.fb_tmp;
	for(i=0;i<g_fb.screen_x*g_fb.screen_y;i++)
		*p++ =  argb;
}

void displayframe()
{
	fb_flush(&g_fb);
}
#ifdef WIN32

void win_displayframe(HDC hdc)
{
	win_flush(&g_fb, hdc);
}
#endif
void preshowpage(const char* file)
{
	
	struct Image imag={0};
	if(load_image(file,  &imag, 0,0,1)!= 0)
		goto exit;
		drawfullscreen(&imag);
		fb_flush(&g_fb);
	exit:
	//	printf("%s =%d \n",__FILE__,__LINE__);
	free(imag.fbbuffer);
	imag.fbbuffer = NULL;
	if(imag.alpha)
		free(imag.alpha);
	printf("%s =%d \n",__FILE__,__LINE__);
}

