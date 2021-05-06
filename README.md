# welding_controller

<h3>How to use</h3>
1. Rotate knob either clockwise or counter clockwise to set speed and direction of travel.<br>
2. Press knob to start motor.<br>
3. Adjust speed or direction by rotating the knob.<br>
4. Press knob to stop.

<h3>Optional Safety Switches</h3>
<b>Emergency Switch/E-stop</b><br>
Provides a way to halt the machine in a panic. Connect between D2 and ground of the Arduino Nano. Switch must be <i>NORMALLY OPEN</i>. The Estop IO pin (D2) is active when pulled to GND.<br>
<br>
<b>Limit switches/Endstops</b><br>
Limits the carriage travel within a safe range. Install microswitches at either end of the rails. Wire each switch between D9 and ground, in <i>NORMALLY OPEN</i> position.
Inductive proximity sensors may also be used. The limit switch IO pin (D9) is actived when pulled to GRD.<br>

<h3>Changing the direction of travel</h3>
If the direction of travel is the opposite of what is on the LCD, it can be changed easily.
Flip the SW5 dip switch on the HBS860H servo driver. 

<h3>How to update the firmware</h3>
1. Install USB Driver on a computer running Windows 7 or later. Skip this step if you've installed this before.<br>
<a href="https://sparks.gogo.co.nz/assets/_site_/downloads/CH34x_Install_Windows_v3_4.zip">Driver for Windows</a><br>
2. Download the latest firmware <a href="https://github.com/johntruman/welding_controller/tree/master/firmware">here</a>.<br>
3. Download the flashing tool, <a href="https://github.com/xinabox/xLoader/releases/latest">Xloader</a>.<br>
4. Open Xloader and load the .hex file from step 2.<br>
5. Set XLoader as follows<br>
Device: Duemilanove/Nano(Atmega328)<br>
COM port: the COM port used by the machine. <br>
Baud rate: 115200<br>
6. Press Upload and wait for upload to complete It shouldn't take more than about 30 seconds.<br>

<h3>BOM</h3>
1. 128x64 LCD with ST7920 driver chip, 1pc, <br>
https://shopee.ph/LCD-1602-2004-0802-12864-I2C-Blue-Yellow-Display-Module-Acrylic-Case-IIC-Adapter-Arduino-i.20469516.827082188  <br>
2. Prototyping PCB (7x9), 1pc,  <br>
https://shopee.ph/PCB-Double-Sided-Prototype-Universal-Board-Experimental-Plate-DIY-Fiber-Glass-i.20469516.1481635835  <br>
3. Arduino Nano (Unsolderd), 1pc,  <br>
https://shopee.ph/Arduino-Nano-V3.0-CH340-ATmega328P-5V-16MHz-Nano-I-O-Expansion-Sensor-Shield-Mini-USB-Cable-i.20469516.2136636814  <br>
4. Rotary Encoder, 1pc,  <br>
https://shopee.ph/EC11-Rotary-Encoder-Module-i.20469516.827014474  <br>
5. 5V2A Power Supply, 2pcs,  <br>
https://shopee.ph/AC-DC-5V-2A-Switching-Power-Supply-UL-Listed-i.20469516.827123203  <br>
6. Emergency Switch, 1pc,  <br>
https://shopee.ph/off-switch-Emergency-switch-Emergency-Stop-Switch-Push-Button-i.333764983.6874203064  <br>
7. Limit Switch (ME8108 or ME8104), 2pcs,  <br>
https://shopee.ph/Waterproof-ME-8108-8104-9101-HL-5030-TM1701-LX5-11DMomentary-AC-Limit-Switch-Travel-Switch-i.333764983.3879348490
