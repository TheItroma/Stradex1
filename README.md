STRADEX1 Presentation
==============
I've always wanted to put my violin skills to use in a MIDI environment, but there really isn't a controller on the market that can emulate the feel of a violin to the extent that Stradex does. 

Stradex features a SoftPot linear resistor to emulate the violin's/mandolin fingerboard In a way that feels like a crossover between an otamatone, a mandoline and a violin, four custom force-sensitive keys to emulate the strings, and three potentiometers for range & modulation control. This setup allows Stradex to generate pitch bends, vibrato, dynamic expression, and incredible range, all while remaining intuitive for the average string player. 

This project took me a good while. Sure, there was a lot of time spent fiddling with faulty ADS1115 chips and iterating on key designs, but most of the headscratching was in the embedded firmware, namely, the MIDI signal interface. Many digital signal filtering and processing techniques had to be employed, including buffering, low-pass filtering, digital hysterisis, and numerous serial optimizations to ensure the controller intellegently decided when to send MIDI signals and when to ignore them. 

## Build guide : 

# [**Over here !**](Docs/BuildGuide.md)

If you already have quite a bit of experience or
just don't want the bloat of the beginner guide : [This is the advanced guide](Docs/AdvancedBuildGuide.md)

Check out my website for more projects + contact me.

[My Website](https://bradylin.com/)
