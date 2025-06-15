# Seven Segment Display Clock
This is the arduino code for the Open Design Seven Segment Display Clock at Printables: https://www.printables.com/model/1327449-open-design-7-segment-clock

There are two buttons with several function to initialize wifi and to have some display effects

The first button is for setup, the second button is for display effects. Just check the code to see what effects are programmed and maybe add your own.

## Functions of the setup button

### Enter Setup Mode
In setup mode you can set the time and the WiFi connection. You enter setup mode by holding down the button for at least 2 seconds. When the clock is in setup mode the dots will blink. You advance to the next section in the setup by pressing the button again for 2 seconds. When the last setup section is entered, pressing the button for 2 seconds again will exit setup mode.
Setting the Time
Press the button for 2 seconds to enter setup mode. The clock enters setup mode. Every subsequent long press for 2 seconds advances to the next setting:

Hours, Minute, WiFi

Increase hours or minutes by single pressing the button until the right time is displayed.

### Setting the WiFi Credentials
The clock can synchronise time by Internet. To enable this you have to enter the credentials of your WiFi. This can be done on a webpage provided by the clock.

Enter settings mode by pressing the button for two seconds. Switch to WiFi setting mode by long pressing the button for two seconds until WiFi setup is reached, the display shows “AP:0”. 

Press the button once to turn on the WiFi settings mode. The display will show “AP:1”
 
WiFi settings mode is now enabled and a new WiFi access point with the name “DP_7S_CLOCK_01”  will be available. The password for this access point is “12605253”.

Connect to the access point. And open the webpage at “http://setwifi.dpc” or “http://192.168.4.1”

Fill in your WiFi credentials. The credentials will be stored on the clock.

The access point will be available for 15 minutes, or until you exit wifi settings mode by setting the WiFi credentials.

When the clock can connect to WiFi the time will be synchronised via the Internet.

