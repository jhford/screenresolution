/*
 * screenresolution
 * Set the screen resolution for the main display from the command line
 * Build: clang -framework ApplicationServices main.c -o screenresolution
 * 
 * John Ford <john@johnford.info>
 * todo:
 *   -make printError easier for making strings
 *   -support more than one monitor
 */

#include <ApplicationServices/ApplicationServices.h>

// Don't know how realistic it is to have more than 10 at this point.
// Feel free to remind me about 640k being enough :)
#define MAX_DISPLAYS 10

struct config{
    size_t w;
    size_t h;
    size_t d;
};

void printError(char* msg);
unsigned int setDisplayToMode (CGDirectDisplayID display, CGDisplayModeRef mode);
unsigned int configureDisplay(CGDirectDisplayID display, struct config * config);
unsigned int listCurrentMode(CGDirectDisplayID display, int displayNum);
unsigned int listAvailableModes(CGDirectDisplayID display, int displayNum);
unsigned int parseStringConfig(const char* string, struct config * out);
size_t bitDepth(CGDisplayModeRef mode);


int main (int argc, const char * argv[])
{
    unsigned int exitcode = 0; 
    CGDirectDisplayID mainDisplay = CGMainDisplayID();
    
    if (argc == 1) {
        // By default, print out the main display's resolution
        if (!listCurrentMode(mainDisplay, 0)){
            exitcode++;
        }
    } else if (argc == 2) {
        CGError rc;
        uint32_t displayCount;
        CGDirectDisplayID activeDisplays [MAX_DISPLAYS];
        rc = CGGetActiveDisplayList(MAX_DISPLAYS, activeDisplays, &displayCount);
        if (strcmp(argv[1], "get") == 0){
            for (int d = 0; d < displayCount; d++){
                if (!listCurrentMode(activeDisplays[d], d)){
                    exitcode++;
                }
            }
        }/* else if (strcmp(argv[1], "list") == 0)
            if (!listAvailableModes(mainDisplay)){
                exitcode++;
            }
        }*/ else {
            fprintf(stderr, "I am sorry %s, but I cannot let you do that\n", getlogin());
            exitcode++;
        }
    } else if (argc == 3 && strcmp(argv[1], "set-main") == 0) {
        struct config newConfig;
        if (parseStringConfig(argv[2], &newConfig)){
            if (!configureDisplay(mainDisplay, &newConfig)){
                exitcode++;
            }
        } else {
            fprintf(stderr, "Error: The mode '%s' could not be parsed\n", argv[2]);
            exitcode++;
        }

    } else {
        printError("Use it the right way!");
        exitcode++;
    }
    return exitcode;
}

void printError(char* msg){
    fprintf(stderr, "Error: %s\n", msg);
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

unsigned int configureDisplay(CGDirectDisplayID display, struct config * config){
    unsigned int returncode = 1;
    CFArrayRef allModes = CGDisplayCopyAllDisplayModes(display, NULL);
    if (allModes == NULL){
        printError("failed trying to look up all modes for display");
    }
    CGDisplayModeRef newMode = NULL;
    size_t pw; // possible width
    size_t ph; // possible height
    size_t pd; // possible depth
    bool looking = true; // used to decide whether to continue looking for modes
    for (int i = 0; i < CFArrayGetCount(allModes) && looking ; i++) {
        CGDisplayModeRef possibleMode = (CGDisplayModeRef) CFArrayGetValueAtIndex(allModes, i);
        pw = CGDisplayModeGetWidth(possibleMode);
        ph = CGDisplayModeGetHeight(possibleMode);
        pd = bitDepth(possibleMode);
        if ( pw == config->w && ph == config->h && pd == config->d ) {
            looking = false; // Stop looking for more modes!
            newMode = possibleMode;
        }
    }
    if (newMode != NULL) {
        printf("Setting mode to %lux%lux%lu\n", pw,ph,pd);
        setDisplayToMode(display,newMode);
    } else {
        fprintf(stderr, "Error: requested mode (%lux%lux%lu) is not available\n", config->w, config->h, config->d);
        returncode = 0;
    }
    return returncode;
}


unsigned int setDisplayToMode (CGDirectDisplayID display, CGDisplayModeRef mode) {
    unsigned int returncode = 1;
    CGError rc;
    CGDisplayConfigRef config;
    rc = CGBeginDisplayConfiguration(&config);
    if (rc != kCGErrorSuccess) {
        printError("failed CGBeginDisplayConfiguration");
        returncode = 0;
    }
    if (returncode){
        rc = CGConfigureDisplayWithDisplayMode(config, display, mode, NULL);
        if (rc != kCGErrorSuccess) {
            printError("failed CGConfigureDisplayWithDisplayMode");
            returncode = 0;
        }
    }
    if (returncode){
        rc = CGCompleteDisplayConfiguration(config, kCGConfigureForSession );
        if (rc != kCGErrorSuccess) {
            printf("failed CGCompleteDisplayConfiguration");
            returncode = 0;
        }
    }
    return returncode;
}
unsigned int listCurrentMode(CGDirectDisplayID display, int displayNum){
    unsigned int returncode = 1;
    CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(display);
    if (currentMode == NULL){
        printError("unable to copy current display mode");
        returncode = 0;
    }
    printf("%d: %lux%lux%lu\n", 
           displayNum,
           CGDisplayModeGetWidth(currentMode), 
           CGDisplayModeGetHeight(currentMode),
           bitDepth(currentMode));
    CGDisplayModeRelease(currentMode);
    return returncode;
}

unsigned int listAvailableModes(CGDirectDisplayID display, int displayNum){
    unsigned int returncode = 1;
    CFArrayRef allModes = CGDisplayCopyAllDisplayModes(display, NULL);
    if (allModes == NULL) {
        returncode = 0;
    }
    printf("Available Modes on Display %d\n", displayNum);
    for (int i = 0; i < CFArrayGetCount(allModes) && returncode; i++) {
        CGDisplayModeRef mode = (CGDisplayModeRef) CFArrayGetValueAtIndex(allModes, i);
        printf("  %lux%lux%lu\n",
               CGDisplayModeGetWidth(mode),
               CGDisplayModeGetHeight(mode),
               bitDepth(mode));
    }
    return returncode;
}

unsigned int parseStringConfig(const char* string, struct config * out){
    unsigned int rc;
    size_t w;
    size_t h;
    size_t d;
    int numConverted = sscanf(string, "%lux%lux%lu", &w, &h, &d);
    if (numConverted != 3) {
        rc = 0;
    } else{
        out->w = w;
        out->h = h;
        out->d = d;
        rc = 1;
    }
    return rc;
}
