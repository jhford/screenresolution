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

#ifndef __CG_UTILS__H
#define __CG_UTILS__H

#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/graphics/IOGraphicsLib.h>

/*
#include <CoreVideo/CVBase.h>
#include <CoreVideo/CVDisplayLink.h>
*/

// http://stackoverflow.com/questions/3060121/core-foundation-equivalent-for-nslog/3062319#3062319
#ifndef __OBJC__
void NSLog(CFStringRef format, ...);
void NSLogv(CFStringRef format, va_list args);
#endif
 
struct config {
    size_t w; // width
    size_t h; // height
    size_t d; // colour depth
    double r; // refresh rate
};

unsigned int setDisplayToMode(CGDirectDisplayID display, CGDisplayModeRef mode);
unsigned int configureDisplay(CGDirectDisplayID display,
                              struct config *config,
                              int displayNum);
unsigned int parseStringConfig(const char *string, struct config *out);
size_t bitDepth(CGDisplayModeRef mode);
CFComparisonResult _compareCFDisplayModes (CGDisplayModeRef *mode1Ptr, CGDisplayModeRef *mode2Ptr, void *context);
char* getPreferredDisplayName(CGDirectDisplayID displayID);

#endif