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

#include "cg_utils.h"

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
    double pr; // possible refresh rate
    int looking = 1; // used to decide whether to continue looking for modes.
    int i;
    for (i = 0 ; i < CFArrayGetCount(allModes) && looking; i++) {
        possibleMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);
        pw = CGDisplayModeGetWidth(possibleMode);
        ph = CGDisplayModeGetHeight(possibleMode);
        pd = bitDepth(possibleMode);
        pr = CGDisplayModeGetRefreshRate(possibleMode);
        if (pw == config->w &&
            ph == config->h &&
            pd == config->d &&
            pr == config->r) {
            looking = 0; // Stop looking for more modes!
            newMode = possibleMode;
        }
    }
    CFRelease(allModes);
    if (newMode != NULL) {
        NSLog(CFSTR("set mode on display %u to %ux%ux%u@%.0f"), displayNum, pw, ph, pd, pr);
        setDisplayToMode(display,newMode);
    } else {
        NSLog(CFSTR("Error: mode %ux%ux%u@%f not available on display %u"), 
                config->w, config->h, config->d, config->r, displayNum);
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

unsigned int parseStringConfig(const char *string, struct config *out) {
    unsigned int rc;
    size_t w;
    size_t h;
    size_t d;
    double r;
    int numConverted = sscanf(string, "%lux%lux%lu@%lf", &w, &h, &d, &r);
    if (numConverted != 4) {
        numConverted = sscanf(string, "%lux%lux%lu", &w, &h, &d);
        if (numConverted != 3) {
            rc = 0;
            NSLog(CFSTR("Error: the mode '%s' couldn't be parsed"), string);
        } else {
            out->w = w;
            out->h = h;
            out->d = d;
            r=60.0;
            rc = 1;
            NSLog(CFSTR("Warning: no refresh rate specified, assuming %.0lfHz"), r);
        }
    } else {
        out->w = w;
        out->h = h;
        out->d = d;
        out->r = r;
        rc = 1;
    }
    return rc;
}

CFComparisonResult _compareCFDisplayModes (CGDisplayModeRef *mode1Ptr, CGDisplayModeRef *mode2Ptr, void *context)
{
    CGDisplayModeRef mode1 = (CGDisplayModeRef)mode1Ptr;
    CGDisplayModeRef mode2 = (CGDisplayModeRef)mode2Ptr;

    size_t width1 = CGDisplayModeGetWidth(mode1);
    size_t width2 = CGDisplayModeGetWidth(mode2);
    
    if(width1 == width2) {
        size_t height1 = CGDisplayModeGetWidth(mode1);
        size_t height2 = CGDisplayModeGetWidth(mode2);
        
        if(height1 == height2) {
            size_t refreshRate1 = CGDisplayModeGetRefreshRate(mode1);
            size_t refreshRate2 = CGDisplayModeGetRefreshRate(mode2);
            
            if(refreshRate1 == refreshRate2) {
                long bitDepth1 = bitDepth(mode1);
                long bitDepth2 = bitDepth(mode2);
                
                if(bitDepth1 == bitDepth2)
                    return kCFCompareEqualTo;
                else
                    return (bitDepth1 < bitDepth2) ? kCFCompareLessThan : kCFCompareGreaterThan;
            }
            else 
                return (refreshRate1 < refreshRate2) ? kCFCompareLessThan : kCFCompareGreaterThan;
        }
        else 
            return (height1 < height2) ? kCFCompareLessThan : kCFCompareGreaterThan;
    }
    else 
        return (width1 < width2) ? kCFCompareLessThan : kCFCompareGreaterThan;
}

char* convertCFStringToCString(CFStringRef toConvert)
{   
    char* toReturn = "";

    if (NULL != toConvert) 
    {
        CFIndex length = CFStringGetLength(toConvert);
        CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
        char *buffer = (char *)malloc(maxSize);

        if (CFStringGetCString(toConvert, buffer, maxSize, kCFStringEncodingUTF8)) 
        {
            toReturn = buffer;
        }
    }

    return toReturn;
}

char* getPreferredDisplayName(CGDirectDisplayID displayID)
{
    char* name = "Unknown";

    //TODO: 'CGDisplayIOServicePort' is deprecated (but still available) in OS X 10.9
    // I believe something else will come out by the time it is fully deprecated...or
    // Apple will document a way of getting this additional display info while using
    // CoreGraphics.
    io_service_t displayServicePort = CGDisplayIOServicePort(displayID);

    if (displayServicePort)
    {   
        CFDictionaryRef displayInfoDict = IODisplayCreateInfoDictionary(displayServicePort, kIODisplayOnlyPreferredName); 

        if(displayInfoDict)
        {
            // this array will be populated with the localized names for the display (i.e. names of the
            // display in different languages)
            CFDictionaryRef namesForDisplay = CFDictionaryGetValue(displayInfoDict, CFSTR(kDisplayProductName));        
            CFStringRef value;

            if (namesForDisplay)
            {
                // TODO: get this working with system's default locale...the rest of the program is in English
                // for now, so it's not an utterly obtuse decision to stick with English.
                name = convertCFStringToCString(CFDictionaryGetValue(namesForDisplay, CFSTR("en_US")));       
            }     

            CFRelease(displayInfoDict);
        }

        IOObjectRelease(displayServicePort);
    }

    return strdup(name);
}