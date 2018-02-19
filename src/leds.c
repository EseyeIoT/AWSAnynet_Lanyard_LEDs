#include <stdio.h>
#include "common/mbuf.h"
#include "common/platform.h"
#include "mgos_app.h"
#include "mgos_gpio.h"
#include "mgos_timers.h"
#include "mgos.h"
#include "leds.h"

static int dataPin, clockPin;

static void init(){
    mgos_gpio_write(dataPin, false);
    mgos_gpio_set_mode(dataPin, MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_write(clockPin, false);
    mgos_gpio_set_mode(clockPin, MGOS_GPIO_MODE_OUTPUT);
}

/* Shift out a byte */
static void transfer(uint8_t b){
    mgos_gpio_write(dataPin, b >> 7 & 1);
    mgos_gpio_write(clockPin, true);
    mgos_gpio_write(clockPin, false);
    mgos_gpio_write(dataPin, b >> 6 & 1);
    mgos_gpio_write(clockPin, true);
    mgos_gpio_write(clockPin, false);
    mgos_gpio_write(dataPin, b >> 5 & 1);
    mgos_gpio_write(clockPin, true);
    mgos_gpio_write(clockPin, false);
    mgos_gpio_write(dataPin, b >> 4 & 1);
    mgos_gpio_write(clockPin, true);
    mgos_gpio_write(clockPin, false);
    mgos_gpio_write(dataPin, b >> 3 & 1);
    mgos_gpio_write(clockPin, true);
    mgos_gpio_write(clockPin, false);
    mgos_gpio_write(dataPin, b >> 2 & 1);
    mgos_gpio_write(clockPin, true);
    mgos_gpio_write(clockPin, false);
    mgos_gpio_write(dataPin, b >> 1 & 1);
    mgos_gpio_write(clockPin, true);
    mgos_gpio_write(clockPin, false);
    mgos_gpio_write(dataPin, b >> 0 & 1);
    mgos_gpio_write(clockPin, true);
    mgos_gpio_write(clockPin, false);
}

/* Shift out 4 zeros to start */
static void startFrame(){
    init();
    transfer(0);
    transfer(0);
    transfer(0);
    transfer(0);
}

static void endFrame(unsigned short count){
    transfer(0xFF);
    for (unsigned short i = 0; i < 5 + count / 16; i++){
        transfer(0);
    }
}

/* Shift out a single LED brightness/RGB */
static void sendRGBColour(unsigned char red, unsigned char green, unsigned char blue, unsigned char brightness){
    transfer(0b11100000 | brightness);
    transfer(blue);
    transfer(green);
    transfer(red);
}

static void sendColour(rgb_colour colour, unsigned char brightness){
    sendRGBColour(colour.red, colour.green, colour.blue, brightness);
}

void led_init(int dp, int cp){
    dataPin = dp;
    clockPin = cp;
}

void ledwrite(rgb_colour *colours, unsigned short count, unsigned char brightness){
    startFrame();
    for(unsigned short i = 0; i < count; i++){
      sendColour(colours[i], brightness);
    }
    endFrame(count);
}

/* Function to walk the LED colours anti-clockwise */
void walk_leds(bool clockwise, rgb_colour *leds, int numleds){
  int i, first, inc, last;
  rgb_colour ledstore;
  if(clockwise){
    first = numleds - 1;
    last = 0;
    inc = -1;
  }else{
    first = 0;
    last = numleds - 1;
    inc = 1;
  }
  i = first;
  memcpy(&ledstore, &leds[first], sizeof(rgb_colour));
  while(i != last){
    memcpy(&leds[i], &leds[i+inc], sizeof(rgb_colour));
    i += inc;
  }
  memcpy(&leds[last], &ledstore, sizeof(rgb_colour));
  ledwrite(leds, numleds, 31);
} 

