# Weather-LED

A C++ Programm that enables an esp8266 to controll an led strip via FastLED and scrape weather data in order to play different modes depending on the current weather. 
Developed with Platform IO

You can switch between the LED Modes via http requests on the http server setup on the esp8266.
The standard mode is the "scrape" mode. In this mode the Microcontroller will scrape the current weather in the input location every 15 mins and change the led mode accordingly.

It is also possible to change the mode to one specific weather condition. The scraper will be disabled automatically until the mode is switched to scrape again.

The current weather modes are:
- Clear
- Clouds
- Rain
- Snow
- Thunderstorm