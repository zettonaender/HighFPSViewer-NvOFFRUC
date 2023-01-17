
## HighFPSViewer-NvOFFRUC
An application that duplicates a second monitor and doubles the FPS with Nvidia Optical Flow (NvOFFRUC) API. Basically DLSS3 frame generation but with support from Turing onwards. This application prioritizes low latency (only one frame behind).

With an RTX 2060, the performance is just enough for 120FPS with a 540p resolution. The results aren't good though, compared to SVP. This is (maybe) why Nvidia limits DLSS3 to the 4000 series cards.

## How to use
You can try it out from the **Release** section (**requires Nvidia 2000 series or better**).

The release uses these settings:
1. **0.5x** (NvOFFRUCTest_Half.exe) or **1x** (NvOFFRUCTest_Full.exe) resolution scaling.
2. Source monitor is indexed **1** (number 2).

Some useful features:
1. Use Alt+Enter for fullscreen.
2. Press F2 to disable mouse cursor drawing (use if the cursor is on the wrong position).

## Compiling
To change the scaling or source number, you'll have to compile the program yourself using Visual Studio and Nvidia Optical Flow SDK 4.0.

You can change them in the "game.h" file under "Important Variables".

1. Change "monitorIndex" to the source monitor's number (you can check it with OBS).
2. Change "resFactor" to the desired scaling for performance reasons (1 is full resolution, 2 is half, etc).
