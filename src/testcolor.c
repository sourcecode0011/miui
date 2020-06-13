#include <stdio.h>
#include "interface.h"
#include "fbviewer.h"


static char isInited = 0;


static char ischange = 0;
static int discolor = 0;

static int init(const char* dir)
{
	if(!isInited)
	{
		ischange = 1;

		isInited = 1;
	
	}
	return 0;
}

static int deinit()
{
	return 0;
}

static int onkey(unsigned short code, unsigned int value)
{

	if(value==1)
	{
		ischange = 1;
		discolor++;
	}
	return 0;
}

static int onrender()
{
	
		if(discolor == 0){drawcolor(0xffffffff);/*play_wav("/tmp/t.wav");*/}
		else if(discolor == 1)drawcolor(0x00ff0000);
		else if(discolor == 2)drawcolor(0x0000ff00);
		else if(discolor == 3)drawcolor(0x000000ff);
		else if(discolor == 4)drawcolor(0xff000000);
		
	return 0;
}

struct page testcolor_page =
{
	"color",
	init,
	deinit,
	onkey,
	onrender,
};
