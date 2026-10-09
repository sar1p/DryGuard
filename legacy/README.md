# Archive of previous programs

`kodingan_iot.cpp` and `NOT-STABLE_Embedded.cpp` were moved from the repository root with contents exactly matching those in the initial project commit `578a8c5`. Both are kept for comparison and are not compiled as part of the modular firmware build.

`NOT-STABLE_Embedded.cpp` has a note in the previous commit about restarting under full load. The archive folder name does not mean that either program has passed hardware testing.

To use an old sketch in the Arduino IDE, copy the selected file to a separate sketch folder and use a `.ino` filename that matches the folder name. Fill in your own device configuration before uploading. Reconcile the physical position after switching firmware, because each version may read a different namespace/checkpoint.
