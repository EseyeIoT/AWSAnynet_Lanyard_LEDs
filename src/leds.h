#ifndef __LEDS_H
#define __LEDS_H

typedef struct rgb_colour{
    unsigned char red, green, blue;
} rgb_colour;
  
void led_init(int dp, int cp);
void ledwrite(rgb_colour *colours, unsigned short count, unsigned char brightness);
void walk_leds(bool clockwise, rgb_colour *leds, int numleds);

#endif
 
