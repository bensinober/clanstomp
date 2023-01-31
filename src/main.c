#include "lcd/lcd.h"
#include "gd32vf103.h"
//#include "gd32vf103v_eval.h"
#include "systick.h"
#include <stdio.h>

#include "drv_usb_hw.h"
#include "usbd_core.h"
#include "midi_usb_core.h"
#include "midi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gd32v_pjt_include.h"

/*
  LilyGo TTGO T-Display GD32 Risc-V processor using usb
  borrowed USB integration from https://github.com/linusreM/GD32V-Virtual-COM-Port
  USB midi borrowed from https://github.com/nyannkov/nano_midi
  USB MIDI specs here: https://www.usb.org/sites/default/files/USB%20MIDI%20v2_0.pdf
*/

extern uint8_t packet_sent, packet_receive;
extern uint32_t receive_length;

usb_core_driver USB_OTG_dev = {
    .dev = {
        .desc = {
            .dev_desc       = (uint8_t *)&device_descriptor,
            .config_desc    = (uint8_t *)&configuration_descriptor,
            .strings        = usbd_strings,
        }
    }
};

void setup();
void loop();

// usb midi msg 4 bytes:
// first byte:  iJack input  ~ code index number (0x8 - note off)
// second byte: midi command ~ CIN + Channel (channel 1 is 0)
// iJack,CIN, cmd,chan   note  velocity
// 0x3   0x9, 0x9 0x0,     0x3c  0x40 - note on  - {0x09, 0x90, 0x48, 0x40}
// 0x3   0x8, 0x8 0x0,     0x3c, 0x40 - note off - {0x08, 0x80, 0x48, 0x00}

unsigned char noteOn[NOTE_SIZE]  = {0x09, 0x90, 0x48, 0x40};
unsigned char noteOff[NOTE_SIZE] = {0x08, 0x80, 0x48, 0x00};
int midiBufIdx = 0;

//unsigned char image[12800]; // Used in LCD_ShowPicture(u16 x1, u16 y1, u16 x2, u16 y2)

// Keyboard
typedef struct {
  char* noteString;  // string repr of note
  uint note;         // byte code for midi note
  int gpioSet;       // Set of GPIO pins (A,B,C)
  int gPin;          // GPIO pin number
  int prevState;     // State holder for sensing keypress
  int midiState;     // State holder for midi on/off
} Key[10];

  Key keys = {
    //{"X", 0x15, GPIOA, GPIO_PIN_4, 0},
    {"G1", 0x1f, GPIOB, GPIO_PIN_9, 0},  // 43
    {"A1", 0x21, GPIOB, GPIO_PIN_8, 0},  // 45
    {"B1", 0x22, GPIOB, GPIO_PIN_7, 0},  // 47
    {"C2", 0x24, GPIOB, GPIO_PIN_6, 0},  // 48
    {"D2", 0x26, GPIOB, GPIO_PIN_5, 0},  // 50
    {"E2", 0x28, GPIOB, GPIO_PIN_4, 0},  // 52
    {"F2", 0x29, GPIOC, GPIO_PIN_15, 0}, // 53
    {"G2", 0x2b, GPIOC, GPIO_PIN_14, 0}, // 55
    {"A2", 0x2d, GPIOC, GPIO_PIN_13, 0}, // 57
  };
  int keySize = sizeof(keys) / sizeof(keys[0]);

int main(void) {
  setup();
  loop();
}


void setup() {

  /* SETUP GPIO */
  rcu_periph_clock_enable(RCU_GPIOA);
  rcu_periph_clock_enable(RCU_GPIOB);
  rcu_periph_clock_enable(RCU_GPIOC);
  /* Setup LEDS */
  gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13);
  gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1 | GPIO_PIN_2);
  gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
  gpio_bit_set(GPIOB, GPIO_PIN_10);

  // Activate back leds
  LEDR(1);
  LEDG(1);
  LEDB(1);

  /* Setup Display */
  Lcd_Init();
  LCD_Clear(BLACK);
  BACK_COLOR = BLACK;
  setRotation(3);


  // Initiate clanboard pins
  int i;
  //gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_4); // BtnA
  for (i=0;i < keySize;i++) {
    gpio_init(keys[i].gpioSet, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, keys[i].gPin);
  }
  // USB init
  eclic_global_interrupt_enable();
  eclic_priority_group_set(ECLIC_PRIGROUP_LEVEL2_PRIO2);
  LCD_ShowString(0,  0, (u8 *)("Wait for USB"), WHITE);
  usb_rcu_config();
  usb_timer_init();
  usb_intr_config();
  usbd_init(&USB_OTG_dev, USB_CORE_ENUM_FS, &usbd_midi_cb);
  midi_usb_init(&USB_OTG_dev, 0);

  // check if USB device is enumerated successfully
  while (USBD_CONFIGURED != USB_OTG_dev.dev.cur_status) {}
  LCD_ShowString(0, 64, (u8 *)("configured!"), BRED);

}

void loop() {

  LCD_ShowString(0,  0, (u8 *)("CLaNSToMP    "), WHITE);
  while (1) {
    // Could use: gpio_input_bit_get(GPIOA, GPIO_PIN_4)
    int i;
    // iterate keys and check for changes
    for (i=0; i < keySize; i++) {
      int state = gpio_input_bit_get(keys[i].gpioSet, keys[i].gPin);
      // button state, toggle!
      if (keys[i].prevState != state) {
        if (keys[i].prevState == 0) {
          // we only trigger midi change on pressed key
          LCD_ShowString(0,  32, (u8 *)(keys[i].noteString), BRED);
          if (keys[i].midiState == 0) {
            noteOn[2] = keys[i].note;
            queueMidiMsg(noteOn);
            keys[i].midiState = 1;
          } else {
            noteOff[2] = keys[i].note;
            queueMidiMsg(noteOff);
            keys[i].midiState = 0;
          }
          keys[i].prevState = 1;
        } else {
          LCD_ShowString(0,  32, (u8 *)("  "), BRED);
          keys[i].prevState = 0;
        }
      } else {
        // No change in state
      }
    }
    if (midiBufIdx > 0) {
      midi_usb_data_send(&USB_OTG_dev, midiBufIdx + 1);
    }
    delay_1ms(5);
  }
}