---
title: "PCB-BuisnessCard"
author: "HexFud"
description: "This will be a pcb style business card with some functions like a little game and maybe a screen"
created_at: "2025-07-31"
---

Started to place down some components in kicad but I am still confused on what microcontroller to chose. I need to find something small that has a lot of gpio pins, my first tought is the CH552G because it also has a usb-c port that can be used for flsahing the firmware on the buisness card 

![schematics](media/scheme.png)

After reading about the microcontroller i finally chose the CH522G, but now i need to change the LEDs because the one i chose are too big and power consuming for what i need to build so i chose something smaller (APT1608)
Update: I decided to build a sort of screen made of LEDs putting on the board an 8x8 led matrix driven by a IS31FL3731-QF led controller. Initially i tought about a single input for all the LED's but while i was wiring them i thought that the couldn' be controlled separately
This is the work in progress matrix:
![led matrix](media/led.png)
