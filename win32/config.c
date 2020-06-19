#include <stdio.h>
#include <stdlib.h>
#include "interface.h"
static int mGamenum, mPreload, mHavesec, mChecksum, mKeynum,mZip=0;
static struct pitem *mKeys=NULL;
static struct games *mGames=NULL;
static char* mCpsav = NULL;
static int parseInt(char* p)
{
	int ret = 0;
	char c;
	char*cp = p;
	for (;;)
	{
		c = *cp;
		if (48 <= c&&c < 58)
		{
			ret *= 10;
			ret += c-'0';
			cp++;
		}
		else break;
	}
	return ret;
}
/*static char* find(char*p, char* sub)
{
	char *ret = NULL;
	int len=strlen(sub);
	
	for (;;)
	{
		if (*p == *sub)
		{
			if (!strncmp(p, sub, len))
			{
				ret = p;
				break;
			}
			p++;
		}
		else
			p++;
	}
	return ret;
}*/
static int  parseGame(char*p, struct games *g)
{
	int ret = 0,len;
	char *line;
	line = strstr(p, "{\"");
	if (line)
	{
		ret = (line - p) + 2;
		p += ret;
		line = strstr(p, "\",\"");
		len = line - p;
		g->emu = (char*)malloc(len+1);
		g->emu[len] = 0;
		strncpy(g->emu,p,len);
		p += len + 3;
		ret += len + 3;
		line = strstr(p, "\"}");
		len = line - p;
		g->rom = (char*)malloc(len + 1);
		g->rom[len] = 0;
		strncpy(g->rom, p, len);
		ret += len + 2;
	}
	return ret;
}
static int parseKeys(char *p, struct pitem *g)
{
	int ret = 0, len;
	char *line;
	line = strstr(p, "{\"");
	if (line)
	{
		ret = (line - p) + 2;
		p += ret;
		line = strstr(p, "\",\"");
		len = line - p;
		g->normal = (char*)malloc(len + 1);
		g->normal[len] = 0;
		strncpy(g->normal, p, len);
		p += len + 3;
		ret += len + 3;
		line = strstr(p, "\",");
		len = line - p;
		g->press = (char*)malloc(len + 1);
		g->press[len] = 0;
		strncpy(g->press, p, len);
		ret += len + 2;
		p += len + 2;
		g->x=parseInt(p);
		line = strstr(p, ",");
		len = (line - p)+1;
		p += len ;
		g->y = parseInt(p);
		line = strstr(p, ",");
		len = (line - p) + 1;
		p += len ;
		g->key = parseInt(p);
		//line = strstr(p, ",");
		//len = (line - p) + 1;
	}
	return ret;
}
static int parseStr(char* p, char** dest)
{
	int ret = 0;
	if (!*dest&&*p == '\"')
	{
		char* line;
		p += 1;
		line = strstr(p, "\"");
		ret = (line - p);
		if (ret > 0)
		{
			*dest = (char*)malloc(ret+1);
			strncpy(*dest, p, ret);
			(*dest)[ret] = 0;
		}
	}
	return ret;
}
int loadconf(const char* path)
{
	int re = 0;
	FILE *fp = fopen(path,"rb");
	if (fp)
	{
		char* buff;
		char*p,*line;
		int len;
		int i,ret;
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buff = (char*)malloc(len);
		fread(buff, 1, len, fp);
		fclose(fp);
		p = buff;
		line = strstr(p, "gamenum");
		if (line)
		{
			p += (line - p)+8;
			mGamenum = parseInt(p);
			
		}
		line = strstr(p, "preload");
		if (line)
		{
			p += (line - p) + 8;
			mPreload = parseInt(p);
		}
		line = strstr(p, "havesec");
		if (line)
		{
			p += (line - p) + 8;
			mHavesec = parseInt(p);
		}
		line = strstr(p, "checksum");
		if (line)
		{
			p += (line - p) + 9;
			mChecksum = parseInt(p);
		}
		line = strstr(p, "keynum");
		if (line)
		{
			p += (line - p) + 7;
			mKeynum = parseInt(p);
		}
		line = strstr(p, "iszip");
		if (line)
		{
			p += (line - p) + 6;
			//p += parseStr(p, &mCpsav);
			mZip = parseInt(p);
		}
		line = strstr(p, "cpsav");
		if (line)
		{
			p += (line - p) + 6;
			p += parseStr(p, &mCpsav);
		}
		
		//games
		line = strstr(p, "games");
		if (line)
		{
			p += (line - p) + 6;
			mGames = (struct games*)malloc(mGamenum*sizeof(*mGames));
			for (i = 0; i < mGamenum; i++)
			{
				ret = parseGame(p, &mGames[i]);
				printf("%s = %s \n", mGames[i].emu, mGames[i].rom);
				p += ret;
			}
		}
		line = strstr(p, "keys");
		if (line)
		{
			p += (line - p) + 5;
			mKeys = (struct pitem*)malloc(mKeynum*sizeof(*mKeys));
			for (i = 0; i < mKeynum; i++)
			{
				ret = parseKeys(p, &mKeys[i]);
				printf("%s = %s x %d %d  %d\n", mKeys[i].normal, mKeys[i].press, mKeys[i].x, mKeys[i].y, mKeys[i].key);
				p += ret;
			}
		}
		free(buff);
		re = 1;
	}
	return re;
}
void freeconf()
{
	int i;
	if (mKeys)
	{
		for (i = 0; i < mKeynum; i++)
		{
			free(mKeys[i].normal);
			free(mKeys[i].press);
		}
		free(mKeys);
	}
	if (mGames)
	{
		for (i = 0; i < mGamenum; i++)
		{
			free(mGames[i].emu);
			free(mGames[i].rom);
		}
		free(mGames);
	}
	if (mCpsav)free(mCpsav);
}

int getGameNum(){ return mGamenum; }
int getPreload(){ return mPreload; }
int getHavesec(){ return mHavesec; }
int getChecksum(){ return mChecksum; }
int getKeynum(){ return mKeynum; }
char* getCpsav(){ return mCpsav; };
int getIsZip(){ return mZip; }
struct pitem *getKeys(){
	return mKeys;
}
struct games *getGames(){
	return mGames;
}