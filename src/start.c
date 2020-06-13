/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifdef WIN32
#include <stdio.h>
#include <windows.h>
#include "unzip.h"

#define EV_KEY 1
#else
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#endif
#include "interface.h"

#include "fbviewer.h"

#include <time.h>
#include "rthreads/rthreads.h"



extern struct page testmode_page;
extern struct page  testcolor_page;


#define GAME_NUM	2

unsigned long timeout2play=0;
#ifndef WIN32
unsigned long GetTickCount()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#endif

struct Image image[GAME_NUM]={0};
struct Image game_prev_bmap[GAME_NUM]={0};

void preloadinit(const char* dir,int p,  int idx)
{
//	struct Image image={0};
	int i;
	char path[128]={0};
	sprintf(path,"%s/res/%d.png",dir,p);printf("%s =%d path %s\n",__FILE__,__LINE__, path);
	load_image(path,  &image[idx],0,0,1);
	/*sprintf(path,"%s/res/%d.png",dir,1);
	preload_image(path, &g_fb, &image[1]);
	sprintf(path,"%s/res/%d.png",dir,10);
	preload_image(path, &g_fb, &image[2]);
	sprintf(path,"%s/res/%d.png",dir,11);
	preload_image(path, &g_fb, &image[3]);*/
	//if (preload_image(path, &g_fb, &image) != 0)
	//	goto exit;
	
	
}
void showpage(const char* dir,int i)
{
	
	printf("%s =%d \n",__FILE__,__LINE__);
	if(0==i)
	drawfullscreen(&image[0]);
	else if(1==i)
	drawfullscreen(&image[1]);
	else if(2==i)
	drawfullscreen(&image[2]);
	else if(3==i)
	drawfullscreen(&image[3]);
	else if(10==i)
	drawfullscreen(&game_prev_bmap[0]);
	else if(11==i)
	drawfullscreen(&game_prev_bmap[1]);
	else if(12==i)
	drawfullscreen(&game_prev_bmap[2]);
	else if(13==i)
	drawfullscreen(&game_prev_bmap[3]);
	//exit:
	//	printf("%s =%d \n",__FILE__,__LINE__);
//	free(image.fbbuffer);
//	image.fbbuffer = NULL;
	printf("%s =%d \n",__FILE__,__LINE__);
}

static struct page *pagemode=NULL;


static int disablekey =0,prepaint = 0,startgame=-1;
static  int curvlomue=25;

static void autoSendKey(char dat)//'1' open '0' close
{
	FILE *fp=fopen("/proc/keynode","wb");
						
	if(fp)
	{
	 	fwrite(&dat,1,1,fp);
		fclose(fp);
	}
}
static char testmodeKey()
{
	FILE*fp=fopen("/proc/keynode","rb");
						
	if(fp)
	{
		unsigned int k=0;
		unsigned char buf[6]={0};
	 	int ret = fread(buf,1,6,fp);
		fclose(fp);
		
		k =  (buf[0]-'0')+(buf[1]-'0')*10+(buf[2]-'0')*100+(buf[3]-'0')*1000+(buf[4]-'0')*10000;
	//	printf("read key %u ret %d =%x %x %x %x \n",k,ret,buf[0],buf[1],buf[2],buf[3]);
		if(k&0x0D)
			return 1;
	}
	return 0;
}

static int readVolume()
{
	FILE*fp=fopen("/data/spk_volume","rb"); 
	int vol=25,len;
	
	if(fp)
	{	
		char buff[3]={0};
		len = fread(buff,1,2,fp);
		//printf("v %s =%d\n", buff,len);
		fclose(fp);
		if(len>0)
		{
			if(len==1)vol=buff[0]-'0';
			else
			{
				vol=10*(buff[0]-'0');
				vol+=(buff[1]-'0');
			}
		}
	}
	//printf("vol %d \n",vol);
	if(vol<0)vol = 0;
	else if(vol>31)vol=31;
	return vol;
}
static void writeVolume(int vol)
{
	FILE*fp=fopen("/data/spk_volume","wb");	
	char buff[128]={0};
	if(fp)
	{
		if(vol>9)
		{
			 buff[0]='0'+(vol/10);
			  buff[1]='0'+(vol%10);
			  fwrite(buff,1,2,fp);
		}
		else{ 
			buff[0]='0'+vol;
			fwrite(buff,1,1,fp);
			}
	 	
		fclose(fp);
	}
	sprintf(buff,"amixer cset numid=3,iface=MIXER,name='Lineout volume' %d",vol);
	system(buff);
}
char isSecondpage = 0;
unsigned long prev_time = 0;
int parseKey(unsigned short type, unsigned short code, unsigned int value)
{
	static int   index = 0;
	
	if (type == EV_KEY)
	{
		printf(" key %d = %d \n", code, value);
		if (pagemode)
		{
			int ret = pagemode->onkey(code, value);
			return 0;
		}

		if ((value == 1) && (code == KEY_VOLUMEDOWN || code == KEY_VOLUMEUP))
		{

			if (code == KEY_VOLUMEUP)
			{
				curvlomue += 2;
				if (curvlomue>31)curvlomue = 31;
			}
			else
			{
				curvlomue -= 2;
				if (curvlomue<0)curvlomue = 0;
			}

			writeVolume(curvlomue);
		}
		if (disablekey)
			return 0;

		if (value == 0)
		{
			if (isSecondpage == 1)
			{
				if (KEY_P1 == code)
				{
					//close_audio();
					deinit_pcmthread();
					disablekey = 1;
					//	close(fd);
					isSecondpage = 0;
					startgame = index;
					prepaint = index;
					//showpage(pwd, index);
					//fd = open("/dev/input/event0", O_RDONLY);
					//FD_SET(fd, &rdfs);
				}
				return 0;
			}


			//	else 
			if (KEY_FIRE == code || (isSecondpage != 0 && KEY_P1 == code))
			{
				if (isSecondpage == 0)
					isSecondpage = 1;
				else isSecondpage = 0;
			}
			else if (KEY_P2 == code)
			{

				//pagemode = &testmode_page;
				//pagemode = &testcolor_page;
			}
		}
		else if (value == 1)
		{
			if (code != KEY_VOLUMEDOWN&&code != KEY_VOLUMEUP)
			{
				if (isSecondpage == 0)
				{
					if (code == KEY_UP || code == KEY_DOWN || code == KEY_LEFT || code == KEY_RIGHT)setIndexPcm(0);
					else if (code == KEY_FIRE)setIndexPcm(1);
				}
				else if (isSecondpage == 1)
				{
					if (code == KEY_P1)setIndexPcm(1);
				}
			}
			if (KEY_FIRE == code)
			{

				if (isSecondpage == 0)
				{
					//isSecondpage  = 1;
					//showpage(pwd, 10+index);
					prepaint = 10 + index;
					//prepaint = index;
				}

			}
			else if (KEY_P1 == code)
				prev_time = GetTickCount();
			else if (isSecondpage == 0)
			{
				if (code == KEY_DOWN)
				{
					index++;
					if (index >= GAME_NUM)
						index = 0;
					//showpage(pwd,index);
					prepaint = index;
				}
				else if (code == KEY_UP)
				{
					index--;
					if (index < 0)index = GAME_NUM - 1;
					//showpage(pwd, index);
					prepaint = index;
				}
			}
		}
		else if (value == 2)
		{
			if (KEY_P1 == code &&isSecondpage == 1)
			{
				printf("tick %u \n", GetTickCount() - prev_time);
				if ((GetTickCount() - prev_time) > 4 * 1000)
				{
					isSecondpage = -1;
					//showpage(pwd, index);
					prepaint = index;
				}
			}
		}
	}
	return 0;
}
#ifdef WIN32
void winKey(int code, unsigned int value)
{
	unsigned short cd=0;
	static int prevkey = 0;
	

	switch (code)
	{
		case VK_LEFT:cd = KEY_LEFT; break;
		case VK_RIGHT:cd = KEY_RIGHT; break;
		case VK_UP:cd = KEY_UP; break;
		case VK_DOWN:cd = KEY_DOWN; break;

		case VK_NUMPAD0:cd = KEY_FIRE; break;
		case VK_NUMPAD1:cd = KEY_P1; break;
		case VK_NUMPAD2:cd = KEY_P2; break;
	}
	
	if (value == 0)
		prevkey = 0;
	else if (prevkey)value = 2;
	else 
		prevkey = cd;
	printf("code %d = %d == value %d \n", code, cd, value);
	parseKey(EV_KEY, cd, value);
}
#else
static void key_thread(void *p)
{
	struct input_event events[64];
	int fd,len,i;
	fd_set rdfs;	
	fd = open("/dev/input/event0", O_RDONLY);
	FD_ZERO(&rdfs);
	FD_SET(fd, &rdfs);
	if(fd < 1){return;}
	while(1)
	{
		select(fd + 1, &rdfs, NULL, NULL, NULL);
      	
		len = read(fd, events, sizeof(events));
  		//printf("le n %d \n", len);
		if(len < (int) sizeof(struct input_event))
			break;
		
         len /= sizeof(*events);
         for (i = 0; i < len; i++)
         {
            uint16_t type = events[i].type;
            uint16_t code = events[i].code;
            uint32_t value = events[i].value;

			//timeout2play = GetTickCount();
		printf( " key %d = %u y %d\n", code ,value, type );
		#if 1
		//if((value&2) &&(EV_KEY ==type))continue;
			parseKey(type, code ,value);
			#endif
	     }
	}
		 close(fd);
}
#endif


int do_list(unzFile uf)

{
	uLong i;
	unz_global_info64 gi;
	int err;

	err = unzGetGlobalInfo64(uf,&gi);
	if (err!=UNZ_OK)
		printf("error %d with zipfile in unzGetGlobalInfo \n",err);
	printf("  Length  Method     Size Ratio   Date    Time   CRC-32     Name\n");
	printf("  ------  ------     ---- -----   ----    ----   ------     ----\n");
	for (i=0;i<gi.number_entry;i++)
	{
		char filename_inzip[256];
		unz_file_info64 file_info;
		uLong ratio=0;
		const char *string_method;
		char charCrypt=' ';
		err = unzGetCurrentFileInfo64(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
		if (err!=UNZ_OK)
		{
			printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
			break;
		}
		if (file_info.uncompressed_size>0)
			ratio = (uLong)((file_info.compressed_size*100)/file_info.uncompressed_size);

		/* display a '*' if the file is crypted */
		if ((file_info.flag & 1) != 0)
			charCrypt='*';

		if (file_info.compression_method==0)
			string_method="Stored";
		else
			if (file_info.compression_method==Z_DEFLATED)
			{
				uInt iLevel=(uInt)((file_info.flag & 0x6)/2);
				if (iLevel==0)
					string_method="Defl:N";
				else if (iLevel==1)
					string_method="Defl:X";
				else if ((iLevel==2) || (iLevel==3))
					string_method="Defl:F"; /* 2:fast , 3 : extra fast*/
			}
			else
				if (file_info.compression_method==Z_BZIP2ED)
				{
					string_method="BZip2 ";
				}
				else
					string_method="Unkn. ";

		printf("uncompressed_size %ll\n",file_info.uncompressed_size);
		printf("  %6s%c",string_method,charCrypt);
		printf("uncompressed_size %ll\n", file_info.compressed_size);
		printf(" %3lu%%  %2.2lu-%2.2lu-%2.2lu  %2.2lu:%2.2lu  %8.8lx   %s\n",
			ratio,
			(uLong)file_info.tmu_date.tm_mon + 1,
			(uLong)file_info.tmu_date.tm_mday,
			(uLong)file_info.tmu_date.tm_year % 100,
			(uLong)file_info.tmu_date.tm_hour,(uLong)file_info.tmu_date.tm_min,
			(uLong)file_info.crc,filename_inzip);
		if ((i+1)<gi.number_entry)
		{
			err = unzGoToNextFile(uf);
			if (err!=UNZ_OK)
			{
				printf("error %d with zipfile in unzGoToNextFile\n",err);
				break;
			}
		}
	}

	return 0;
}


#define WRITEBUFFERSIZE (8192)


int do_extract_currentfile(unzFile uf, const char* popt_extract_without_path, int *popt_overwrite, FILE *crcfp)
{
	char filename_inzip[128];
	char* filename_withoutpath;
	char* p;
	int err=UNZ_OK;
	FILE *fout=NULL;
	void* buf;
	uInt size_buf;

	unz_file_info64 file_info;
	uLong ratio=0;
	err = unzGetCurrentFileInfo64(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);

	if (err!=UNZ_OK)
	{
		printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
		return err;
	}

	if (crcfp){
		fwrite(&(file_info.crc), 1, 4, crcfp);
	}
	size_buf = WRITEBUFFERSIZE;
	buf = (void*)malloc(size_buf);
	if (buf==NULL)
	{
		printf("Error allocating memory\n");
		return UNZ_INTERNALERROR;
	}

	p = filename_withoutpath = filename_inzip;
/*	while ((*p) != '\0')
	{
		if (((*p)=='/') || ((*p)=='\\'))
			filename_withoutpath = p+1;
		p++;
	}*/

	//if ((*filename_withoutpath)=='\0')
	{
		/*if ((*popt_extract_without_path)==0)
		{
			printf("creating directory: %s\n",filename_inzip);
			mymkdir(filename_inzip);
		}*/
	}
	//else
	{
		char write_filename[128] = {0};
		int skip=0;

		if (popt_extract_without_path)
			sprintf(write_filename, "%s%s", popt_extract_without_path, filename_inzip);
		else
			sprintf(write_filename, "%s",  filename_inzip);

		err = unzOpenCurrentFilePassword(uf,NULL);
		if (err!=UNZ_OK)
		{
			printf("error %d with zipfile in unzOpenCurrentFilePassword\n",err);
		}

		/*if (((*popt_overwrite)==0) && (err==UNZ_OK))
		{
			char rep=0;
			FILE* ftestexist;
			ftestexist = FOPEN_FUNC(write_filename,"rb");
			if (ftestexist!=NULL)
			{
				fclose(ftestexist);
				do
				{
					char answer[128];
					int ret;

					printf("The file %s exists. Overwrite ? [y]es, [n]o, [A]ll: ", write_filename);
					ret = scanf("%1s", answer);
					if (ret != 1)
					{
						exit(EXIT_FAILURE);
					}
					rep = answer[0];
					if ((rep >= 'a') && (rep <= 'z'))
						rep -= 0x20;
				}
				while ((rep != 'Y') && (rep != 'N') && (rep != 'A'));
			}

			if (rep == 'N')
				skip = 1;

			if (rep == 'A')
				*popt_overwrite = 1;
		}
		*/
		if (/*(skip == 0) && */(err == UNZ_OK))
		{
			fout = fopen(write_filename, "wb");
			/* some zipfile don't contain directory alone before file */
			/*if ((fout == NULL) && ((*popt_extract_without_path) == 0) &&
				(filename_withoutpath != (char*)filename_inzip))
			{
				char c = *(filename_withoutpath - 1);
				*(filename_withoutpath - 1) = '\0';
				makedir(write_filename);
				*(filename_withoutpath - 1) = c;
				fout = FOPEN_FUNC(write_filename, "wb");
			}
			*/
			if (fout == NULL)
			{
				printf("error opening %s\n", write_filename);
			}
		}

		if (fout != NULL)
		{
			printf(" extracting: %s\n", write_filename);

			do
			{
				err = unzReadCurrentFile(uf, buf, size_buf);
				if (err<0)
				{
					printf("error %d with zipfile in unzReadCurrentFile\n", err);
					break;
				}
				if (err>0)
					if (fwrite(buf, err, 1, fout) != 1)
					{
						printf("error in writing extracted file\n");
						err = UNZ_ERRNO;
						break;
					}
			} while (err>0);
			if (fout)
				fclose(fout);

			/*if (err == 0)
				change_file_date(write_filename, file_info.dosDate,
				file_info.tmu_date);*/
		}

		if (err == UNZ_OK)
		{
			err = unzCloseCurrentFile(uf);
			if (err != UNZ_OK)
			{
				printf("error %d with zipfile in unzCloseCurrentFile\n", err);
			}
		}
		else
			unzCloseCurrentFile(uf); /* don't lose the error */
	}

	free(buf);
	return err;
}


int do_extract(unzFile uf, const char *outdir, int opt_overwrite,const char* password)
{
	uLong i;
	unz_global_info64 gi;
	int err;
	FILE* fout = NULL;
	FILE*crcfp;
	char crcpath[64] = {0};
	sprintf(crcpath, "%s/crc.txt", outdir);
	crcfp = fopen(crcpath,"wb");
	err = unzGetGlobalInfo64(uf, &gi);
	if (err != UNZ_OK)
		printf("error %d with zipfile in unzGetGlobalInfo \n", err);

	for (i = 0; i<gi.number_entry; i++)
	{
		if (do_extract_currentfile(uf, outdir,
			&opt_overwrite,
			crcfp) != UNZ_OK)
			break;

		if ((i + 1)<gi.number_entry)
		{
			err = unzGoToNextFile(uf);
			if (err != UNZ_OK)
			{
				printf("error %d with zipfile in unzGoToNextFile\n", err);
				break;
			}
		}
	}
	if (crcfp)fclose(crcfp);
	return 0;
}

void unzlib()
{
#if 1

	static char* zipfilename = "game.zip";
	unzFile uf = NULL;
#        ifdef USEWIN32IOAPI
	zlib_filefunc64_def ffunc;
#        endif

	char buffer[8192];
	uLong crc;
	int len;
	/*uLong crc = crc32(0L, Z_NULL, 0);


	FILE *fp = fopen("data/PacMan.rom","rb");
	while ((len = fread(buffer, 1,8192,fp))>0) {
	crc = crc32(crc, buffer, len);
	}
	fclose(fp);
	printf("%u \n", crc);*/
	FILE* crcfp = fopen("data/crc.txt", "rb");
	if(crcfp)
	{
		int i = 0;
		len = fread(buffer, 1, 8192, crcfp);
		for (i = 0; i < len; i += 4)
		{
			memcpy(&crc, buffer + i, 4);
			printf("%x \n", crc);
		}
		fclose(crcfp);
	}
	/* strncpy doesnt append the trailing NULL, of the string is too long. */
	
#        ifdef USEWIN32IOAPI
	fill_win32_filefunc64A(&ffunc);
	uf = unzOpen2_64(zipfilename, &ffunc);
#        else
	uf = unzOpen64(zipfilename);
#        endif



if (uf == NULL)
{
	printf("Cannot open %s or %s.zip\n", zipfilename, zipfilename);
	return 1;
}

 //do_list(uf);
do_extract(uf, "data/", 1, NULL);
/*else if (opt_do_extract == 1)
{
#ifdef _WIN32
	if (opt_extractdir && _chdir(dirname))
#else
	if (opt_extractdir && chdir(dirname))
#endif
	{
		printf("Error changing into %s, aborting\n", dirname);
		exit(-1);
	}

	if (filename_to_extract == NULL)
		ret_value = do_extract(uf, opt_do_extract_withoutpath, opt_overwrite, password);
	else
		ret_value = do_extract_onefile(uf, filename_to_extract, opt_do_extract_withoutpath, opt_overwrite, password);
}
*/
unzClose(uf);
#endif
}

int appInit()
{
	loadconf("conf.txt");
	if(0 != initfb())return -1;
	unzlib();
	return 0;
}

void appDeinit()
{
	int i;
	deinitfb();
	//free images
	for(i=0;i<GAME_NUM;i++)
	{
		if(image[i].fbbuffer)free(image[i].fbbuffer);
		if(image[i].alpha)free(image[i].alpha);
	}
	freeconf();
}
int appRun()
{
	struct Image ti = { 0 };
	preshowpage("./res/second.png");
	load_image("./res/v.png", &ti, 500, 50, 0);
	drawrect(&ti);
	if (ti.fbbuffer)free(ti.fbbuffer);
	if (ti.alpha)
		free(ti.alpha);

	gr_color(0xff, 0xff, 0xff, 0xff);//rgb a
	draw_text("=====test run to write data!==========", -1, -1, 1);

	return 0;
}
#ifdef WIN32

int appRepaint(HDC hdc)
{
	win_displayframe( hdc);
	return 0;
}

#else
int main(int argc, char **argv)
{

	sthread_t *mkeythread;

	int index=0;
	int i,len;		

	char pwd[128]={0};
	char isSecondpage =0;
	unsigned long prev_time=0;	
	fprintf(stderr,"===============start=init=ui====================\n");
	
	if(appInit() != 0)return 0;
	
	if(argc<2||!argv[1]/*getcwd(pwd,128)*/){
		strcpy(pwd, "/etc");
	}
	else 
		strcpy(pwd, argv[1]);
	
	#if 0 //
	FILE*fp=NULL;
	extern unsigned char kDingData[96798];
	i =0;
	gr_color(0xff,  0xff, 0xff,0xff);//rgb a
	draw_text("=====test run to write data!==========", -1, -1,0);
	displayframe();
	int tcon = 0;
	char testbuff[128]={0};
		while(1)
		{
		tcon++;
			if(!fp)fp = fopen("/data/testwrite","wb");
			if(fp)
			{
			if(tcon%30==0){
			clear_screen();gr_color(0xff,  0xff, 0xff,0xff);//rgb a
			sprintf(testbuff,"=test run to write right!==%d", tcon);
			draw_text(testbuff, -1, -1,0);
			displayframe();}
				len = fwrite(kDingData+i,1,1024,fp);
				//printf("write %d ",len);
				if(len > 0)
				{	i+=len;
					if(i>90*1024){fclose(fp);fp=NULL;i=0;}
				}
				else{fclose(fp);fp=NULL;i=0;}
			}
			else
			{if(tcon %30){clear_screen();gr_color(0xff,  0xff, 0xff,0xff);//rgb a
			sprintf(testbuff,"=test run to write right!==%d", tcon);
				draw_text(testbuff, -1, -1,0);
				displayframe();tcon=0;}
			}
		}
	#else
	preshowpage("/etc/res/second.png");
	//usleep(30000);
	preloadinit(pwd,0,0);
	
	preshowpage("/etc/res/mask.png");

	preloadinit(pwd,1,1);
	preloadinit(pwd,10,2);
	preloadinit(pwd,11,3);
	

	//usleep(30000);
	showpage(pwd,index);
	//fb_show(&image[index], &g_fb);
	//draw_png("/tmp/d.png", &g_fb,0,0);
	displayframe();
	fprintf(stderr,"==========run==========\n");
	mkeythread = sthread_create(key_thread, NULL);
	curvlomue = readVolume();
	writeVolume(curvlomue);
	if(testmodeKey())
		pagemode = &testcolor_page;
	init_pcmthread();
	
	
	
	while(1)
	{
		if(pagemode)
		{
			pagemode->init(pwd);
			pagemode->onrender();
		}
		else
		{
			if(prepaint != index)
			{
			
				index = prepaint;
				showpage(pwd,index);
				printf( " index: %d\n", index );
			
				
			}
			if(startgame != -1)
			{
				char tmppath[128]={0};
			drawcolor(0);
			displayframe();
			displayframe();
					if(startgame == 0)
						sprintf(tmppath,"%s/start.sh 0", pwd);
					else 
						sprintf(tmppath,"%s/start.sh 1", pwd);
					autoSendKey('1');
					usleep(33*1000);
					system(tmppath);
					autoSendKey('0');
					startgame = -1;
					disablekey = 0;
					
					showpage(pwd,prepaint);
					init_pcmthread();
			}
		}

		//======flush=========
		displayframe();
		//printf("flush \n");
		usleep(33*1000);
	/*	if(timeout2play == 0)timeout2play = GetTickCount();
		if((GetTickCount() - timeout2play) > 60*1000)
		{
			startgame =prepaint&0x01;
			timeout2play = 0;
		}*/
	}
	if(mkeythread) sthread_join(mkeythread);
	deinit_pcmthread();

	appDeinit();
	//...
	#endif
	printf("=====app exit=====\n");
return 0;
}

#endif