# Screen Resolution

This is a tool that can be used to determine current resolution,
list available resolutions and set resolutions for active displays
on Mac OS 10.6, and possibly above.  I have only tested 10.6.

I used clang for development, but the code seems to compile
just fine with gcc.  The code might not be as well layed out
as it could be, feel free to send a pull request.

### Build+Install

Running the following commands in Terminal.app will result in 
dmg with a pkg file being created if the system has Xcode 4.
    
    git clone github.com:jhford/screenresolution
    cd screenresolution
    make dmg

At this point, I'd recommend testing that things work!  I have
written a 'test' makefile target.  Because this script expects two
monitors that both use the same resolution, it mightn't work 
properly for you if you only have one

    make test ORIG_RES=1920x1200x32

This will cause your screen to flicker as it changes the mode a
couple times on each monitor.

The makefiles support the `DESTDIR` (alternate root) and `PREFIX` 
variables.  If you don't know what those are, you probably don't
want them.

This will create a DMG file and a PKG file.  If you know or care 
about the differences, you probably know what to do at this point.
If you want to install this program on the system you built it on,
you can run 
    
    open screenresolution.pkg

If you want to put this program on another system, you can choose
between the pkg file, the dmg file, the binaries or use the 
install make target with DESTDIR to specify an alternate root.

### Running

There are three commands that this program supports: `get`, `list` 
and `set`.  All three modes operate on active displays [1].

The `get` mode will show you the resolution of all active displays

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

The `set` command takes a list of modes.  It will apply the modes
in the list of modes to the list of displays, starting with 0.
Modes in excess of the number of active displays will be ignored.
If you wish to set a monitor but not the lower numbered displays,
there is a keyword `skip` which can be subsituted for a resolution.
This keyword will cause the first display to be skipped.  If you
specify more resolutions than you have active screens, the extra
resolutions will be ignored.


## Examples

__Example 1:__
>
    This example works with one or more screens
    $ screenresolution set 800x600x32

>Result:
>
    The main display will change to 800x600x32, second screen
    will not be changed

__Example 2:__
>
This example assumes two screens
>
    $ screenresolution set 800x600x32 800x600x32

>__Result:__
>
    The first and second monitor on the system will be set to 
    800x600x32

__Example 3:__

>This example assumes two screens
>
    $ screen resolution set skip 800x600x32

>__Result:__
>
    This will not touch the first screen but will set the second
    screen to 800x600x32

___

[1]See discussion point for explanation of what active display means.
http://developer.apple.com/library/mac/documentation/GraphicsImaging/Reference/Quartz_Services_Ref/Reference/reference.html#//apple_ref/c/func/CGGetActiveDisplayList
