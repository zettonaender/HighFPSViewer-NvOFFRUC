
## HighFPSViewer-NvOFFRUC
An application that duplicates a second monitor and doubles the FPS with Nvidia Optical Flow (NvOFFRUC) API. Basically DLSS3 frame generation but with support from Turing onwards. This application prioritizes low latency (only one frame behind).

With an RTX 2060, the performance is just enough for 120FPS with a 540p resolution. The results aren't good though, compared to SVP. This is (maybe) why Nvidia limits DLSS3 to the 4000 series cards.

## How to use
You can try it out from the **Release** section (**requires Nvidia 2000 series or better**).

Remarks:
1. Use Alt+Enter for fullscreen.
2. Press F2 while focused to disable mouse cursor drawing.
3. If you get performance issues, change the resolution scaling (can be decimal).

## Compiling
Compiled using Visual Studio 2022 and Nvidia Optical Flow SDK 4.0 . You'll need access to the SDK through Nvidia Developer.
