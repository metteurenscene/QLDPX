//
//  DPXImage.h
//  QLDPX
//
//  Created by Thomas Angarano on 12/01/2019.
//  Copyright © 2019 Thomas Angarano. All rights reserved.
//

#ifndef QLDPX_DPXIMAGE_H_
#define QLDPX_DPXIMAGE_H_

#include <CoreGraphics/CoreGraphics.h>

#include <stdio.h>

typedef const void * DPXImage;

// DPXImage readDPXImage(CFURLRef url)
// reads a DPX image from the given URL
// returns
//  - a reference to the DPX image if the file exists and is a DPX file.
//      The caller takes ownership of the returned image and has to release
//      it with releaseDPXImage when it is no longer needed.
//  - NULL if the file couldn't be read
DPXImage readDPXImage(CFURLRef url);

// void releaseDPXImage(DPXImage)
// release a DPX image and free its memory.
// The DPXImage and pointers derived from it (e.g. with *DPXcreator)
// must not be used after calling this function.
void releaseDPXImage(DPXImage);

// const char *DPXcreator(DPXImage)
// returns a pointer to the "creator" field (stored as a 0-terminated C string)
// in the DPX header. The pointer will be valid until the DPXImage has been
// released with releaseDPXImage.
const char *DPXcreator(DPXImage);

// CGSize DPXsize(DPXImage)
// returns the size of the image
CGSize DPXsize(DPXImage);

// CGImageRef createCGImageFromDPX(DPXImage)
// create a CGImage representation of the DPX image.
// The caller takes ownership of the returned CGImage
CGImageRef createCGImageFromDPX(DPXImage);

// CGImageRef createCGImageWithSizeFromDPX(DPXImage, CGSize)
// create a thumbnail represation of the image with the given size.
// If the size is larger than the original image, the original image
// will be returned.
// The caller takes ownership of the returned CGImage
CGImageRef createThumbnailCGImageWithSizeFromDPX(DPXImage, CGSize);

#endif  // QLDPX_DPXIMAGE_H_
