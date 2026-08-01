## PCB-Badge

I built this electronic badge that can be used for example in hachatons or other IT event if you open the project on KICAD you can also customize the writings.

![Final work](media/finish2.png)

## How this was made

This project consists only of one PCB 85x100mm and other components like the led matrix controller the microcontoller etc. It also has a NFC tag that can be written with the microcontroller and 2 minigames showed on the 8x8 led matrix on the front.

## How to flash the firmware

1. Install Arduino IDE
   
2. Add the board package going into File -> Preferences and in the additional board URL you paste this https://raw.githubusercontent.com/DeqingSun/ch55xduino/ch55xduino/package_ch55xduino_mcs51_index.json
  
3. Navigate to Tools -> Board -> Boards Manager search ch55xduino and click install
   
4. Plug the microcontroller into the pc and select the correct board you are using in my case a CH552
   
5. You can now flash the firmware

## Notes

If you want to bring your own game on the PCB you're welcome! just fork this repository and then if i'll like your work i am going to mention you guys in the original code. Unfortunatley we only have 2 buttons but in the future I can design another PCB with more functions

## AI decleration

Ai helped me to build the part of the firmware that controls the LED matrix
