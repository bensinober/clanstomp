# USB midi foot pedal project

So this is a shot at making a self-sustained midi foot pedal board for live use.

Point is simple, enumerate a USB Midi device, sending NoteOn / NoteOff / Clip Midi messages
using micro switches and a led to display status / notes sent to USB host / Sampler.


![WIP](https://github.com/bensinober/clanstomp/blob/main/docs/clanstomp.jpg?raw=true "a very basic board")

TODO:

- [ ] Hook up another switch to use as tap tempo / bpm command messages

- [ ] Find a way to send or trigger midi clips in sampler


## Code

Written in C, using the GD32 usb and lcd processor headers and code.

https://github.com/sipeed/platform-gd32v/blob/master/examples/eval-blink/src/main.c

Based ot Lilygo TTGO T-DISPLAY GD32 RISC-V unit

Display is 135 x 240 px

Platform.io project

https://github.com/sipeed/platform-gd32v/blob/master/examples/eval-blink/src/main.c

https://github.com/ivaniacono/GD32-RISCV-Example

# Build and flash

Put device in DTU mode by press and hold BOOT and then RESET button

    pio run --target upload
