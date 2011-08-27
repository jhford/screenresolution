/*
 * screenresolution
 * Set the screen resolution for the main display from the command line
 * Usage: screenresolution 1920 1200 32
 * Build: clang -framework ApplicationServices main.c -o screenresolution
 * 
 * John Ford <john@johnford.info>
 */

#include <ApplicationServices/ApplicationServices.h>

bool switchDisplayToMode (CGDirectDisplayID display, CGDisplayModeRef mode);
size_t bitDepth(CGDisplayModeRef mode);

int main (int argc, const char * argv[])
{
    unsigned int exitcode = 0; 
    size_t w;
    size_t h;
    size_t d;
    CGDirectDisplayID mainDisplay;
    
    mainDisplay = CGMainDisplayID();
    
    if (argc == 1) {
        CGDisplayModeRef currentMode;
        currentMode = CGDisplayCopyDisplayMode(mainDisplay);
        printf("%lux%lux%lu\n", CGDisplayModeGetWidth(currentMode), 
                                 CGDisplayModeGetHeight(currentMode),
                                 bitDepth(currentMode));
        CGDisplayModeRelease(currentMode);
    } else if (argc == 4) {
        CFArrayRef allModes = CGDisplayCopyAllDisplayModes(mainDisplay, NULL);
        CGDisplayModeRef newMode = NULL;
        size_t pw; // possible width
        size_t ph; // possible height
        size_t pd; // possible depth
        bool looking = true; // used to decide whether to continue looking for modes
        w = atoi(argv[1]);
        h = atoi(argv[2]);
        d = atoi(argv[3]);
        for (int i = 0; i < CFArrayGetCount(allModes) && looking ; i++) {
            CGDisplayModeRef possibleMode = (CGDisplayModeRef) CFArrayGetValueAtIndex(allModes, i);
            pw = CGDisplayModeGetWidth(possibleMode);
            ph = CGDisplayModeGetHeight(possibleMode);
            pd = bitDepth(possibleMode);
            if ( pw == w && ph == h && pd == d ) {
                looking = false; // Stop looking for more modes!
                newMode = possibleMode;
                //printf("Found matching mode(%lux%lux%lux)", pw,ph,pd);
            }
#ifdef DEBUG
            else {
                printf("DEBUG: ignoring %lux%lux%lu\n", pw,ph,pd);
            }
#endif
        }
            if (newMode != NULL) {
                printf("Setting mode to %lux%lux%lu\n", pw,ph,pd);
                switchDisplayToMode(mainDisplay,newMode);
            } else {
                fprintf(stderr, "Error: requested mode (%lux%lux%lu) is not available\n",w,h,d);
                exitcode++;
            }
    } else {
        fprintf(stderr, "Usage: %s <width> <height> <depth>\n", argv[0]);
        exitcode++;
    }
    return exitcode;
}


size_t bitDepth(CGDisplayModeRef mode) {
    size_t depth = 0;
	CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(mode);
	if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive)) {
		depth = 32;
    }
	else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive)) {
		depth = 16;
    }
	else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive)) {
		depth = 8;
    }
	return depth;
}

bool switchDisplayToMode (CGDirectDisplayID display, CGDisplayModeRef mode) {
    CGError rc;
    CGDisplayConfigRef config;
    rc = CGBeginDisplayConfiguration(&config);
    if (rc != kCGErrorSuccess) {
        printf("ERROR: failed CGBeginDisplayConfiguration\n");
        return false;
    }
    rc = CGConfigureDisplayWithDisplayMode(config, display, mode, NULL);
    if (rc != kCGErrorSuccess) {
        printf("ERROR: failed CGConfigureDisplayWithDisplayMode\n");
        return false;
    }
    rc = CGCompleteDisplayConfiguration(config, kCGConfigureForSession );
    if (rc != kCGErrorSuccess) {
        printf("ERROR: failed CGCompleteDisplayConfiguration\n");
        return false;
    }
    return true;
}
