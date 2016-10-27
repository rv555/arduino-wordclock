# arduino-wordclock
Just another wordclock, based on an Arduino nano with WS2812B LED stripes.
The current layout is german but it is easy to adjust to other languages.

#Functionality
- Displaying the time (obviously)
- Displaying it as mean time (show "five past" from x:02:30 til x:07:29)
- Real-Time-Clock with battery backup
- DCF-77 radio controlled time
- 2 buttons for setup (set time, set brightness, set whether to show "Es ist", set "mean time")
- Config menu with scrolling text

The clock is working for me, but might not be perfect in realization.
Feel free to comment or correct me. I am looking forward to your input.

#Used libraries
- Adafruit_NeoPixel.h - connect WS2812B LED stripes
- Adafruit_NeoMatrix.h - use LED stripe as matrix
- Adafruit_GFX.h - draw lines and text on a matrix
- DS3232RTC.h - Real time clock
- Time.h - time functionality
- Wire.h - I2C for RTC
- DCF77.h - radio controlled clock
- EEPROM.h - save configuration
