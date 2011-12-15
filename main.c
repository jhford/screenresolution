// vim: ts=4:sw=4
/*
 * screenresolution sets the screen resolution on Mac computers.
 * Copyright (C) 2011  John Ford <john@johnford.info>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <ApplicationServices/ApplicationServices.h>
#include "version.h"

// Number of modes to list per line.
#define MODES_PER_LINE 3

// I have written an alternate list routine that spits out WAY more info
// #define LIST_DEBUG 1

struct config {
    size_t w;
    size_t h;
    size_t d;
};

unsigned int setDisplayToMode(CGDirectDisplayID display, CGDisplayModeRef mode);
unsigned int configureDisplay(CGDirectDisplayID display,
                              struct config *config,
                              int displayNum);
unsigned int listCurrentMode(CGDirectDisplayID display, int displayNum);
unsigned int listAvailableModes(CGDirectDisplayID display, int displayNum);
unsigned int parseStringConfig(const char *string, struct config *out);
size_t bitDepth(CGDisplayModeRef mode);

// http://stackoverflow.com/questions/3060121/core-foundation-equivalent-for-nslog/3062319#3062319
void NSLog(CFStringRef format, ...);



int main(int argc, const char *argv[]) {
    // http://developer.apple.com/library/IOs/#documentation/CoreFoundation/Conceptual/CFStrings/Articles/MutableStrings.html
    int i;
    CFMutableStringRef args = CFStringCreateMutable(NULL, 0);
    CFStringEncoding encoding = CFStringGetSystemEncoding();
    CFStringAppend(args, CFSTR("starting screenresolution argv="));
    for (i = 0 ; i < argc ; i++) {
        CFStringAppendCString(args, argv[i], encoding);
        // If I were so motivated, I'd probably use CFStringAppendFormat
        CFStringAppend(args, CFSTR(" "));
    }
    // This has security implications.  Will look at that later
    NSLog(CFSTR("%@"), args);
    unsigned int exitcode = 0;

    if (argc > 1) {
        int d;
        int keepgoing = 1;
        CGError rc;
        uint32_t displayCount = 0;
        uint32_t activeDisplayCount = 0;
        CGDirectDisplayID *activeDisplays = NULL;

        rc = CGGetActiveDisplayList(0, NULL, &activeDisplayCount);
        if (rc != kCGErrorSuccess) {
            NSLog(CFSTR("%s"), "Error: failed to get list of active displays");
            return 1;
        }
        // Allocate storage for the next CGGetActiveDisplayList call
        activeDisplays = (CGDirectDisplayID *) malloc(activeDisplayCount * sizeof(CGDirectDisplayID));
        if (activeDisplays == NULL) {
            NSLog(CFSTR("s"), "Error: could not allocate memory for display list");
            return 1;
        }
        rc = CGGetActiveDisplayList(activeDisplayCount, activeDisplays, &displayCount);
        if (rc != kCGErrorSuccess) {
            NSLog(CFSTR("%s"), "Error: failed to get list of active displays");
            return 1;
        }

        // This loop should probably be in another function.
        for (d = 0; d < displayCount && keepgoing; d++) {
            if (strcmp(argv[1], "get") == 0) {
                if (!listCurrentMode(activeDisplays[d], d)) {
                    exitcode++;
                }
            } else if (strcmp(argv[1], "list") == 0) {
                if (!listAvailableModes(activeDisplays[d], d)) {
                    exitcode++;
                }
            } else if (strcmp(argv[1], "set") == 0) {
                if (d < (argc - 2)) {
                    if (strcmp(argv[d+2], "skip") == 0 && d < (argc - 2)) {
                        printf("Skipping display %d\n", d);
                    } else {
                        struct config newConfig;
                        if (parseStringConfig(argv[d + 2], &newConfig)) {
                            if (!configureDisplay(activeDisplays[d], &newConfig, d)) {
                                exitcode++;
                            }
                        } else {
                            exitcode++;
                        }
                    }
                }
            } else if (strcmp(argv[1], "-version") == 0) {
                printf("screenresolution version %s\nLicensed under GPLv2\n", VERSION);
                keepgoing = 0;
            } else {
                NSLog(CFSTR("I'm sorry %s. I'm afraid I can't do that"), getlogin());
                exitcode++;
                keepgoing = 0;
            }
        }
        free(activeDisplays);
        activeDisplays = NULL;
    } else {
        NSLog(CFSTR("%s"), "Incorrect command line");
        exitcode++;
    }
    return exitcode > 0;
}

size_t bitDepth(CGDisplayModeRef mode) {
    size_t depth = 0;
	CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(mode);
    // my numerical representation for kIO16BitFloatPixels and kIO32bitFloatPixels
    // are made up and possibly non-sensical
    if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(kIO32BitFloatPixels), kCFCompareCaseInsensitive)) {
        depth = 96;
    } else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(kIO64BitDirectPixels), kCFCompareCaseInsensitive)) {
        depth = 64;
    } else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(kIO16BitFloatPixels), kCFCompareCaseInsensitive)) {
        depth = 48;
    } else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive)) {
        depth = 32;
    } else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(kIO30BitDirectPixels), kCFCompareCaseInsensitive)) {
        depth = 30;
    } else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive)) {
        depth = 16;
    } else if (kCFCompareEqualTo == CFStringCompare(pixelEncoding, CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive)) {
        depth = 8;
    }
    CFRelease(pixelEncoding);
    return depth;
}

unsigned int configureDisplay(CGDirectDisplayID display, struct config *config, int displayNum) {
    unsigned int returncode = 1;
    CFArrayRef allModes = CGDisplayCopyAllDisplayModes(display, NULL);
    if (allModes == NULL) {
        NSLog(CFSTR("Error: failed trying to look up modes for display %u"), displayNum);
    }

    CGDisplayModeRef newMode = NULL;
    CGDisplayModeRef possibleMode;
    size_t pw; // possible width.
    size_t ph; // possible height.
    size_t pd; // possible depth.
    int looking = 1; // used to decide whether to continue looking for modes.
    int i;
    for (i = 0 ; i < CFArrayGetCount(allModes) && looking; i++) {
        possibleMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);
        pw = CGDisplayModeGetWidth(possibleMode);
        ph = CGDisplayModeGetHeight(possibleMode);
        pd = bitDepth(possibleMode);
        if (pw == config->w &&
            ph == config->h &&
            pd == config->d) {
            looking = 0; // Stop looking for more modes!
            newMode = possibleMode;
        }
    }
    CFRelease(allModes);
    if (newMode != NULL) {
        NSLog(CFSTR("set mode on display %u to %ux%ux%u"), displayNum, pw, ph, pd);
        setDisplayToMode(display,newMode);
    } else {
        NSLog(CFSTR("Error: mode %ux%ux%u not available on display %u"), 
                config->w, config->h, config->d, displayNum);
        returncode = 0;
    }
    return returncode;
}

unsigned int setDisplayToMode(CGDirectDisplayID display, CGDisplayModeRef mode) {
    CGError rc;
    CGDisplayConfigRef config;
    rc = CGBeginDisplayConfiguration(&config);
    if (rc != kCGErrorSuccess) {
        NSLog(CFSTR("Error: failed CGBeginDisplayConfiguration err(%u)"), rc);
        return 0;
    }
    rc = CGConfigureDisplayWithDisplayMode(config, display, mode, NULL);
    if (rc != kCGErrorSuccess) {
        NSLog(CFSTR("Error: failed CGConfigureDisplayWithDisplayMode err(%u)"), rc);
        return 0;
    }
    rc = CGCompleteDisplayConfiguration(config, kCGConfigureForSession);
    if (rc != kCGErrorSuccess) {
        NSLog(CFSTR("Error: failed CGCompleteDisplayConfiguration err(%u)"), rc);        
        return 0;
    }
    return 1;
}

unsigned int listCurrentMode(CGDirectDisplayID display, int displayNum) {
    unsigned int returncode = 1;
    CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(display);
    if (currentMode == NULL) {
        NSLog(CFSTR("%s"), "Error: unable to copy current display mode");
        returncode = 0;
    }
    NSLog(CFSTR("Display %d: %ux%ux%u@%.0f"),
           displayNum,
           CGDisplayModeGetWidth(currentMode),
           CGDisplayModeGetHeight(currentMode),
           bitDepth(currentMode),
           CGDisplayModeGetRefreshRate(currentMode));
    CGDisplayModeRelease(currentMode);
    return returncode;
}

unsigned int listAvailableModes(CGDirectDisplayID display, int displayNum) {
    unsigned int returncode = 1;
    int i;
    CFArrayRef allModes = CGDisplayCopyAllDisplayModes(display, NULL);
    if (allModes == NULL) {
        returncode = 0;
    }
#ifndef LIST_DEBUG
    printf("Available Modes on Display %d\n", displayNum);

#endif
    CGDisplayModeRef mode;
    for (i = 0; i < CFArrayGetCount(allModes) && returncode; i++) {
        mode = (CGDisplayModeRef) CFArrayGetValueAtIndex(allModes, i);
        // This formatting is functional but it ought to be done less poorly.
#ifndef LIST_DEBUG
        if (i % MODES_PER_LINE == 0) {
            printf("  ");
        } else {
            printf("\t");
        }
        char modestr [50];
        sprintf(modestr, "%lux%lux%lu@%.0f",
               CGDisplayModeGetWidth(mode),
               CGDisplayModeGetHeight(mode),
               bitDepth(mode),
               CGDisplayModeGetRefreshRate(mode));
        printf("%-20s ", modestr);
        if (i % MODES_PER_LINE == MODES_PER_LINE - 1) {
            printf("\n");
        }
#else
        uint32_t ioflags = CGDisplayModeGetIOFlags(mode);
        printf("display: %d %4lux%4lux%2lu r:%.2f usable:%u ioflags:%4x valid:%u safe:%u default:%u",
                displayNum,
                CGDisplayModeGetWidth(mode),
                CGDisplayModeGetHeight(mode),
                bitDepth(mode),
                CGDisplayModeGetRefreshRate(mode),
                CGDisplayModeIsUsableForDesktopGUI(mode),
                ioflags,
                ioflags & kDisplayModeValidFlag ?1:0,
                ioflags & kDisplayModeSafeFlag ?1:0,
                ioflags & kDisplayModeDefaultFlag ?1:0 );
        printf(" safety:%u alwaysshow:%u nevershow:%u notresize:%u requirepan:%u int:%u simul:%u",
                ioflags & kDisplayModeSafetyFlags ?1:0,
                ioflags & kDisplayModeAlwaysShowFlag ?1:0,
                ioflags & kDisplayModeNeverShowFlag ?1:0,
                ioflags & kDisplayModeNotResizeFlag ?1:0,
                ioflags & kDisplayModeRequiresPanFlag ?1:0,
                ioflags & kDisplayModeInterlacedFlag ?1:0,
                ioflags & kDisplayModeSimulscanFlag ?1:0 );
        printf(" builtin:%u notpreset:%u stretched:%u notgfxqual:%u valagnstdisp:%u tv:%u vldmirror:%u\n",
                ioflags & kDisplayModeBuiltInFlag ?1:0,
                ioflags & kDisplayModeNotPresetFlag ?1:0,
                ioflags & kDisplayModeStretchedFlag ?1:0,
                ioflags & kDisplayModeNotGraphicsQualityFlag ?1:0,
                ioflags & kDisplayModeValidateAgainstDisplay ?1:0,
                ioflags & kDisplayModeTelevisionFlag ?1:0,
                ioflags & kDisplayModeValidForMirroringFlag ?1:0 );
#endif
    }
    CFRelease(allModes);
    return returncode;
}

unsigned int parseStringConfig(const char *string, struct config *out) {
    unsigned int rc;
    size_t w;
    size_t h;
    size_t d;
    int numConverted = sscanf(string, "%lux%lux%lu", &w, &h, &d);
    if (numConverted != 3) {
        rc = 0;
        NSLog(CFSTR("Error: the mode '%s' couldn't be parsed"), string);
    } else {
        out->w = w;
        out->h = h;
        out->d = d;
        rc = 1;
    }
    return rc;
}
