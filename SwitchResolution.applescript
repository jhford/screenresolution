-- Simple dialog that asks for some MacBook Pro Retina 13" resolutions and sets the display size

-- Avi Alkalay <avi@unix.sh>
-- Mar 2015
-- São Paulo, Brazil

set ScreenResolutionCommand to "/usr/bin/screenresolution"

set AppleScript's text item delimiters to "|"
set res to text items of (do shell script ScreenResolutionCommand & " list 2>/dev/null | grep x | sed -e 's/x /x/g' | xargs echo | tr ' ' '\n' | sort -run | head -3 | perl -e '$i=0; while (<>) {$i++; chop; m^(.*)x(.*)x(.*)^; print \"|\" if ($i>1); print \"$1×$2^$_\"};'")

set AppleScript's text item delimiters to "^"
set resolutions to {}
repeat with i in res
	set r to text items of i
	set end of resolutions to r
end repeat

set names to {}
repeat with i in resolutions
	set thename to item 1 of i
	set end of names to thename
end repeat

-- choose from list names with prompt "Choose from the list."

display dialog "Chose Resolution" buttons names

set the button_pressed to the button returned of the result

repeat with i in resolutions
	set thename to item 1 of i
	set theresolution to item 2 of i
	if the button_pressed is thename then
		do shell script ScreenResolutionCommand & " set " & theresolution
	end if
end repeat