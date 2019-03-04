#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>

#include "DPXImage.h"

OSStatus GeneratePreviewForURL(void *thisInterface, QLPreviewRequestRef preview, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options);
void CancelPreviewGeneration(void *thisInterface, QLPreviewRequestRef preview);

/* -----------------------------------------------------------------------------
   Generate a preview for file

   This function's job is to create preview for designated file
   ----------------------------------------------------------------------------- */

OSStatus GeneratePreviewForURL(void *thisInterface, QLPreviewRequestRef preview, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options)
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

  // The above might have taken some time, so before proceeding make sure the user didn't cancel the request
  if (QLPreviewRequestIsCancelled(preview)) {
    releaseDPXImage(img);
    return noErr;
  }
  
  CGSize size = DPXsize(img);

  CGContextRef cgContext = QLPreviewRequestCreateContext(preview, size, true, NULL);
  if (cgContext) {

    CGImageRef cgDPX = createCGImageFromDPX(img);
    if (cgDPX != NULL) {
      CGContextDrawImage(cgContext, CGRectMake(0.0, 0.0, size.width, size.height), cgDPX);
    }

    QLPreviewRequestFlushContext(preview, cgContext);

    CGImageRelease(cgDPX);
    CFRelease(cgContext);
  }

  releaseDPXImage(img);
  return noErr;
}

void CancelPreviewGeneration(void *thisInterface, QLPreviewRequestRef preview)
{
    // Implement only if supported
}
