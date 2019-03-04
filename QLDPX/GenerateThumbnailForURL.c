#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>

#include "DPXImage.h"

OSStatus GenerateThumbnailForURL(void *thisInterface, QLThumbnailRequestRef thumbnail, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options, CGSize maxSize);
void CancelThumbnailGeneration(void *thisInterface, QLThumbnailRequestRef thumbnail);

/* -----------------------------------------------------------------------------
    Generate a thumbnail for file

   This function's job is to create thumbnail for designated file as fast as possible
   ----------------------------------------------------------------------------- */

OSStatus GenerateThumbnailForURL(void *thisInterface, QLThumbnailRequestRef thumbnail, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options, CGSize maxSize)
{
  // Is the URL's file extension '.dpx'?
  CFStringRef urlString = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
  if (CFStringCompareWithOptions(urlString, CFSTR(".dpx"), CFRangeMake(CFStringGetLength(urlString) - 4, 4), kCFCompareCaseInsensitive) != kCFCompareEqualTo) {
    CFRelease(urlString);
    return noErr;
  };
  CFRelease(urlString);
  
  DPXImage img = readDPXImage(url);
  if (!img) {
    return noErr;
  }
  
  if (QLThumbnailRequestIsCancelled(thumbnail)) {
    releaseDPXImage(img);
    return noErr;
  }

  CGImageRef cgDPX = createThumbnailCGImageWithSizeFromDPX(img, QLThumbnailRequestGetMaximumSize(thumbnail));
 
  if (QLThumbnailRequestIsCancelled(thumbnail)) {
    CGImageRelease(cgDPX);
    releaseDPXImage(img);
    return noErr;
  }
  
  if (cgDPX != NULL) {
    CFDictionaryRef properties = NULL;
    QLThumbnailRequestSetImage(thumbnail, cgDPX, properties);
  }
  
  CGImageRelease(cgDPX);
  releaseDPXImage(img);
  return noErr;
}

void CancelThumbnailGeneration(void *thisInterface, QLThumbnailRequestRef thumbnail)
{
    // Implement only if supported
}
