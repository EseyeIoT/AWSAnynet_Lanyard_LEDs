AWS Re:invent Lanyard board ESP32 with Anynet Click

example application supporting button 1 ia SINGLE click and button 2 as DOUBLE click

This application subscribes to leds/<thingname> and lights the lanyard LEDs according to the received json.

message format: 
{ "leds": "255.255.255"} to light all LEDs white
{ "led1": "255,0,0"} to turn LED1 red
