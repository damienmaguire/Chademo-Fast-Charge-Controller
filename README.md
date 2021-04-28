# Chademo-Fast-Charge-Controller
Development of a control unit to permit Chademo 50kw DC Fast Charging on converted electric vehicles.

Designfiles in Designspark PCB 9.0 format.

01/03/20 : Design files uploaded. Prototypes ordered from JLCPCB. Untested as of this release.
<br>
<br>


16/04/20 : Uploaded pinout.


02/06/20 : Following some testing failures, V2 hardware now uploaded. Problems arose with the HV voltage divider and isolation distances.

15/09/20 : Following failure of the two previous version based on having HV sensing onthe board the decision was made to use an external sensor and transmitt the data via can. The device chosen is the ISA IVT shunt that I now use in all my vehicles for battery monitoring :
https://www.isabellenhuette.de/en/precision-measurement/standard-products/ivt-series/

Isaac Kelly on the Openinverter forum did the big task of porting the old firmware from the Atmega328 to the much more capable SAM3X8E and removing a lot of legacy rubbish :
https://openinverter.org/forum/viewtopic.php?f=17&t=524

The result is now a working system to implement Chademo on EV conversion without the need to sense HV on the actual board. 
Software is fully opensource.
Hardware schematics and gerbers released. Full sources in the near future.



Hardware based on the JLD505 design by :
Collin Kidder, Paulo Alemeida, Jack Rickard.

Software : https://github.com/collin80/JLD505


This software is MIT licensed:

Copyright (c) 2014 Collin Kidder, Paulo Alemeida, Jack Rickard

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


10/02/20 : Initial commit and PCB layout commenced. 

28/04/21 : New V3 hardware and software uploaded. Based on the Openinverter software system by Johannes Huebner using an STM32F103C8T6 MCU.

