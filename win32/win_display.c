#include <stdio.h>
#include <windows.h>
#include "fbviewer.h"
int fb_init(struct framebuffer *fb)
{
	fb->var.xres = 1024;
	fb->var.yres = 768;
	fb->var.bits_per_pixel = 32;
	fb->fix.line_length = 1024 << 2;
	fb->screen_x = 1024;
	fb->screen_y = 768;
	fb->fb_tmp = (unsigned char *)malloc(fb->var.xres * fb->var.yres * fb->var.bits_per_pixel / 8);
	return 0;
}
void fb_release(struct framebuffer *fb)
{
	if (fb != NULL) {
		
		if (fb->fb_tmp != NULL) {
			free(fb->fb_tmp);
			fb->fb_tmp = NULL;
		}
	}
}
void fb_flush(struct framebuffer *fb)
{

}

void win_flush(struct framebuffer *fb, HDC hdc)
{
	//SetStretchBltMode(hdc, COLORONCOLOR);
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = fb->screen_x;
	bmi.bmiHeader.biHeight = -fb->screen_y; //当图像是倒立显示的时候，把biHeight改为对应的负值
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32; //点的大小位数   
	bmi.bmiHeader.biCompression = BI_RGB;
	SetStretchBltMode(hdc, HALFTONE); //防止图像失真
#if 1
	StretchDIBits(hdc, 0, 0, fb->screen_x, fb->screen_y, 0, 0, fb->screen_x, fb->screen_y, fb->fb_tmp, &bmi, DIB_RGB_COLORS, SRCCOPY);
#else
	HDC mdc = CreateCompatibleDC(hdc);
	HBITMAP MyBit = CreateCompatibleBitmap(hdc, 1024, 768);
	SetBitmapBits(MyBit, fb->screen_x*fb->screen_y*4, fb->fb_tmp);
	SelectObject(mdc, MyBit);
	BitBlt(hdc, 0, 0, 1024, 768, mdc, 0, 0, SRCCOPY);
		DeleteObject((HGDIOBJ)mdc);
#endif
	//SetDIBitsToDevice(hdc, 0, 0, fb->screen_x, fb->screen_y, 0, 0, fb->screen_x, fb->screen_y, fb->fb_tmp, &bmi, DIB_RGB_COLORS);
}