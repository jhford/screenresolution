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

// Don't know how realistic it is to have more than 10 at this point.
// Feel free to remind me about 640k being enough :)
#define MAX_DISPLAYS 10

// Number of modes to list per line.
#define MODES_PER_LINE 4

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


int main(int argc, const char *argv[]) {
    unsigned int exitcode = 0;

    if (argc > 1) {
        int d;
        int keepgoing = 1;
        CGError rc;
        uint32_t displayCount;
        CGDirectDisplayID activeDisplays[MAX_DISPLAYS];

        rc = CGGetActiveDisplayList(MAX_DISPLAYS, activeDisplays, &displayCount);
        if (rc != kCGErrorSuccess) {
            fprintf(stderr, "Error: failed to get list of active displays");
            exitcode++;
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
                if (strcmp(argv[d+2], "skip") == 0 && d < argc - 2) {
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
            } else if (strcmp(argv[1], "-version") == 0) {
                printf("screenresolution version %s\nLicensed under GPLv2\n", VERSION);
                keepgoing = 0;
            } else {
                fprintf(stderr, "I'm sorry %s. I'm afraid I can't do that\n", getlogin());
                exitcode++;
                keepgoing = 0;
            }
        }
    } else {
        fprintf(stderr, "Why failed? Because usage.\n");
        exitcode++;
    }
    return exitcode > 0;
}

size_t bitDepth(CGDisplayModeRef mode) {
    size_t depth = 0;
	CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(mode);
    // my numerical representation for kIO16BitFloatPixels and kIO32bitFloatPixels
    // are made up and possible non-sensical
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
    return depth;
}

unsigned int configureDisplay(CGDirectDisplayID display, struct config *config, int displayNum) {
    unsigned int returncode = 1;
    CFArrayRef allModes = CGDisplayCopyAllDisplayModes(display, NULL);
    if (allModes == NULL) {
        fprintf(stderr, "Error: failed trying to look up modes for display %d", displayNum);
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
    if (newMode != NULL) {
        printf("Setting mode on display %d to %lux%lux%lu\n",
                displayNum, pw, ph, pd);
        setDisplayToMode(display,newMode);
    } else {
        fprintf(stderr,
                "Error: requested mode (%lux%lux%lu) is not available\n",
                config->w, config->h, config->d);
        returncode = 0;
    }
    return returncode;
}

unsigned int setDisplayToMode(CGDirectDisplayID display, CGDisplayModeRef mode) {
    CGError rc;
    CGDisplayConfigRef config;
    rc = CGBeginDisplayConfiguration(&config);
    if (rc != kCGErrorSuccess) {
        fprintf(stderr, "Error: failed CGBeginDisplayConfiguration");
        return 0;
    }
    rc = CGConfigureDisplayWithDisplayMode(config, display, mode, NULL);
    if (rc != kCGErrorSuccess) {
        fprintf(stderr, "Error: failed CGConfigureDisplayWithDisplayMode");
        return 0;
    }
    rc = CGCompleteDisplayConfiguration(config, kCGConfigureForSession);
    if (rc != kCGErrorSuccess) {
        fprintf(stderr, "Error: failed CGCompleteDisplayConfiguration");
        return 0;
    }
    return 1;
}

unsigned int listCurrentMode(CGDirectDisplayID display, int displayNum) {
    unsigned int returncode = 1;
    CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(display);
    if (currentMode == NULL) {
        fprintf(stderr, "Error: unable to copy current display mode");
        returncode = 0;
    }
    printf("Display %d: %lux%lux%lu\n",
           displayNum,
           CGDisplayModeGetWidth(currentMode),
           CGDisplayModeGetHeight(currentMode),
           bitDepth(currentMode));
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
    printf("Available Modes on Display %d\n", displayNum);

    CGDisplayModeRef mode;
    for (i = 0; i < CFArrayGetCount(allModes) && returncode; i++) {
        mode = (CGDisplayModeRef) CFArrayGetValueAtIndex(allModes, i);
        // This formatting is functional but it ought to be done less poorly.
        if (i % MODES_PER_LINE == 0) {
            printf("  ");
        } else {
            printf("\t");
        }
        printf("%lux%lux%lu ",
               CGDisplayModeGetWidth(mode),
               CGDisplayModeGetHeight(mode),
               bitDepth(mode));
        if (i % MODES_PER_LINE == MODES_PER_LINE - 1) {
            printf("\n");
        }
    }
    printf("\n");
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
        fprintf(stderr, "Error: the mode '%s' couldn't be parsed", string);
    } else {
        out->w = w;
        out->h = h;
        out->d = d;
        rc = 1;
    }
    return rc;
}
