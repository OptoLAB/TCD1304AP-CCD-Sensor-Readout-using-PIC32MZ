# TCD1304AP CCD Sensor Readout using PIC32MZ
 Demo project for TCD1304AP linear CCD array sensor using mini-32 for PIC32MZ starter board
## About project

Project present simple readout demo for TCD TCD1304AP CCD sensor with 3648 light sensitive pixels. Firmware is built around timer modules using MPLAB Harmony v3. Communication with PC is achieved using USB in CDC mode (256000 baud rate). "GET" command initiates data tranfer. "SET" command can bi used for adjusting integration time (10us-655.35ms), veritcal resolution (6, 8, 10 or 12 bits) and horizontal resolution (number of measurement points/pixels). 

Folder img contains some oscilloscope screenshots of important signals obtained during firmware development.

Demo project is built using following ecosystem:

 - [MINI-32 For PIC32MZ](https://www.mikroe.com/mini-32-for-pic32mz) starter boards by Mikroelektronika.
 - [MPLAB X IDE v6.05](https://www.microchip.com/en-us/tools-resources/develop/mplab-x-ide) - Integrated Development Environment by Microchip.
 - [XC32](https://www.microchip.com/en-us/tools-resources/develop/mplab-xc-compilers) v3.01 free compiler version for 32-bit MCUs by Microchip
 - [MPLAB Harmony v3](https://www.microchip.com/en-us/tools-resources/configure/mplab-harmony) - Embedded Software Development Framework for 32-bit by Microchip.
 - [PICKIT 3](https://www.microchip.com/en-us/development-tool/PG164130) In-circuit debugger

More info can be found [here](https://www.optolab.ftn.uns.ac.rs/index.php/education/project-base/287-tcd1304ap-ccd-sensor)

Demo video on [youtube](https://youtu.be/KC7FMGzbEMY) 
