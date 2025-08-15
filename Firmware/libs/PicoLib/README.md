# Pico Library

This library provides wrappers for various Pico SDK functions, making it easier to work with the Raspberry Pi Pico microcontroller.

This includes:

- GPIO control
- PWM (Pulse Width Modulation)
- ADC (Analog to Digital Conversion)
- I2C communication
- SPI communication
- UART communication
- Various PIO (Programmable Input/Output) functions

It can be combersume to use these functions directly, especially when trying to use DMA (Direct Memory Access) or PIO state machines.

PWM is also a bit convuluted to use, as it requires setting up a timer and configuring the PWM slice.

Ideally we try to ensure these are compile time constants, so that the code is as efficient as possible.
