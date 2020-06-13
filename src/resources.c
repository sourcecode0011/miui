/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif
/*#include <memory>
#include <regex>
#include <string>
#include <vector>
#include <stdarg.h>*/

//#include <android-base/stringprintf.h>
//#include <android-base/strings.h>
#include <png.h>

#include "miui.h"

#define SURFACE_DATA_ALIGNMENT 8

static GRSurface* malloc_surface(size_t data_size) {
    size_t size = sizeof(GRSurface) ;//+ data_size + SURFACE_DATA_ALIGNMENT;
   // unsigned char* temp = (unsigned char*)(malloc(size));
   // if (temp == NULL) return NULL;
    GRSurface* surface = (GRSurface*)malloc(size);
   if(!surface)return NULL;
    surface->data =(unsigned char*) malloc(data_size);
		//temp + sizeof(GRSurface) +
       // (SURFACE_DATA_ALIGNMENT - (sizeof(GRSurface) % SURFACE_DATA_ALIGNMENT));
    return surface;
}
#if 1
typedef struct{
	png_structp png_ptr_;
  png_infop info_ptr_;
  png_uint_32 width_;
  png_uint_32 height_;
  png_byte channels_;
   FILE *png_fp_;
}pnghandle;
static int loadpngres(pnghandle* handle, const char * name)
{
	handle->png_fp_ = fopen(name, "rb");//"rbe");
 if(!handle->png_fp_)
 	return -1;
 	


  unsigned char header[8];
  size_t bytesRead = fread(header, 1, sizeof(header), handle->png_fp_);
  if (bytesRead != sizeof(header)) {
    return -2;
  }

  if (png_sig_cmp(header, 0, sizeof(header))) {
    return -3;
  }

  handle->png_ptr_ = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!handle->png_ptr_) {
    return -4;
  }

  handle->info_ptr_ = png_create_info_struct(handle->png_ptr_);
  if (!handle->info_ptr_) {
    return -5;
  }

  if (setjmp(png_jmpbuf(handle->png_ptr_))) {
    return -6;
  }

  png_init_io(handle->png_ptr_, handle->png_fp_);
  png_set_sig_bytes(handle->png_ptr_, sizeof(header));
  png_read_info(handle->png_ptr_, handle->info_ptr_);

  int color_type;
  int bit_depth;
  png_get_IHDR(handle->png_ptr_, handle->info_ptr_, &handle->width_, &handle->height_, &bit_depth, &color_type, nullptr, nullptr,
               nullptr);

  handle->channels_ = png_get_channels(handle->png_ptr_, handle->info_ptr_);

  if (bit_depth == 8 && handle->channels_ == 3 && color_type == PNG_COLOR_TYPE_RGB) {
    // 8-bit RGB images: great, nothing to do.
  } else if (bit_depth <= 8 && handle->channels_ == 1 && color_type == PNG_COLOR_TYPE_GRAY) {
    // 1-, 2-, 4-, or 8-bit gray images: expand to 8-bit gray.
    png_set_expand_gray_1_2_4_to_8(handle->png_ptr_);
  } else if (bit_depth <= 8 && handle->channels_ == 1 && color_type == PNG_COLOR_TYPE_PALETTE) {
    // paletted images: expand to 8-bit RGB.  Note that we DON'T
    // currently expand the tRNS chunk (if any) to an alpha
    // channel, because minui doesn't support alpha channels in
    // general.
    png_set_palette_to_rgb(handle->png_ptr_);
    handle->channels_ = 3;
  } else {
    fprintf(stderr, "minui doesn't support PNG depth %d channels %d color_type %d\n", bit_depth,
            handle->channels_, color_type);
    return -7;
  }
  return 0;
}
static void freepngres(pnghandle* handle)
{
	if(handle->png_fp_)fclose(handle->png_fp_);
	  if (handle->png_ptr_) {
		png_destroy_read_struct(&handle->png_ptr_, &handle->info_ptr_, nullptr);
	  }

}
#else

// This class handles the png file parsing. It also holds the ownership of the png pointer and the
// opened file pointer. Both will be destroyed/closed when this object goes out of scope.
class PngHandler {
 public:
  PngHandler(const char* name);

  ~PngHandler();

  png_uint_32 width() const {
    return width_;
  }

  png_uint_32 height() const {
    return height_;
  }

  png_byte channels() const {
    return channels_;
  }

  png_structp png_ptr() const {
    return png_ptr_;
  }

  png_infop info_ptr() const {
    return info_ptr_;
  }

  int error_code() const {
    return error_code_;
  };

  operator bool() const {
    return error_code_ == 0;
  }

 private:
  png_structp png_ptr_;
  png_infop info_ptr_;
  png_uint_32 width_;
  png_uint_32 height_;
  png_byte channels_;

  // The |error_code_| is set to a negative value if an error occurs when opening the png file.
  int error_code_;
  // After initialization, we'll keep the file pointer open before destruction of PngHandler.
  //std::unique_ptr<FILE, decltype(&fclose)> 
  FILE *png_fp_;
};

static void StringAppendV(std::string* dst, const char* format, va_list ap) {
  // First try with a small fixed size buffer
  char space[1024];

  // It's possible for methods that use a va_list to invalidate
  // the data in it upon use.  The fix is to make a copy
  // of the structure before using it and use that copy instead.
  va_list backup_ap;
  va_copy(backup_ap, ap);
  int result = vsnprintf(space, sizeof(space), format, backup_ap);
  va_end(backup_ap);

  if (result < static_cast<int>(sizeof(space))) {
    if (result >= 0) {
      // Normal case -- everything fit.
      dst->append(space, result);
      return;
    }

    if (result < 0) {
      // Just an error.
      return;
    }
  }

  // Increase the buffer size to the size requested by vsnprintf,
  // plus one for the closing \0.
  int length = result + 1;
  char* buf = new char[length];

  // Restore the va_list before we use it again
  va_copy(backup_ap, ap);
  result = vsnprintf(buf, length, format, backup_ap);
  va_end(backup_ap);

  if (result >= 0 && result < length) {
    // It fit
    dst->append(buf, result);
  }
  delete[] buf;
}

static std::string StringPrintf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  std::string result;
  StringAppendV(&result, fmt, ap);
  va_end(ap);
  return result;
}

PngHandler::PngHandler(const char* name) : error_code_(0), png_fp_(nullptr),png_ptr_(NULL) {
 // std::string res_path = StringPrintf("/usr/share/system/%s.png", name.c_str());
	png_fp_ = fopen(name, "rbe");
 if(!png_fp_)
 {	
 error_code_ = -1;
 return;
 	}
/*  png_fp_.reset(fp);
  if (!png_fp_) {
    error_code_ = -1;
    return;
  }*/

  unsigned char header[8];
  size_t bytesRead = fread(header, 1, sizeof(header), png_fp_/*.get()*/);
  if (bytesRead != sizeof(header)) {
    error_code_ = -2;
    return;
  }

  if (png_sig_cmp(header, 0, sizeof(header))) {
    error_code_ = -3;
    return;
  }

  png_ptr_ = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr_) {
    error_code_ = -4;
    return;
  }

  info_ptr_ = png_create_info_struct(png_ptr_);
  if (!info_ptr_) {
    error_code_ = -5;
    return;
  }

  if (setjmp(png_jmpbuf(png_ptr_))) {
    error_code_ = -6;
    return;
  }

  png_init_io(png_ptr_, png_fp_/*.get()*/);
  png_set_sig_bytes(png_ptr_, sizeof(header));
  png_read_info(png_ptr_, info_ptr_);

  int color_type;
  int bit_depth;
  png_get_IHDR(png_ptr_, info_ptr_, &width_, &height_, &bit_depth, &color_type, nullptr, nullptr,
               nullptr);

  channels_ = png_get_channels(png_ptr_, info_ptr_);

  if (bit_depth == 8 && channels_ == 3 && color_type == PNG_COLOR_TYPE_RGB) {
    // 8-bit RGB images: great, nothing to do.
  } else if (bit_depth <= 8 && channels_ == 1 && color_type == PNG_COLOR_TYPE_GRAY) {
    // 1-, 2-, 4-, or 8-bit gray images: expand to 8-bit gray.
    png_set_expand_gray_1_2_4_to_8(png_ptr_);
  } else if (bit_depth <= 8 && channels_ == 1 && color_type == PNG_COLOR_TYPE_PALETTE) {
    // paletted images: expand to 8-bit RGB.  Note that we DON'T
    // currently expand the tRNS chunk (if any) to an alpha
    // channel, because minui doesn't support alpha channels in
    // general.
    png_set_palette_to_rgb(png_ptr_);
    channels_ = 3;
  } else {
    fprintf(stderr, "minui doesn't support PNG depth %d channels %d color_type %d\n", bit_depth,
            channels_, color_type);
    error_code_ = -7;
  }
}

PngHandler::~PngHandler() {
	if(png_fp_)fclose(png_fp_);
  if (png_ptr_) {
    png_destroy_read_struct(&png_ptr_, &info_ptr_, nullptr);
  }
}

// "display" surfaces are transformed into the framebuffer's required
// pixel format (currently only RGBX is supported) at load time, so
// gr_blit() can be nothing more than a memcpy() for each row.  The
// next two functions are the only ones that know anything about the
// framebuffer pixel format; they need to be modified if the
// framebuffer format changes (but nothing else should).

// Allocate and return a GRSurface* sufficient for storing an image of
// the indicated size in the framebuffer pixel format.
static GRSurface* init_display_surface(png_uint_32 width, png_uint_32 height) {
    GRSurface* surface = malloc_surface(width * height * 4);
    if (surface == NULL) return NULL;

    surface->width = width;
    surface->height = height;
    surface->row_bytes = width * 4;
    surface->pixel_bytes = 4;

    return surface;
}

// Copy 'input_row' to 'output_row', transforming it to the
// framebuffer pixel format.  The input format depends on the value of
// 'channels':
//
//   1 - input is 8-bit grayscale
//   3 - input is 24-bit RGB
//   4 - input is 32-bit RGBA/RGBX
//
// 'width' is the number of pixels in the row.
static void transform_rgb_to_draw(unsigned char* input_row,
                                  unsigned char* output_row,
                                  int channels, int width) {
    int x;
    unsigned char* ip = input_row;
    unsigned char* op = output_row;

    switch (channels) {
        case 1:
            // expand gray level to RGBX
            for (x = 0; x < width; ++x) {
                *op++ = *ip;
                *op++ = *ip;
                *op++ = *ip;
                *op++ = 0xff;
                ip++;
            }
            break;

        case 3:
            // expand RGBA to RGBX
            for (x = 0; x < width; ++x) {
                *op++ = *ip++;
                *op++ = *ip++;
                *op++ = *ip++;
                *op++ = 0xff;
            }
            break;

        case 4:
            // copy RGBA to RGBX
            memcpy(output_row, input_row, width*4);
            break;
    }
}

int res_create_display_surface(const char* name, GRSurface** pSurface) {
  *pSurface = nullptr;

  PngHandler png_handler(name);
  if (!png_handler) return png_handler.error_code();

  png_structp png_ptr = png_handler.png_ptr();
  png_uint_32 width = png_handler.width();
  png_uint_32 height = png_handler.height();

  GRSurface* surface = init_display_surface(width, height);
  if (!surface) {
    return -8;
  }

#if defined(RECOVERY_ABGR) || defined(RECOVERY_BGRA)
  png_set_bgr(png_ptr);
#endif

  for (png_uint_32 y = 0; y < height; ++y) {
    std::vector<unsigned char> p_row(width * 4);
    png_read_row(png_ptr, p_row.data(), nullptr);
    transform_rgb_to_draw(p_row.data(), surface->data + y * surface->row_bytes,
                          png_handler.channels(), width);
  }

  *pSurface = surface;

  return 0;
}

int res_create_multi_display_surface(const char* name, int* frames, int* fps,
                                     GRSurface*** pSurface) {
  *pSurface = nullptr;
  *frames = -1;

  PngHandler png_handler(name);
  if (!png_handler) return png_handler.error_code();

  png_structp png_ptr = png_handler.png_ptr();
  png_uint_32 width = png_handler.width();
  png_uint_32 height = png_handler.height();

  *frames = 1;
  *fps = 20;
  png_textp text;
  int num_text;
  if (png_get_text(png_ptr, png_handler.info_ptr(), &text, &num_text)) {
    for (int i = 0; i < num_text; ++i) {
      if (text[i].key && strcmp(text[i].key, "Frames") == 0 && text[i].text) {
        *frames = atoi(text[i].text);
      } else if (text[i].key && strcmp(text[i].key, "FPS") == 0 && text[i].text) {
        *fps = atoi(text[i].text);
      }
    }
    printf("  found frames = %d\n", *frames);
    printf("  found fps = %d\n", *fps);
  }

  int result = 0;
  GRSurface** surface = nullptr;
  if (*frames <= 0 || *fps <= 0) {
    printf("bad number of frames (%d) and/or FPS (%d)\n", *frames, *fps);
    result = -10;
    goto exit;
  }

  if (height % *frames != 0) {
    printf("bad height (%d) for frame count (%d)\n", height, *frames);
    result = -9;
    goto exit;
  }

  surface = static_cast<GRSurface**>(calloc(*frames, sizeof(GRSurface*)));
  if (!surface) {
    result = -8;
    goto exit;
  }
  for (int i = 0; i < *frames; ++i) {
    surface[i] = init_display_surface(width, height / *frames);
    if (!surface[i]) {
      result = -8;
      goto exit;
    }
  }

#if defined(RECOVERY_ABGR) || defined(RECOVERY_BGRA)
  png_set_bgr(png_ptr);
#endif

  for (png_uint_32 y = 0; y < height; ++y) {
    std::vector<unsigned char> p_row(width * 4);
    png_read_row(png_ptr, p_row.data(), nullptr);
    int frame = y % *frames;
    unsigned char* out_row = surface[frame]->data + (y / *frames) * surface[frame]->row_bytes;
    transform_rgb_to_draw(p_row.data(), out_row, png_handler.channels(), width);
  }

  *pSurface = surface;

exit:
  if (result < 0) {
    if (surface) {
      for (int i = 0; i < *frames; ++i) {
        free(surface[i]);
      }
      free(surface);
    }
  }
  return result;
}
#endif
int res_create_alpha_surface(const char* name, GRSurface** pSurface) {
  *pSurface = nullptr;
#if 0
  PngHandler png_handler(name);
  if (!png_handler)
  	return png_handler.error_code();
 

  if (png_handler.channels() != 1) {
    return -7;
  }

  png_structp png_ptr = png_handler.png_ptr();
  png_uint_32 width = png_handler.width();
  png_uint_32 height = png_handler.height();
  #else
  pnghandle hand={0};
 int ret = loadpngres(&hand, name);
 if(ret != 0)
 {
 	freepngres(&hand);
 	return ret;
 }
if(hand.channels_ != 1){
 	freepngres(&hand);
 	return -7;
 }
  png_structp png_ptr = hand.png_ptr_;
  png_uint_32 width = hand.width_;
  png_uint_32 height = hand.height_;
  #endif

  GRSurface* surface = malloc_surface(width * height);
  if (!surface) {
  	freepngres(&hand);
    return -8;
  }
  surface->width = width;
  surface->height = height;
  surface->row_bytes = width;
  surface->pixel_bytes = 1;

#if defined(RECOVERY_ABGR) || defined(RECOVERY_BGRA)
  png_set_bgr(png_ptr);
#endif

  for (png_uint_32 y = 0; y < height; ++y) {
    unsigned char* p_row = surface->data + y * surface->row_bytes;
    png_read_row(png_ptr, p_row, nullptr);
  }

  *pSurface = surface;
  freepngres(&hand);

  return 0;
}
#if 0
// This function tests if a locale string stored in PNG (prefix) matches
// the locale string provided by the system (locale).
bool matches_locale(const std::string& prefix, const std::string& locale) {
  // According to the BCP 47 format, A locale string may consists of:
  // language-{extlang}-{script}-{region}-{variant}
  // The locale headers in PNG mostly consist of language-{region} except for sr-Latn, and some
  // android's system locale can have the format language-{script}-{region}.

  // Return true if the whole string of prefix matches the top part of locale. Otherwise try to
  // match the locale string without the {script} section.
  // For instance, prefix == "en" matches locale == "en-US", prefix == "sr-Latn" matches locale
  // == "sr-Latn-BA", and prefix == "zh-CN" matches locale == "zh-Hans-CN".
  if (strncmp(locale.c_str(), prefix.c_str(), strlen(prefix.c_str())) == 0) {
    return true;
  }

  size_t separator = prefix.find('-');
  if (separator == std::string::npos) {
    return false;
  }
  std::regex loc_regex(prefix.substr(0, separator) + "-[A-Za-z]*" + prefix.substr(separator));
  return std::regex_match(locale, loc_regex);
}

std::vector<std::string> get_locales_in_png(const std::string& png_name) {
  PngHandler png_handler(png_name.c_str());
  if (!png_handler) {
    printf("Failed to open %s, error: %d\n", png_name.c_str(), png_handler.error_code());
    return {};
  }
  if (png_handler.channels() != 1) {
    printf("Expect input png to have 1 data channel, this file has %d\n", png_handler.channels());
    return {};
  }

  std::vector<std::string> result;
  std::vector<unsigned char> row(png_handler.width());
  for (png_uint_32 y = 0; y < png_handler.height(); ++y) {
    png_read_row(png_handler.png_ptr(), row.data(), nullptr);
    int h = (row[3] << 8) | row[2];
    std::string loc(reinterpret_cast<char*>(&row[5]));
    if (!loc.empty()) {
      result.push_back(loc);
    }
    for (int i = 0; i < h; ++i, ++y) {
      png_read_row(png_handler.png_ptr(), row.data(), nullptr);
    }
  }

  return result;
}

int res_create_localized_alpha_surface(const char* name,
                                       const char* locale,
                                       GRSurface** pSurface) {
  *pSurface = nullptr;
  if (locale == nullptr) {
    return 0;
  }

  PngHandler png_handler(name);
  if (!png_handler) return png_handler.error_code();

  if (png_handler.channels() != 1) {
    return -7;
  }

  png_structp png_ptr = png_handler.png_ptr();
  png_uint_32 width = png_handler.width();
  png_uint_32 height = png_handler.height();

  for (png_uint_32 y = 0; y < height; ++y) {
    std::vector<unsigned char> row(width);
    png_read_row(png_ptr, row.data(), nullptr);
    int w = (row[1] << 8) | row[0];
    int h = (row[3] << 8) | row[2];
    int len = row[4];
    char* loc = reinterpret_cast<char*>(&row[5]);

    if (y + 1 + h >= height || matches_locale(loc, locale)) {
      printf("  %20s: %s (%d x %d @ %d)\n", name, loc, w, h, y);

      GRSurface* surface = malloc_surface(w * h);
      if (!surface) {
        return -8;
      }
      surface->width = w;
      surface->height = h;
      surface->row_bytes = w;
      surface->pixel_bytes = 1;

      for (int i = 0; i < h; ++i, ++y) {
        png_read_row(png_ptr, row.data(), nullptr);
        memcpy(surface->data + i * w, row.data(), w);
      }

      *pSurface = surface;
      break;
    }

    for (int i = 0; i < h; ++i, ++y) {
      png_read_row(png_ptr, row.data(), nullptr);
    }
  }

  return 0;
}
#endif
void res_free_surface(GRSurface* surface) {
	if(surface)
	{
	if(surface->data)free(surface->data);
	  free(surface);
	}
}
