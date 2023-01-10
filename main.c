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

#include "version.h"
#include "cg_utils.h"
// Number of modes to list per line.
#define MODES_PER_LINE 3

// I have written an alternate list routine that spits out WAY more info
// #define LIST_DEBUG 1

unsigned int listAvailableModes(CGDirectDisplayID display, int displayNum);
unsigned int listCurrentMode(CGDirectDisplayID display, int displayNum);
unsigned int listMaxSupported(CGDirectDisplayID display, int displayNum);

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
            NSLog(CFSTR("%s"), "Error: could not allocate memory for display list");
            return 1;
        }
        rc = CGGetActiveDisplayList(activeDisplayCount, activeDisplays, &displayCount);
        if (rc != kCGErrorSuccess) {
            NSLog(CFSTR("%s"), "Error: failed to get list of active displays");
            return 1;
        }

        if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "?") == 0) {
	    printf("Usage: Run screenreader with one of the following: get, list, or set [resolution].\n");
        return(0);
        }


        // This loop should probably be in another function.
        for (d = 0; d < displayCount && keepgoing; d++) {
            if (strcmp(argv[1], "get") == 0) {
                if (!listCurrentMode(activeDisplays[d], d)) {
                    exitcode++;
                }
            } else if (strcmp(argv[1], "getMax") == 0) {
                if (!listMaxSupported(activeDisplays[d], d)) {
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
        NSLog(CFSTR("%s"), "Invalid command. Run screenreader with get, list, or set [resolution].");
        exitcode++;
    }
    return exitcode > 0;
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

unsigned int listMaxSupported(CGDirectDisplayID display, int displayNum) {
    unsigned int returncode = 1;
    CGDisplayModeRef maxSupportedMode = CGDisplayCopyDisplayMode(display);

    CFStringRef keys[1] = { kCGDisplayShowDuplicateLowResolutionModes };
    CFBooleanRef values[1] = { kCFBooleanTrue };
    CFDictionaryRef options = CFDictionaryCreate(
        kCFAllocatorDefault, (const void**) keys, (const void**) values, 1,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFArrayRef allModes = CGDisplayCopyAllDisplayModes(display, options);
    if (allModes == NULL) {
        NSLog(CFSTR("Error: failed trying to look up modes for display %u"), displayNum);
    }

    CFIndex displayModeCount = CFArrayGetCount(allModes);
    for (int i = 0; i < displayModeCount; ++i) {
        CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);
        CFComparisonResult result = _compareCFDisplayModes(mode, maxSupportedMode, NULL);
        if (result == kCFCompareGreaterThan) {
            maxSupportedMode = CGDisplayModeRetain(mode);
        }
    }

    NSLog(CFSTR("Display %d: %ux%ux%u@%.0f"),
           displayNum,
           CGDisplayModeGetWidth(maxSupportedMode),
           CGDisplayModeGetHeight(maxSupportedMode),
           bitDepth(maxSupportedMode),
           CGDisplayModeGetRefreshRate(maxSupportedMode));

    CFRelease(allModes);
    CFRelease(options);
    CGDisplayModeRelease(maxSupportedMode);
    return returncode;
}

unsigned int listAvailableModes(CGDirectDisplayID display, int displayNum) {
    unsigned int returncode = 1;
    int numModes = 0;
    int i;

    CFArrayRef allModes = CGDisplayCopyAllDisplayModes(display, NULL);
    if (allModes == NULL) {
        returncode = 0;
    }

    numModes = CFArrayGetCount(allModes);

    // sort the array of display modes
    CFMutableArrayRef allModesSorted =  CFArrayCreateMutableCopy(
                                          kCFAllocatorDefault,
                                          numModes,
                                          allModes
                                        );

    CFArraySortValues(
        allModesSorted,
        CFRangeMake(0, CFArrayGetCount(allModesSorted)),
        (CFComparatorFunction) _compareCFDisplayModes,
        NULL
    );

#ifndef LIST_DEBUG
    if(displayNum != 0)
        printf("\n\n");
    printf("Available Modes on Display %d\n", displayNum);
#endif

    CGDisplayModeRef mode;

    int modesPerColumn = numModes / MODES_PER_LINE;

    for (i = 0; (i < numModes) && returncode; i++) {
        int rowNumber = (i / MODES_PER_LINE);
        int idxDisplayMode = (i % MODES_PER_LINE) * modesPerColumn + rowNumber;

        // if there are an even number of display modes to display,
        // the last mode must have it's index decremented by 1
        idxDisplayMode = MIN(idxDisplayMode, numModes - 1);

        mode = (CGDisplayModeRef) CFArrayGetValueAtIndex(allModesSorted, idxDisplayMode);

        // This formatting is functional but it ought to be done less poorly.
#ifndef LIST_DEBUG
        if (i % MODES_PER_LINE == 0) {
            printf("  ");
        } else {
            printf("\t");
        }

        char modestr [50];
        sprintf(modestr, "%4lux%4lux%lu@%.0f",
               CGDisplayModeGetWidth(mode),
               CGDisplayModeGetHeight(mode),
               bitDepth(mode),
               CGDisplayModeGetRefreshRate(mode));
        printf("%-20s ", modestr);

        if(i % MODES_PER_LINE == MODES_PER_LINE - 1)
            printf("\n");
#else
        uint32_t ioflags = CGDisplayModeGetIOFlags(mode);
        printf("display: %d %4lux%4lux%2lu@%.0f usable:%u ioflags:%4x valid:%u safe:%u default:%u",
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

    if (numModes % MODES_PER_LINE)
        printf("\n");

    CFRelease(allModes);
    CFRelease(allModesSorted);

    return returncode;
}
