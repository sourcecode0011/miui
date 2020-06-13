#include <stdio.h>
#include "interface.h"
#include "fbviewer.h"

#define KEY_P1_V 		1<<0
#define KEY_P2_V		1<<1
#define KEY_FIRE_V 	1<<2
#define KEY_UP_V 		1<<3
#define KEY_DWON_V 	1<<4
#define KEY_LEFT_V 	1<<5
#define KEY_RIGHT_V 	1<<6

#define KEY_VOLUMEUP_V 	1<<7
#define KEY_VOLUMEDOWN_V 	1<<8

static char isInited = 0;

#define NUM 2
struct pitem items[NUM]=
{
		{"/tmp/button_0.png", "/tmp/button_1.png",200,200, KEY_P1_V},
		{"/tmp/button_0.png", "/tmp/button_1.png",700,200, KEY_P2_V},
};


struct Image ti={0};
unsigned short gkey=0;
static char ischange = 0;

void drawitem(int idx, char state)
{
	load_image(state?items[idx].press:items[idx].normal, &ti,items[idx].x,items[idx].y, 0);
	drawrect(&ti);
	if(ti.fbbuffer)free(ti.fbbuffer);
	if(ti.alpha)
		free(ti.alpha);
	ti.fbbuffer = NULL;
	ti.alpha = NULL;
	ti.rgb = NULL;
}
static int init(const char* dir)
{
	if(!isInited)
	{
	int i=0;
printf("=====init test======\n");	
		isInited = 1;
		ischange =1;
	}
	return 0;
}

static int deinit()
{
	return 0;
}

static int onkey(unsigned short code, unsigned int value)
{
#if 1
	if(value==1)
	{
		ischange = 1;
		switch(code)
		{
			case KEY_P1: 	 gkey  |=0x1;break;
			case KEY_P2: 	 gkey  |=0x2;break;
			case KEY_FIRE: 	 gkey  |=0x4;break;
		}
	}
	else if(value==0)
	{
		ischange = 1;
		switch(code)
		{
			case KEY_P1: 	 gkey  &=~0x1;break;
			case KEY_P2: 	 gkey  &=~0x2;break;
			case KEY_FIRE: 	 gkey  &=~0x4;break;
		}
	}
	#endif
	return 0;
}

static int onrender()
{
		int i=0;
	
		drawcolor(0x00000000);
		for(i=0;i< NUM; i++)
		{
			if(gkey&items[i].key)
				drawitem(i, 1);
			else drawitem(i, 0);
		}
	
	return 0;
}

struct page testmode_page =
{
	"test",
	init,
	deinit,
	onkey,
	onrender,
};
