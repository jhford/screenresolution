This is a set of tools (GUI and command line) that let you use MacBook Pro Retina's
highest and unsupported resolutions. As an example, a Retina MacBook Pro 13" can be set
to 2560×1600 maximum resolution, as opposed to Apple's max supported 1650×1050.

![chose resolution](https://cloud.githubusercontent.com/assets/3484242/7044411/8c3d8532-ddc9-11e4-85fc-5301aee68b40.png)

The SwitchResolution GUI app will show you the 3 highest resolutions to chose. While
the screenresolution command lets you list available resolutions get current and set a
new one. They were all tested on Yosemite.

The screenresolution command was developed in C, while the SwitchResolution app is a
very simple AppleScript compiled into an app.

MacBook Pro Retina machines always use the maximum hardware resolution. The screen
resolution manipulated by this tools is what is actually presented to apps. This way text,
widgets, menus and screen real estate are increased an decreased for optimal use.  

Fast change resolution with SwitchResolution
============================================

The SwitchResolution app purpose is to give fast one-click access to highest resolutions.
Best way to use it is to fix it on your dock because you will find that different
resolutions fit better for different kinds of work. You will prefer highest resolutions
when working with many windows and lower resolutions when simply reading text.

Running the command line tool
=============================
There are three commands that this program supports: get, list 
and set.  All three modes operate on active displays [1].

The get mode will show you the resolution of all active displays

    $ screenresolution get
    Display 0: 1920x1200x32
    Display 1: 1920x1200x32
 
 The list mode will show you to the available resolutions of all
 active displays, seperated by various whitespace.

    Available Modes on Display 0
      1920x1200x8   1920x1200x16    1920x1200x32    960x600x8 
      960x600x16    960x600x32      1680x1050x8 	1680x1050x16 
    <snip>
    Available Modes on Display 1
    <snip>

The set command takes a list of modes.  It will apply the modes
in the list of modes to the list of displays, starting with 0.
Modes in excess of the number of active displays will be ignored.
If you wish to set a monitor but not the lower numbered displays,
there is a keyword 'skip' which can be subsituted for a resolution.
This keyword will cause the first display to be skipped.  If you
specify more resolutions than you have active screens, the extra
resolutions will be ignored.

This example works with one or more screens
    $ screenresolution set 800x600x32
Result 1:
    The main display will change to 800x600x32, second screen
    will not be changed

This example assumes two screens
    $ screenresolution set 800x600x32 800x600x32
Result 2:
    The first and second monitor on the system will be set to 
    800x600x32

This example assumes two screens
    $ screen resolution set skip 800x600x32
This will not touch the first screen but will set the second
    screen to 800x600x32

Build+Install
====================
Running the following commands will result in
dmg with a pkg file being created if the system has Xcode 4.
    
    git clone github.com:avibrazil/screenresolution
    cd screenresolution
    make dmg
    
or, change the last command to a more economical

	make pkg

At this point, I'd recommend testing that things work!  I have
written a 'test' makefile target.  Because this script expects two
monitors that both use the same resolution, it mightn't work 
properly for you if you only have one

    make test ORIG_RES=1920x1200x32

This will cause your screen to flicker as it changes the mode a
couple times on each monitor.

The makefiles support the DESTDIR (alternate root) and PREFIX 
variables.  If you don't know what those are, you probably don't
want them.

This will create a DMG file and a PKG file.  If you know or care 
about the differences, you probably know what to do at this point.
If you want to install this program on the system you built it on,
you can run 
    
    open screenresolution.pkg

Or simply double click on the PKG file to install it on your system.

[1]See discussion point for explanation of what active display means.
http://developer.apple.com/library/mac/documentation/GraphicsImaging/Reference/Quartz_Services_Ref/Reference/reference.html#//apple_ref/c/func/CGGetActiveDisplayList
