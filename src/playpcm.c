#include <stdio.h>

#include "rthreads/rthreads.h"
#include "data.h"
static char allow = 1;

#ifndef WIN32

#include <alsa/asoundlib.h>
static snd_pcm_t *pcm = NULL;
static struct {
	snd_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;
} hwparams, rhwparams;



//static int mmap_flag = 0;
static int interleaved = 1;
static snd_pcm_uframes_t chunk_size = 1024;
static unsigned period_time = 0;
static unsigned buffer_time = 0;
static snd_pcm_uframes_t period_frames = 50;//4096;
static snd_pcm_uframes_t buffer_frames = 16384>>1;
static int monotonic = 0;

static int can_pause = 0;

static int avail_min = -1;

static size_t  bits_per_sample, bits_per_frame;


static void set_params(void)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_uframes_t buffer_size;
	int err;
	size_t n;
	unsigned int rate;
	snd_pcm_uframes_t start_threshold, stop_threshold;
	snd_pcm_hw_params_alloca(&params);
	//
	err = snd_pcm_hw_params_any(pcm, params);
	if (err < 0) {
		printf("Broken configuration for this PCM: no configurations available\n");
	}
	
	/*if (mmap_flag) {
		snd_pcm_access_mask_t *mask = alloca(snd_pcm_access_mask_sizeof());
		snd_pcm_access_mask_none(mask);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_INTERLEAVED);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_NONINTERLEAVED);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_COMPLEX);
		err = snd_pcm_hw_params_set_access_mask(pcm, params, mask);
	} else */
	if (interleaved)
		err = snd_pcm_hw_params_set_access(pcm, params,
						   SND_PCM_ACCESS_RW_INTERLEAVED);
	else
		err = snd_pcm_hw_params_set_access(pcm, params,
						   SND_PCM_ACCESS_RW_NONINTERLEAVED);
	if (err < 0) {
		printf("Access type not available\n");
	}
	err = snd_pcm_hw_params_set_format(pcm, params, hwparams.format);
	if (err < 0) {
		printf("Sample format non available\n");
		//show_available_sample_formats(params);

	}
	err = snd_pcm_hw_params_set_channels(pcm, params, hwparams.channels);
	if (err < 0) {
		printf("Channels count non available\n");

	}

#if 0
	err = snd_pcm_hw_params_set_periods_min(pcm, params, 2);
	assert(err >= 0);
#endif
	rate = hwparams.rate;
int dir = 1;
	err = snd_pcm_hw_params_set_rate_near
		(pcm, params, &rate, &dir);

	hwparams.rate = rate;
	#if 1
	if (buffer_time == 0 && buffer_frames == 0) {
		err = snd_pcm_hw_params_get_buffer_time_max(params,
							    &buffer_time, 0);
		assert(err >= 0);
		if (buffer_time > 500000)
			buffer_time = 500000;
		
	}
	if (period_time == 0 && period_frames == 0) {
		if (buffer_time > 0)
			period_time = buffer_time / 4;
		else
			period_frames = buffer_frames / 4;
		
	}
	if (period_time > 0)
		err = snd_pcm_hw_params_set_period_time_near(pcm, params,
							     &period_time, 0);
	else
	err = snd_pcm_hw_params_set_period_size_near(pcm, params,
							     &period_frames, 0);
	assert(err >= 0);
	if (buffer_time > 0) {
		err = snd_pcm_hw_params_set_buffer_time_near(pcm, params,
							     &buffer_time, 0);
	} else {
		err = snd_pcm_hw_params_set_buffer_size_near(pcm, params,
							     &buffer_frames);
	}
	assert(err >= 0);
	monotonic = snd_pcm_hw_params_is_monotonic(params);
	can_pause = snd_pcm_hw_params_can_pause(params);
	#endif
	//snd_pcm_hw_params_set_period_size(pcm,params,500,1);
	//snd_pcm_hw_params_set_buffer_size(pcm,params,200);
	
	err = snd_pcm_hw_params(pcm, params);
	if (err < 0) {
		printf("Unable to install hw params:\n");
		//snd_pcm_hw_params_dump(params, log);
		
	}
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
	if (chunk_size == buffer_size) {
		printf("Can't use period equal to buffer size (%lu == %lu)\n",
		      chunk_size, buffer_size);
		
	}
	snd_pcm_sw_params_alloca(&swparams);	
	
	snd_pcm_sw_params_current(pcm, swparams);
	if (avail_min < 0)
		n = chunk_size;
	else
		n = (double) rate * avail_min / 1000000;
	//	n =1024;
	err = snd_pcm_sw_params_set_avail_min(pcm, swparams, n);
		printf("snd_pcm_sw_params_set_avail_min=%d==\n",n);
	snd_pcm_sw_params_set_start_threshold(pcm, swparams, 1);
	//err = snd_pcm_sw_params_set_stop_threshold(pcm, swparams, 1024);

	if (snd_pcm_sw_params(pcm, swparams) < 0) {
		printf("unable to install sw params:\n");
		//snd_pcm_sw_params_dump(swparams, log);
		
	}

	//if (setup_chmap())
		//prg_exit(EXIT_FAILURE);


	bits_per_sample = snd_pcm_format_physical_width(hwparams.format);
	//significant_bits_per_sample = 
	snd_pcm_format_width(hwparams.format);
	bits_per_frame = bits_per_sample * hwparams.channels;
	 
	// fprintf(stderr, "real chunk_size = %i, frags = %i, total = %i\n", chunk_size, setup.buf.block.frags, setup.buf.block.frags * chunk_size);

	printf("bits_per_frame %d \n",bits_per_frame);

//	snd_pcm_nonblock(pcm,0);


	//buffer_frames = buffer_size;	/* for position test */
}
static ssize_t pcm_write(u_char *data, size_t count)
{
	ssize_t r;
	ssize_t result = 0;

	if (count < chunk_size) {
		snd_pcm_format_set_silence(hwparams.format, data + count * bits_per_frame / 8, (chunk_size - count) * hwparams.channels);
		count = chunk_size;
	}
	//data = remap_data(data, count);
	while 
		(count > 0 &&allow) {
		
		r = //writei_func
			snd_pcm_writei(pcm, data, count);
		
	//	if (test_position)
		//	do_test_position();
		#if 0
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
			//if (!test_nowait)
			//	snd_pcm_wait(pcm, 100);
		} else if (r == -EPIPE) {
			xrun();
		} else if (r == -ESTRPIPE) {
			//suspend();
		} else if (r < 0) {
			printf("write error: \n");
			//prg_exit(EXIT_FAILURE);
			
		}
			#else
			 if (r == -EPIPE)printf("=========error============\n");
			if ( r < 0 ) {	
				if ( r == -EAGAIN ) {
					/* Apparently snd_pcm_recover() doesn't handle this case - does it assume snd_pcm_wait() above? */	
					//usleep(100);	
					snd_pcm_wait(pcm, 5);
					
					//snd_pcm_avail_update(pcm);
					continue;	
					
					}			
				r = snd_pcm_recover(pcm, r, 0);		
				if ( r < 0 ) {
					/* Hmm, not much we can do - abort */		
					//fprintf(stderr, "ALSA write failed (unrecoverable): %s\n", SDL_NAME(snd_strerror)(status));	
					//this->enabled = 0;	
					printf("write pcm error %d \n", r);
					return r;			
					}			
				continue;		
			}
			
			#endif
		if (r > 0) {
		
			result += r;
			count -= r;
			data += r * bits_per_frame / 8;
		}
	}
	return result;
}

//		FILE* testfp;
void close_pcm()
{
	if(pcm)snd_pcm_close(pcm);
	pcm = NULL;
}

int game_audio_play(unsigned short*pBuf,int nSize)
{
//int size = nSize<<2;
//if(testfp){fwrite(pBuf, 1, size,testfp);}
#if 1
//	if (pcm_write_tiny(pcm, pBuf, nSize<<2)) {
 //               printf("Error playing sample %d \n", nSize);
//            }
pcm_write( pBuf, nSize<<2* 8 / bits_per_frame);
	#else
        while (fifo_write_avail(g_data.buffer) < size){
		//printf("wait audio play %d \n",size);
		scond_wait(g_data.cond,NULL);//g_data.lock);
		
	}
	
		slock_lock(g_data.lock);
		fifo_write(g_data.buffer, pBuf, size);

		slock_unlock(g_data.lock);
		#endif
		//usleep(1000);
		
	return nSize;
}



int open_pcm()
{
	//close_pcm();
	if(pcm)return 0;
	int ret = snd_pcm_open(&pcm, "default",SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	printf("%s ret %d\n",__FUNCTION__,ret);
	if(ret < 0)
	{close_pcm();	return -1;}
	rhwparams.format = SND_PCM_FORMAT_S16_LE;
	rhwparams.rate = 44100;
	rhwparams.channels = 1;
	hwparams = rhwparams;
	//chunk_size = 1024;
	set_params();
	snd_pcm_start(pcm);
	return 0;

}

int play_wav(const char *filepath)
{
	FILE* fp= fopen(filepath,"rb");
	if(!fp)return -1;
	close_pcm();
	int ret = snd_pcm_open(&pcm, "default",SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	printf("%s ret %d\n",__FUNCTION__,ret);
	if(ret < 0)
		return -1;
	rhwparams.format = SND_PCM_FORMAT_S16_LE;
	rhwparams.rate = 44100;
	rhwparams.channels = 2;
	hwparams = rhwparams;
	//chunk_size = 8192;
	set_params();
	snd_pcm_start(pcm);
	char *buf=(char*)malloc(4096);
	int len=fread(buf,1,44,fp); 
	do{
		if(!allow)break;
		len = fread(buf,1,4096,fp);
		if(len>4)
		{
			game_audio_play(buf,len >>2);
			
		}
	}while(len>0);
	fclose(fp);
	free(buf);
	printf("==end %s \n", filepath);
	close_pcm();
	return 0;
}
#else
void close_pcm()
{

}
int open_pcm()
{
	return 0;
}
int game_audio_play(unsigned short*pBuf, int nSize)
{
	return nSize;
}
int play_wav(const char *filepath)
{
	return 0;
}
#endif
//noise 
void play_pcm()
{
printf("play noise == === \n");


	if(0 != open_pcm())return ;
	
	{
		char *m=(char*)malloc(4096);
		if(m){memset(m,0,4096);
		game_audio_play(m,4096 >>2);
		game_audio_play(m,4096 >>2);
		game_audio_play(kGameData,kGameDataLen >>2);
		
		
		//game_audio_play(m,4096 >>2);
		//game_audio_play(m,4096 >>2);
		free(m);m=NULL;}
	}
	
	//close_pcm();
}
//ding
void play_dingpcm()
{
	printf("play ding ===== \n");

	if(0 != open_pcm())return ;
	game_audio_play(kDingData,kDingDataLen >>2);
	//close_pcm();
}
static sthread_t *mAudiothread = NULL;
static char pcmRunning = 0;
static char needlock = 1;
static char play_nextindex = -1;
static char isruning = 0;
static slock_t *audio_lock=NULL;

static int getNext()
{
	int ret = play_nextindex;
	play_nextindex = -1;
	return ret;
}
static void audio_thread(void* p)
{
	char play_index = -1;

//int ret = (int)p;
//printf("p==%d %x\n",ret,p);
	while(pcmRunning)
	{
		if(needlock&&audio_lock)slock_lock(audio_lock);
		play_index =  getNext();
		needlock = 1;
		if(play_index>=0)
		{
			isruning = 1;
			allow = 1;
			if(play_index == 0)
				play_pcm();
			else if(play_index == 1)
				play_dingpcm();
			else if(play_index == 2)
				play_wav("/tmp/2khz0dbR.wav");
			else if(play_index == 3)
				play_wav("/tmp/1khz0dbL.wav");
			else if(play_index == 4)
				play_wav("/tmp/1khz0dbLR.wav");
			isruning = 0;
		}
		
		
	}
	printf("=exit==audio thread \n");
	close_pcm();

}

void setIndexPcm(int indx)
{
	allow =0;
	play_nextindex = indx;
	if(isruning)needlock=0;
	if(audio_lock)slock_unlock(audio_lock);
}

int init_pcmthread()
{
	pcmRunning = 1;

	if(!mAudiothread)
	{
		mAudiothread = sthread_create(audio_thread, NULL);
		audio_lock = slock_new();
	}

}
int deinit_pcmthread()
{
	pcmRunning=0;
	allow =0;
	if(audio_lock)
	{
		slock_unlock(audio_lock);
		
	}
	if(mAudiothread) sthread_join(mAudiothread);
			mAudiothread = NULL;
			
	if(audio_lock)
	{
		slock_free(audio_lock);
		audio_lock = NULL;
	}
		

}

