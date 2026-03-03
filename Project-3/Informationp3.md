I used a Makerbase MKS MINI 12864 for the SD write/read component.
I used and Arduino Mega 2560.
I used a 10 ohm Potentiometer
I used a BNO-055 for the sda, scl gyro data.
I used a Adafruit 2.8 inch shield touchscreen.
I also used a 10K resistor between the MISO pin of the sd card.

The main problem that i had was that the MOSI of the SD card used the same addres as the Adafruit screen so i had to disconect it manually by bending it so it would conflict.
Also some times the sd card wouldn't boot and was not being read. Sometimes my computer wasnt able to read the sd and ill have to format it into a FAT32.
