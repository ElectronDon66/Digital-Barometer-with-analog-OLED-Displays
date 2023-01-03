# Digital-Barometer-with-analog-OLED-Displays
Precision Barometer with analog meter display and 10 Hr histogram 
This is an atmospheric air pressure barometer plus a 10 hour history on a cartesian plot display. The cartesian display is normalized to the current reading for higher resolution, It is +/- 0.2 in/Hg. It uses two OLED SSD1306 displays One display is the instantaneous Barometric pressure that updates once a second. 2nd Display is a cartesian plot of the relative pressure change versus time (Hrs). It updates once an hour. It has 10 hrs history. the history reads left to right with the right side of the history being the oldest reading and the left side being the most recent. Four LEDs indicate if pressure is going up(Green LED),down(Red LED), steady (yellow LED in combination with either red or green LEDs depending on last change) ) and if the change is > .06 in/hg (Blue LED also in combination with either the red or green LED). The LEDs are updated at the same time as the history/cartesian plot.
Pressure is in inches of mercury It requires a Arduino Mega 2560 processor Several library files are needed to support the hardware and graphics plotting A potentiometer (trimpot) is included in the hardware to adjust for the local altitude offset. To adjust for altitude offset just get the local sea level barometric reading such as from a local airport or your smart phone and adjust the trim pot for the same reading. The needed library files are as follows: #include <Adafruit_SSD1306.h> #include <splash.h> #include "U8glib.h" #include <math.h> #include <Adafruit_BMP085.h> //Also runs BMP180 #include <Adafruit_GFX.h> find and copy these files to your Arduino library The program has been debugged and runs on the an EGEOO Mega 2560 which is supposed to be the same as an Arduino Mega There is an associated Youtube video showing operation and a finished project where a temperature and humidity readout were added. These will be covered in a seperate project. Special thanks to Kris Kasprzak for his OLED graphics code which were used here. END / DEW