# WeatherLED

A C++ Programm that enables an esp8266 to controll an led strip via FastLED. 
Deceloped with Platform IO
It has different modes that are based on weather.

You can switch between the LED Modes via http requests on the http server setup on the esp8266.
The standard mode is the "scrape" mode. In this mode the Microcontroller will scrape the current weather in th einput location every 15 mins and change the led mode accordingly.

It is also possible to change the mode to one specific weather condition. The scraper will be disabled automatically until the mode is switched to scrape again.

The current weather modes are:

Rain
Snow
Clear
Clouds
[more coming soon]