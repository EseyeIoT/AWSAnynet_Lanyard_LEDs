/*
*  Application to send IoT Button Message to AWS IoT Service from Anynet Click
*
*/
#include <stdio.h>
#include "common/mbuf.h"
#include "common/platform.h"
#include "mgos_app.h"
#include "mgos_gpio.h"
#include "mgos_timers.h"
#include "mgos_uart.h"
#include "mgos.h"
#include "leds.h"

#define UART_NO 1

/* To use the LEDs */
rgb_colour leds[6];

int state =0;

// State machine called on a timer
static void timer_cb(void *arg) {

  // First increment the state
  state++;
       
  switch (state){
    case 1 :
    printf("Visit: www.eseye.com/aws_dev_kit/ to find out more\r\n");
    printf("Publish to topic leds {\"leds\": \"255.255.255\"}");
    mgos_uart_printf(UART_NO,"at+awsver\r\n");
    break;

    case 2 :
//   printf("request Software version\r\n");
    mgos_uart_printf(UART_NO,"at+awsver\r\n");
    break;

   case 3 :
//     printf("SIM number\r\n");
    mgos_uart_printf(UART_NO,"at+QCCID\r\n");
    break;

   case 4 :
//    printf("IMEI\r\n");
    mgos_uart_printf(UART_NO,"at+GSN\r\n");
    break;

   case 5 :
    mgos_uart_printf(UART_NO,"at+awssubopen=0,\"leds\"\r\n");
    break;

   default :
//    show that the code is running with a dot on the console
    //printf(".\r\n");
    break;

    case 10 :
// print a ! to show that the console is actually scrolling
    //printf("!\r\n");
    state= 6;
  }
  (void) arg;
}

static void led_timer_cb(void *arg) {
  /* Walk the LEDs clockwise */
  walk_leds(true, leds, 6);
  (void) arg;
}

//callback used on a  press of USER BTN 1
static void button_cb(int pin, void *arg) {
  int button = *(int *)arg;
  if(button == 0)
    mgos_uart_printf(UART_NO,"at+awsbutton=SINGLE\r\n");
  else
    mgos_uart_printf(UART_NO,"at+awsbutton=DOUBLE\r\n");
    
  (void) arg;
}

int esp32_uart_rx_fifo_len(int uart_no);

static void decode_msg(char *buf, int subidx){
  if(subidx == 0){
    /* Look for the led suffix (s for all 0-5 for individual) */
    char *elem = strstr(buf, "led");
    int whichled;
        
    if(isdigit((int)elem[3]))
        whichled = strtol(&elem[3], NULL, 10);
    else
        whichled = -1;
    /* Skip to the values */
    elem += 4;
    if(elem != NULL){
      int red = 0, green = 0, blue = 0;
      char *next;
      while(!isdigit((int)*elem))
          elem++;
      /* Get red */
      red = strtol(elem, &next, 10);
      if(next){
        while(!isdigit((int)*next))
          next++;
        elem = next;
        /* Get green */
        green = strtol(elem, &next, 10);
      }
      if(next){
        while(!isdigit((int)*next))
          next++;
        elem = next;
        /* Get blue */
        blue = strtol(elem, &next, 10);
      }
      /* Set individual or all led(s) */
      if(whichled != -1){
        printf("set LED %d to %d,%d,%d\n", whichled, red, green, blue);
        leds[whichled].red = red;
        leds[whichled].green = green;
        leds[whichled].blue = blue;
      }else{
        int count = 0;
        printf("set all LEDs to %d,%d,%d\n", red, green, blue);
        while(count < 6){
          leds[count].red = red;
          leds[count].green = green;
          leds[count].blue = blue;
          count++;
        }
      }
      ledwrite(leds, 6, 31);
    }
  }
}

static int topicrx = -1;
static int rxlen = -1;
static struct mbuf awsbuffer;

//UART Dispatcher callback Echoes anyting from the Click Module to the Console
static void uart_dispatcher(int uart_no, void *arg) {
  assert(uart_no == UART_NO);
  size_t rx_av = mgos_uart_read_avail(uart_no);

  if (rx_av > 0) {
    struct mbuf rxb;
    mbuf_init(&rxb, 0);
    mgos_uart_read_mbuf(uart_no, &rxb, rx_av);

    if (rxb.len > 0) {
      char *rxbuf = rxb.buf;
      int rxidx = 0;
      char *findaws;
      
      printf("%.*s", (int) rxb.len, rxb.buf);
      
      findaws = strstr(rxbuf, "AWS:");
      if(findaws != NULL){
        rxidx = (int)findaws - (int)rxb.buf;
        rxidx += 4;
        topicrx = strtol(&rxbuf[rxidx], NULL, 10);
        rxidx += 2;
      }
      if(topicrx != -1 && rxlen == -1){
        char *numend;
        if(rxidx < rxb.len){
          rxlen = strtol(&rxbuf[rxidx], &numend, 10);
          rxidx += ((int)numend - (int)&rxbuf[rxidx]);
        }
      }
      if(rxlen > 0){
        if(rxlen <= (rxb.len - rxidx)){
          mbuf_append(&awsbuffer, &rxbuf[rxidx], rxlen);
          rxlen = 0;
        }else{
          mbuf_append(&awsbuffer, &rxbuf[rxidx], rxb.len - rxidx);
          rxlen -= (rxb.len - rxidx);
        }
      }
      if(rxlen == 0){
        mbuf_append(&awsbuffer, "\0", 1);
        decode_msg(awsbuffer.buf, topicrx);
        mbuf_free(&awsbuffer);
        mbuf_init(&awsbuffer, 0);
        topicrx = -1;
        rxlen = -1;
      }
    }
    mbuf_free(&rxb);
  }
  (void) arg;
}

 
int ub1 = 0;
int ub2 = 1;

enum mgos_app_init_result mgos_app_init(void) {

  struct mgos_uart_config ucfg;

  leds[0].red = 255;
  leds[0].green = 0;
  leds[0].blue = 0;
  leds[1].red = 0;
  leds[1].green = 255;
  leds[1].blue = 0;
  leds[2].red = 0;
  leds[2].green = 0;
  leds[2].blue = 255;
  leds[3].red = 255;
  leds[3].green = 255;
  leds[3].blue = 0;
  leds[4].red = 0;
  leds[4].green = 255;
  leds[4].blue = 255;
  leds[5].red = 255;
  leds[5].green = 0;
  leds[5].blue = 255;
  
  led_init(16, 14);
  ledwrite(leds, 6, 31);

  /* Init receive/collate buffer */
  mbuf_init(&awsbuffer, 0);

  mgos_uart_config_set_defaults(UART_NO, &ucfg);

  /*
   * the Anynet Click Module uses 9600 8n1 so overwrite the defaults
   */

  ucfg.baud_rate = 9600;
  ucfg.num_data_bits = 8;
  ucfg.parity = MGOS_UART_PARITY_NONE;
  ucfg.stop_bits = MGOS_UART_STOP_BITS_1;
  if (!mgos_uart_configure(UART_NO, &ucfg)) {
    return MGOS_APP_INIT_ERROR;
  }

   /* register callback to send IoT Button Message on Press of  */
  mgos_gpio_set_button_handler(32,MGOS_GPIO_PULL_UP, MGOS_GPIO_INT_EDGE_NEG, 500, button_cb, (void *)&ub1);
  mgos_gpio_set_button_handler(33,MGOS_GPIO_PULL_UP, MGOS_GPIO_INT_EDGE_NEG, 500, button_cb, (void *)&ub2);

  state = 0; // initialised state

  //register a callback every 2 seconds. Used to interogate Click module and to show heartbeat
  mgos_set_timer(2000 /* ms */, true /* repeat */, timer_cb, NULL /* arg */);
  mgos_set_timer(200, true, led_timer_cb, NULL);

  //register a callback for data bytes received from the click module.
  mgos_uart_set_dispatcher(UART_NO, uart_dispatcher, NULL /* arg */);

// and enable the receiver on this UART
  mgos_uart_set_rx_enabled(UART_NO, true);

//  printf("send an at to show the module is connected\r\n");
  mgos_uart_printf(UART_NO,"at\r\n");
  
  return MGOS_APP_INIT_SUCCESS;
}
 
