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

    /*! Initializes the I/O lines and sends a "Start Frame" signal to the LED
     *  strip.
     *
     * This is part of the low-level interface provided by this class, which
     * allows you to send LED colors as you are computing them instead of
     * storing them in an array.  To use the low-level interface, first call
     * startFrame(), then call sendColor() some number of times, then call
     * endFrame(). */
static void startFrame(){
    init();
    transfer(0);
    transfer(0);
    transfer(0);
    transfer(0);
}

    /*! Sends an "End Frame" signal to the LED strip.  This is the last step in
     * updating the LED strip if you are using the low-level interface described
     * in the startFrame() documentation.
     *
     * After this function returns, the clock and data lines will both be
     * outputs that are driving low.  This makes it easier to use one clock pin
     * to control multiple LED strips. */
static void endFrame(unsigned short count){
      // We need to send some more bytes to ensure that all the LEDs in the
      // chain see their new color and start displaying it.
      //
      // The data stream seen by the last LED in the chain will be delayed by
      // (count - 1) clock edges, because each LED before it inverts the clock
      // line and delays the data by one clock edge.  Therefore, to make sure
      // the last LED actually receives the data we wrote, the number of extra
      // edges we send at the end of the frame must be at least (count - 1).
      // For the APA102C, that is sufficient.
      //
      // The SK9822 only updates after it sees 32 zero bits followed by one more
      // rising edge.  To avoid having the update time depend on the color of
      // the last LED, we send a dummy 0xFF byte.  (Unfortunately, this means
      // that partial updates of the beginning of an LED strip are not possible;
      // the LED after the last one you are trying to update will be black.)
      // After that, to ensure that the last LED in the chain sees 32 zero bits
      // and a rising edge, we need to send at least 65 + (count - 1) edges.  It
      // is sufficent and simpler to just send (5 + count/16) bytes of zeros.
      //
      // We are ignoring the specification for the end frame in the APA102/SK9822
      // datasheets because it does not actually ensure that all the LEDs will
      // start displaying their new colors right away.

    transfer(0xFF);
    for (unsigned short i = 0; i < 5 + count / 16; i++){
        transfer(0);
    }
}

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
