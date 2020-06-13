#ifndef __INTERFACE_H
#define __INTERFACE_H

#define KEY_P1 		310
#define KEY_P2		311
#define KEY_FIRE 	304
#define KEY_UP 		103
#define KEY_DOWN 	108
#define KEY_LEFT 	105
#define KEY_RIGHT 	106

#define KEY_VOLUMEUP 	115
#define KEY_VOLUMEDOWN 	114







struct page
{
const char* name;
int (*init)(const char * dir);
int (*deinit)();
 int (*onkey)(unsigned short code , unsigned int value);
 int (*onrender)();
};

struct pitem
{
	char* normal;
	char* press;
	int x;
	int y;
	unsigned short key;
};

struct games
{
	char* emu;
	char* rom;
};

#endif
