STRADEX1 Documentation and Build Instructions
==============
I've always wanted to put my violin skills to use in a MIDI environment, but there really isn't a controller on the market that can emulate the feel of a violin to the extent that Stradex does. 

Stradex features a SoftPot linear resistor to emulate the violin's fingerboard, four custom force-sensitive keys to emulate the strings, and three potentiometers for range & modulation control. This setup allows Stradex to generate pitch bends, vibrato, dynamic expression, and incredible range, all while remaining intuitive for the average string player. 

This project took me a good while. Sure, there was a lot of time spent fiddling with faulty ADS1115 chips and iterating on key designs, but most of the headscratching was in the embedded firmware, namely, the MIDI signal interface. Many digital signal filtering and processing techniques had to be employed, including buffering, low-pass filtering, digital hysterisis, and numerous serial optimizations to ensure the controller intellegently decided when to send MIDI signals and when to ignore them. 

BOM:
1x Raspberry Pi Pico 2
1x 8.5' SoftPot
2x ADS1115 breakout boards
3x 10k Potentiometers
4x 10k FSR
4x Soft tactile pushbuttons
10x Resistors (10k)
8x M3 18mm machine screws
4x M3 40mm machine screws
12x M3 flanged nuts

Instructions to make it:
1. Download the 3MF files inside 3D_files and using your choice of slicer, 3D print the files
2. Download the dxf file inside SDX_plates and lazer cut out of 1/8' acrylic, or extrude them by 1/8' in CAD and 3D print them if you dont have access to a lazer cutter.
3. Download the SDX_gerbers files and order a PCB from a manufacturer, or make your own on a prefboard based on the schematic.
4. Assemble the PCB with through-hole components based on the footprints on the board.
5. Snap the keys onto the hinges, and screw them into the four slots on the board using the 18mm M3 screws. Make sure the FSR is bent over the button and sandwiched between the soft tactule pushbuttons and the underside of the keys.
6. Install the PCB into the body chassis, and place the two acrylic plates on both sides of the body. Then screw the four 40mm M3 screws in to secure the pieces. 
7. Download Pico C SDK and Clone SDX_midi to import as a Pico project. Flash the firmware onto the Pico 2 and it should be detectable as a MIDI device. You can also flash the SDX_sensor_USB_debugger to print all hardware sensor raw values through the serial window to make sure you got the hardware correct.

Check out my website for more projects + contact me.

[My Website](bradylin.com)