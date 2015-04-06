-- Simple dialog that asks for some MacBook Pro Retina 13" resolutions and sets the display size

-- Avi Alkalay <avi@unix.sh>
-- Mar 2015
-- São Paulo, Brazil

display dialog "Chose Resolution" buttons {"2560×1600", "2048×1280 (big but confortable)", "1650×1050 (highest supported by Apple)"} default button 2

set the button_pressed to the button returned of the result

if the button_pressed is "2048×1280 (big but confortable)" then
	do shell script "/usr/local/bin/screenresolution set 2048x1280x32@0"
else if the button_pressed is "2560×1600" then
	do shell script "/usr/local/bin/screenresolution set 2560x1600x32@0"
else
	do shell script "/usr/local/bin/screenresolution set 1650x1050x32@0"
end if
