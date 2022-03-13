# cooler-ruler
Arduino ESP32 Kool-bot substitute


#Background

I'm a beginner at microcontrollers, but familiar with electricity, and I enjoy being frugal especially if it will help others.  Farmer friends had been elated at Cool-Bots, which costs $350 and turns an air conditioner into a cooler.  I looked at what these products did and was surprised at the cost for how little it is.  I set out to do it cheaper, with greater functionality.

My farmer friend spent the winter using old cooler panels (free) and making an insulated room in an unheated basement.  There was a little window to the outside, and he did a great job installing the door.  So there is our walk-in cooler.

#Features

Setpoint adjustable by console buttons
Wifi transmission of data to ThingSpeak cloud server
Alarms for too hot or too cold
Humidity measurement
Outside air temperature measurement
Inside air temperature measurement
Defrost mode if fins freeze up
Graphing display and messages

#Materials

Embarking on my first microprocessor project, I had to buy a quantity of basic parts like wires, resistors, and buttons.  So the cost of my second one would be less than the first.  I also learned lessons that I'll pass on.  But for the sake of being able to create a workable project, I'll describe the materials I used:

1. HiLetGo ESP32 LoRa Wifi Kit 32 board with OLED display.  Bought on Amazon for $18.49.  In hindsight, I'd probably go with a separate display.  Also, it was a little work to find drivers for this board, which will be the case with any board.  And finally, you have to solder the headers onto this board, which is some pretty precise soldering close to plastic ribbon cables.  I bought flux and was careful, but this was seemingly an unnecessary hardship.
2. Hypertronix 115mmx90mmx55m IP-65 Waterproof Enclosure with breadboard.  Bought on Amazon for $15.95.  This was a great buy.  It has connectors for the wires, and a waterproof gasket, and was easy to access.
3. Pack of 5 Thermal Sensors (DS18B20) Amazon $9.99
4. Humidity Sensor (DHT22) Amazon $6.99
5. Pack of 10 100-ohm 5-watt resistors Amazon $6.49
6. 10A 240V relay 2pack - Amazon $7.99
6. Resistor kit - Amazon $7.49
7. Wire kit.  Elegoo 120pcs Wire Kit Amazon $6.98
8. Momentary Buttons of various colors, 10pk - Amazon $8.99 
9. Speaker - I found a speaker in an old Dell computer that had an amplifier on it.  Before I found it, I bought a 5-pack of LM386 audio amplifiers on ebay.  I could have just used that to hook up to any simple speaker. FREE
10. Fresnel lens - I liked the project box and the board, but I wanted to use the display and see it through the box.  So I used a dremel to cut out a hole in the box, directly above the board, then superglued a fresnel lens over the hole.  It magnified the display nicely and you could read it just fine.  So it worked well, but I think I would have prefered to have the display mounted to the cover.  I had to get in and out of the project box many times to fix things. Ebay $1.64
11. 10cc Syringe Solder Paste Flux (MECHANIC XG-Z40) $6.31
12. A 19V 1A powersupply - Goodwill $2.00

TOTAL INITIAL COST = $107

I bought a MCIGICM 400 Points Solderless Breadboard which I used to play with.  You don't solder anything, just push it in.  It works nicely with the wires I bought.

I learned that the ESP32 is superior to the ESP8266, being newer.  Also I learned that there are many configurations of the ESP32 microprocessor, so depending on what board you get, your available input/output pins will be different.  So there may be a better board out there for this project, but you will have to do the research to find out what pins to use.  And I'll mention again that if you get a different board, try to find one with headers soldered on already.

#Concepts
I learned many concepts, and there are helpful tutorials out there to explain these to a first-timer like me.  I'll just mention them here:
1. Buttons need pull-up resistors.  4.7K was recommended, but I didn't have.  10K worked fine.
2. Buttons need to be debounced to work correctly.
3. I took wave files and converted them to C code.  The ESP32 Audio Player plays them nicely.  My buttons made a sound when pressed, and an alarm.
4. ThingSpeak has a free account and you can connect your microprocessor to a wireless network and send your data to ThingSpeak.
5. The CoolerRuler works by turning on the relay to the 100-ohm resistors which are placed next to the temperature sensor for the air conditioner.  It usually sits in front of the fins.
6. A 19V powersupply with 3 x 100-ohm resistors in series worked very well.  You could use 12V with 2 resistors, and maybe 5V with 1.  This gets the temperature of the heater up to around 80F. 


#My Pins:
Temp Sensor - 17
Heater - 2
Red button (set point up) -12
Blue button (set point down) - 13
Humidity - 21
Audio - 25


#References
Much of the initial understanding came from the ColdSnap project.
https://github.com/vgeorge/coldsnap
https://web.archive.org/web/20160211232038/http://people.umass.edu:80/~dac/projects/ColdSnap/ColdSnap.html
https://farmhack.org/archive/coldsnap-air-conditioner-override-controller
https://www.raspberrypi.org/forums/viewtopic.php?t=141686
https://cdn.hackaday.io/files/269911154782944/Heltec_WIFI-LoRa-32_DiagramPinout.jpg
https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
https://github.com/gadjetnut/heltec-esp32-mqtt
https://randomnerdtutorials.com/esp32-multiple-ds18b20-temperature-sensors/
https://randomnerdtutorials.com/esp32-ds18b20-temperature-arduino-ide/
https://randomnerdtutorials.com/esp32-thingspeak-publish-arduino/
https://circuitdigest.com/microcontroller-projects/esp32-based-audio-player
