//
//  DPXImage.c
//  QLDPX
//
//  Created by Thomas Angarano on 12/01/2019.
//  Copyright © 2019 Thomas Angarano. All rights reserved.
//

#include <math.h>
#include <CoreFoundation/CoreFoundation.h>

#include "DPXImage.h"

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

typedef struct file_information
{
  UInt32 magic_num;      // magic number 0x53445058 (SDPX) or 0x58504453 (XPDS)
  UInt32 offset;         // offset to image data in bytes
  char vers[8];          // which header format version is being used (v1.0)
  UInt32 file_size;      // file size in bytes
  UInt32 ditto_key;      // read time short cut - 0 = same, 1 = new
  UInt32 gen_hdr_size;   // generic header length in bytes
  UInt32 ind_hdr_size;   // industry header length in bytes
  UInt32 user_data_size; // user-defined data length in bytes
  char file_name[100];   // image file name
  char create_time[24];  // file creation date "yyyy:mm:dd:hh:mm:ss:LTZ"
  char creator[100];     // file creator's name
  char project[200];     // project name
  char copyright[200];   // right to use or copyright info
  UInt32 key;            // encryption ( FFFFFFFF = unencrypted )
  char Reserved[104];    // reserved field TBD (need to pad)
} FileInformation;

typedef struct _image_information
{
  UInt16 orientation;         // image orientation */
  UInt16 element_number;      // number of image elements */
  UInt32 pixels_per_line;     // or x value */
  UInt32 lines_per_image_ele; // or y value, per element */
  struct _image_element
  {
    UInt32 data_sign;          // data sign (0 = unsigned, 1 = signed ) Note: "Core set images are unsigned"
    UInt32 ref_low_data;       // reference low data code value
    int ref_low_quantity;      // reference low quantity represented
    UInt32 ref_high_data;      // reference high data code value
    int ref_high_quantity;     // reference high quantity represented
    UInt8 descriptor;          // descriptor for image element
    UInt8 transfer;            // transfer characteristics for element
    UInt8 colorimetric;        // colormetric specification for element
    UInt8 bit_size;            // bit size for element
    UInt16 packing;            // packing for element
    UInt16 encoding;           // encoding for element
    UInt32 data_offset;        // offset to data of element
    UInt32 eol_padding;        // end of line padding used in element
    UInt32 eo_image_padding;   // end of image padding used in element
    char description[32];      // description of element
  } image_element[8];          // NOTE THERE ARE EIGHT OF THESE

  UInt8 reserved[52];          // reserved for future use (padding)
} ImageInformation;

typedef struct _image_orientation
{
  UInt32 x_offset;         // X offset
  UInt32 y_offset;         // Y offset
  int x_center;            // X center
  int y_center;            // Y center
  UInt32 x_orig_size;      // X original size
  UInt32 y_orig_size;      // Y original size
  char file_name[100];     // source image file name
  char creation_time[24];  // source image creation date and time
  char input_dev[32];      // input device name
  char input_serial[32];   // input device serial number
  UInt16 border[4];        // border validity (XL, XR, YT, YB)
  UInt32 pixel_aspect[2];  // pixel aspect ratio (H:V)
  UInt8 reserved[28];      // reserved for future use (padding)
} ImageOrientation;

typedef struct _motion_picture_film_header
{
  char film_mfg_id[2];    // film manufacturer ID code (2 digits from film edge code)
  char film_type[2];      // file type (2 digits from film edge code)
  char offset[2];         // offset in perfs (2 digits from film edge code)
  char prefix[6];         // prefix (6 digits from film edge code)
  char count[4];          // count (4 digits from film edge code)
  char format[32];        // format (i.e. academy)
  UInt32 frame_position;  // frame position in sequence
  UInt32 sequence_len;    // sequence length in frames
  UInt32 held_count;      // held count (1 = default)
  int frame_rate;         // frame rate of original in frames/sec
  int shutter_angle;      // shutter angle of camera in degrees
  char frame_id[32];      // frame identification (i.e. keyframe)
  char slate_info[100];   // slate information
  UInt8 reserved[56];     // reserved for future use (padding)
} MotionPictureFilm;

typedef struct _television_header
{
  UInt32 time_code;       // SMPTE time code
  UInt32 userBits;        // SMPTE user bits
  UInt8 interlace;        // interlace ( 0 = noninterlaced, 1 = 2:1 interlace
  UInt8 field_num;        // field number
  UInt8 video_signal;     // video signal standard
  UInt8 unused;           // used for byte alignment only
  int hor_sample_rate;    // horizontal sampling rate in Hz
  int ver_sample_rate;    // vertical sampling rate in Hz
  int frame_rate;         // temporal sampling rate or frame rate in Hz
  int time_offset;        // time offset from sync to first pixel
  int gamma;              // gamma value
  int black_level;        // black level code value
  int black_gain;         // black gain
  int break_point;        // breakpoint
  int white_level;        // reference white level code value
  int integration_times;  // integration time(s)
  UInt8 reserved[76];     // reserved for future use (padding)
} TelevisionHeader;

typedef struct _dpxImageHeader {
  FileInformation fileInformationHeader;
  ImageInformation imageInformationHeader;
  ImageOrientation imageOrientationHeader;
  MotionPictureFilm mpfHeader;
  TelevisionHeader tvHeader;
} DPXImageHeader;


void freeDPXDataProviderMemory(void *info, const void *data, size_t size) {
  free((void *)data);
}

DPXImage readDPXImage(CFURLRef url) {
  
  CFDataRef dpxFileData;
  CFDictionaryRef dpxFileProperties;
  CFArrayRef propertyRequest = CFArrayCreate(kCFAllocatorDefault, NULL, 0, NULL);
  SInt32 errorCode = 0;
  
  if (!CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, url, &dpxFileData, &dpxFileProperties, propertyRequest, &errorCode)) {
    CFRelease(propertyRequest);
    return NULL;
  }
  
  if (CFDataGetLength(dpxFileData) < sizeof(DPXImageHeader)) {
    // Not a dpx file: file is too small
    CFRelease(dpxFileData);
    CFRelease(dpxFileProperties);
    CFRelease(propertyRequest);
    return NULL;
  }
  
  // release the objects we don't need anymore
  CFRelease(dpxFileProperties);
  CFRelease(propertyRequest);
  
  const DPXImageHeader *fileHeader = (const DPXImageHeader *)CFDataGetBytePtr(dpxFileData);

  if ((fileHeader->fileInformationHeader.magic_num != 0x53445058) && (fileHeader->fileInformationHeader.magic_num != 0x58504453) && (fileHeader->fileInformationHeader.magic_num != 0x802A5FD7)) { // 0x802A5FD7 is for Cineon (.cin)
    // not a DPX (or Cineon) file
    CFRelease(dpxFileData);

    return NULL;
  }

  return dpxFileData;
}

void releaseDPXImage(DPXImage image) {
  CFRelease(image);
}

DPXImage DPXisValid(const DPXImage image) {
  if (!image) {
    return NULL;
  }

  const DPXImageHeader *fileHeader = (const DPXImageHeader *)CFDataGetBytePtr(image);
  if ((fileHeader->fileInformationHeader.magic_num != 0x53445058) && (fileHeader->fileInformationHeader.magic_num != 0x58504453) && (fileHeader->fileInformationHeader.magic_num != 0x802A5FD7)) { // 0x802A5FD7 is for Cineon (.cin)
    // not a DPX (or Cineon) file
    return NULL;
  }
  
  return image;
}

const char* DPXcreator(const DPXImage image) {
  if (!DPXisValid(image)) {
    return "DPXImage: not a valid image";
  }
  
  const DPXImageHeader *fileHeader = (const DPXImageHeader *)CFDataGetBytePtr(image);

  return fileHeader->fileInformationHeader.creator;
}

size_t DPXwidth(const DPXImage image) {
  if (!DPXisValid(image)) {
    return 0;
  }
  
  const DPXImageHeader *fileHeader = (const DPXImageHeader *)CFDataGetBytePtr(image);
  UInt32 width = fileHeader->imageInformationHeader.pixels_per_line;
  if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
    width = CFSwapInt32(width);
  }

  return width;
}

size_t DPXheight(const DPXImage image) {
  if (!DPXisValid(image)) {
    return 0;
  }
  
  const DPXImageHeader *fileHeader = (const DPXImageHeader *)CFDataGetBytePtr(image);
  UInt32 height = fileHeader->imageInformationHeader.lines_per_image_ele;
  if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
    height = CFSwapInt32(height);
  }
  return height;
}

CGSize DPXsize(const DPXImage image) {
  return CGSizeMake(DPXwidth(image), DPXheight(image));
}

// return a CGImage containing the image
CGImageRef createCGImageFromDPX(const DPXImage image) {
  if (!DPXisValid(image)) {
    return NULL;
  }

  const DPXImageHeader *fileHeader = (const DPXImageHeader *)CFDataGetBytePtr(image);

  size_t width = DPXwidth(image);
  size_t height = DPXheight(image);

  const UInt8 bitSize = fileHeader->imageInformationHeader.image_element[0].bit_size;
  size_t bitsPerComponent = 8;
  CGBitmapInfo  bitmapInfo = kCGBitmapByteOrderDefault;
  if (bitSize == 16) {
    bitsPerComponent = 16;
    if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
      bitmapInfo = kCGBitmapByteOrder16Little;
    } else {
      bitmapInfo = kCGBitmapByteOrder16Big;
    }
  }
  if (bitSize == 32) {
    // 32 bits means float
    bitsPerComponent = 32;
    if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
      bitmapInfo = kCGBitmapByteOrder32Little | kCGBitmapFloatComponents;
    } else {
      bitmapInfo = kCGBitmapByteOrder32Big | kCGBitmapFloatComponents;
    }
 }

  const UInt8 descriptor = fileHeader->imageInformationHeader.image_element[0].descriptor;
  size_t components = 3;
  if (descriptor >= 1 && descriptor <= 8) {
    components = 1;
  } else if ((bitSize != 10) && (bitSize != 12)) {
      if (descriptor == 51) {
      components = 4;
      bitmapInfo |= kCGImageAlphaLast;
    } else if (descriptor == 52) {
      components = 4;
      bitmapInfo |= kCGImageAlphaFirst;
    }
  }
  
  size_t bitsPerPixel = components * bitsPerComponent;
  size_t bytesPerRow = bitsPerPixel / 8 * width;
  CGColorSpaceRef colourSpace = CGColorSpaceCreateDeviceRGB();

  UInt32 padding = fileHeader->imageInformationHeader.image_element[0].eol_padding;
  if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
    padding = CFSwapInt32(padding);
  }
  
  UInt16 packing = fileHeader->imageInformationHeader.image_element[0].packing;
  if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
    packing = CFSwapInt16(packing);
  }

  UInt8 *data = calloc(height, bytesPerRow);

  const UInt32 *sourceData = (const UInt32 *)CFDataGetBytePtr(image);
  UInt32 offset = fileHeader->imageInformationHeader.image_element[0].data_offset;
  if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
    offset = CFSwapInt32(offset);
  }
  sourceData +=  offset / 4;

  // extract image_element[0];
  if (bitSize == 8 || bitSize == 16) {
    memcpy(data, sourceData, height * bytesPerRow);
  }
  if (bitSize == 10) {
    for (size_t y = 0; y < height; y++) {
      for (size_t x = 0; x < width; x++) {
        if (packing == 1) { // 10-bit components filled
          if (descriptor == 50) {  // RGB source image

            UInt32 sourcePixel = sourceData[(y * width + x)];
            if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
              sourcePixel = CFSwapInt32(sourcePixel);
            }
            data[(y * width + x) * 3 + 0] = sourcePixel >> 24;
            data[(y * width + x) * 3 + 1] = sourcePixel >> 14;
            data[(y * width + x) * 3 + 2] = sourcePixel >> 4;

          } else if (descriptor == 51) {   // RGBA

            const size_t targetIndex = (y * width + x) * 3;

            // calculate the index of this pixel's R component in the source data
            const size_t componentBaseIndex = (y * width + x) * 4;  // 4 components per source pixel

            for (size_t component = 0; component < 3; component++) {

              const size_t sourceIndex = (componentBaseIndex + component) / 3;  // 1 32bit source pixel holds 3 10bit components
              const size_t shift = 24 - ((componentBaseIndex + component) % 3) * 10;

              UInt32 sourceWord = sourceData[sourceIndex];
              if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
                sourceWord = CFSwapInt32(sourceWord);
              }

              data[targetIndex + component] = sourceWord >> shift;
            }
          }
        }
      }
    }
  }
  if (bitSize == 12) {
    if (packing == 1) {
      const size_t length = width * height * 3 / 2; // (3 words per 2 pixels)
      size_t dataIndex = 0;
      
      for (size_t i = 0; i < length; i++) {
        UInt32 sourceWord = sourceData[i];
        if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
          sourceWord = CFSwapInt32(sourceWord);
        }

        data[dataIndex++] = sourceWord  >> 24;
        data[dataIndex++] = sourceWord >> 8;
      }
    }
  }

  CGDataProviderRef imageDataProvider = CGDataProviderCreateWithData(NULL, data, bytesPerRow * height, &freeDPXDataProviderMemory);
  if (imageDataProvider == NULL) {
    CGColorSpaceRelease(colourSpace);
    return NULL;
  }

  CGImageRef cgImage = CGImageCreate(width, height, bitsPerComponent, bitsPerPixel, bytesPerRow, colourSpace, bitmapInfo, imageDataProvider, NULL, false, kCGRenderingIntentDefault);

  CGColorSpaceRelease(colourSpace);
  CGDataProviderRelease(imageDataProvider);

  return cgImage;
}

// return a CGImage with a specified size containing the image
// caution: this is meant to be used to create thumbnails and uses a very simple scaling algorithm
CGImageRef createThumbnailCGImageWithSizeFromDPX(const DPXImage image, CGSize size) {
  if (!DPXisValid(image)) {
    return NULL;
  }
  
  const DPXImageHeader *fileHeader = (const DPXImageHeader *)CFDataGetBytePtr(image);
  
  size_t width = DPXwidth(image);
  size_t height = DPXheight(image);
  
  if (width <= size.width && height <= size.height) return createCGImageFromDPX(image);
  
  CGFloat scale = fmax((CGFloat)width / (size.width - 1.0f), (CGFloat)height / (size.height - 1.0f));

  size_t thumbwidth = (size_t)(width / scale) + 1;
  size_t thumbheight = (size_t)(height / scale) + 1;

  // a simple 'close neighbour' scaling should be fine for thumbnails
  
  const UInt8 bitSize = fileHeader->imageInformationHeader.image_element[0].bit_size;
  size_t bitsPerComponent = 8;
  CGBitmapInfo  bitmapInfo = kCGBitmapByteOrderDefault;
  if (bitSize == 16) {
    bitsPerComponent = 16;
    if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
      bitmapInfo = kCGBitmapByteOrder16Little;
    } else {
      bitmapInfo = kCGBitmapByteOrder16Big;
    }
  }
  if (bitSize == 32) {
    // 32 bits means float
    bitsPerComponent = 32;
    if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
      bitmapInfo = kCGBitmapByteOrder32Little;
    } else {
      bitmapInfo = kCGBitmapByteOrder32Big;
    }
  }
  
  const UInt8 descriptor = fileHeader->imageInformationHeader.image_element[0].descriptor;
  size_t components = 3;
  if (descriptor >= 1 && descriptor <= 8) {
    components = 1;
  } else if ((bitSize != 10) && (bitSize != 12)) {
    if (descriptor == 51) {
      components = 4;
      bitmapInfo |= kCGImageAlphaLast;
    } else if (descriptor == 52) {
      components = 4;
      bitmapInfo |= kCGImageAlphaFirst;
    }
  }

  
  size_t bitsPerPixel = components * bitsPerComponent;
  size_t bytesPerRow = bitsPerPixel / 8 * thumbwidth;
  CGColorSpaceRef colourSpace = CGColorSpaceCreateDeviceRGB();
  
  UInt32 padding = fileHeader->imageInformationHeader.image_element[0].eol_padding;
  if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
    padding = CFSwapInt32(padding);
  }
  
  UInt16 packing = fileHeader->imageInformationHeader.image_element[0].packing;
  if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
    packing = CFSwapInt16(packing);
  }
  
  UInt8 *data = calloc(thumbheight, bytesPerRow);
  
  const UInt32 *sourceData = (const UInt32 *)CFDataGetBytePtr(image);
  UInt32 offset = fileHeader->imageInformationHeader.image_element[0].data_offset;
  if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
    offset = CFSwapInt32(offset);
  }
  sourceData +=  offset / 4;

  // extract image_element[0];
  if (bitSize == 8 || bitSize == 16) {

    for (size_t y = 0; y < thumbheight; y++) {
      for (size_t x = 0; x < thumbwidth; x++) {

        const size_t targetIndex = (y * thumbwidth + x) * components;
        const size_t sourceIndex = (MIN((size_t)(y * scale), height - 1) * width + MIN((size_t)(x * scale), width - 1)) * components;

        for (size_t component = 0; component < components; component++) {
          if (bitSize == 8) {

            const UInt8 *sourceData8 = (const UInt8 *)sourceData;
            data[targetIndex + component] = sourceData8[sourceIndex + component];

          } else if (bitSize == 16) {

            UInt16 *data16 = (UInt16 *)data;
            const UInt16 *sourceData16 = (const UInt16 *)sourceData;
            data16[targetIndex + component] = sourceData16[sourceIndex + component];
          }
        }
      }
    }
  }
  if (bitSize == 10) {

    for (size_t y = 0; y < thumbheight; y++) {
      for (size_t x = 0; x < thumbwidth; x++) {
        if (packing == 1) { // 10-bit components filled
          if (descriptor == 50) {

            UInt32 sourcePixel = sourceData[MIN((size_t)(y * scale), height - 1) * width + MIN((size_t)(x * scale), width - 1)];

            if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
              sourcePixel = CFSwapInt32(sourcePixel);
            }

            data[(y * thumbwidth + x) * 3 + 0] = sourcePixel >> 24;
            data[(y * thumbwidth + x) * 3 + 1] = sourcePixel >> 14;
            data[(y * thumbwidth + x) * 3 + 2] = sourcePixel >> 4;

          } else if (descriptor == 51) { // RGBA
            const size_t targetIndex = (y * thumbwidth + x) * 3;  // 3 components in the target image

            // calculate the index of this pixel's R component in the source data
            const size_t componentBaseIndex = (MIN((size_t)(y * scale), height - 1) * width + MIN((size_t)(x * scale), width - 1)) * 4;  // 4 components per source pixel

            for (size_t component = 0; component < 3; component++) {
              const size_t sourceIndex = (componentBaseIndex + component) / 3;  // 1 32bit source pixel holds 3 10bit components
              const size_t shift = 24 - ((componentBaseIndex + component) % 3) * 10;

              UInt32 sourceWord = sourceData[sourceIndex];
              if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
                sourceWord = CFSwapInt32(sourceWord);
              }

              data[targetIndex + component] = sourceWord >> shift;
            }
          }
        }
      }
    }
  }
  if (bitSize == 12) {
    if (packing == 1) {
      for (size_t y = 0; y < thumbheight; y++) {
        for (size_t x = 0; x < thumbwidth; x++) {
          const size_t targetIndex = (y * thumbwidth + x) * 3;

          // calculate the index of this pixel's R component in the source data
          const size_t componentBaseIndex = (MIN((size_t)(y * scale), height - 1) * width + MIN((size_t)(x * scale), width - 1)) * 3;
          
          for (size_t component = 0; component < 3; component++) {
            const size_t sourceIndex = (componentBaseIndex + component) / 2;  // 1 32bit source word holds 2 12bit components
            const size_t shift = ((componentBaseIndex + component) % 2 == 0) ? 24 : 8;

            UInt32 sourceWord = sourceData[sourceIndex];
            if (fileHeader->fileInformationHeader.magic_num == 0x58504453) {
              sourceWord = CFSwapInt32(sourceWord);
            }

            data[targetIndex + component] = sourceWord >> shift;
          }
        }
      }
    }
  }
  
  CGDataProviderRef imageDataProvider = CGDataProviderCreateWithData(NULL, data, bytesPerRow * thumbheight, &freeDPXDataProviderMemory);
  if (imageDataProvider == NULL) {
    CGColorSpaceRelease(colourSpace);
    return NULL;
  }
  
  CGImageRef cgImage = CGImageCreate(thumbwidth, thumbheight, bitsPerComponent, bitsPerPixel, bytesPerRow, colourSpace, bitmapInfo, imageDataProvider, NULL, false, kCGRenderingIntentDefault);
  
  CGColorSpaceRelease(colourSpace);
  CGDataProviderRelease(imageDataProvider);
  
  return cgImage;
}
