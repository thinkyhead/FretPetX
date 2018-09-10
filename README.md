# FretPetX

FretPet is a guitar-oriented music sequencer for Mac OS X implemented using the Carbon framework.

![FretPet Document Window](/Resources/manual/section/_img/document.png)

Features include:
- A 4-channel sequencer using the built-in AU Synthesizer and/or MIDI output
- 8 interactive palettes to view and edit the current chord
- Transform filters to do interesting things with the selected chords
- Skinnable Guitar palette graphics using bundles
- Export to QuickTime, MIDI, and SunVox
- Keyboard shortcuts for most features, for accessibility

## Source Code

This version of FretPetX (starting with 1.4.0) dates back to late 2012 and can be compiled with versions of XCode dating from that point in time.

Due to its use of several deprecated API calls, this version is not viable for publication on the App Store, and in fact an entirely new Cocoa version of FretPetX would need to be developed to be accepted. Therefore I've decided to open source this Carbon version of FretPet under the MIT License so that you can study and learn from it. I'm sure it's not the greatest example of macOS C++, but it works pretty well and has a lot of cool and useful subroutines, such as those for file export.

In order to compile the code in its current state, you'll need to remove all dependencies on the Sparkle framework and the Kagi and App Store authorizations and use an earlier version of Mac OS X with an installation of XCode 4. If you manage to get this into shape so it can compile in the latest macOS (now 10.13.6) and XCode (now 9.4.1), please let me know!
