An application that duplicates a second monitor and doubles the FPS with Nvidia Optical Flow (NvOFFRUC) API. Basically DLSS3 frame generation but with support from Turing onwards.

You can try it out from the Release section (requires Nvidia 2000 series or better).

The release uses these settings:
1. 0.5x resolution scaling.
2. Second monitor (source) is on the left.

You'll have to compile the program yourself using Visual Studio if you want to change the settings.
You can change them in the "game.h" file under "Important Variables".

1. Set "isOnTheLeft" to false if the source monitor is not on the left. This will disable capturing of the cursor.
2. Change "monitorIndex" to the source monitor's number (you can check it with OBS).
3. Change "resFactor" to the desired scaling for performance reasons (1 is full resolution, 2 is half, etc) .


With an RTX 2060, the performance is just enough for 120FPS with a 540p resolution. The results aren't good though, compared to SVP. This is (maybe) why Nvidia limits DLSS3 to the 4000 series cards. Better results can be obtained by using a third party application such as SVP.