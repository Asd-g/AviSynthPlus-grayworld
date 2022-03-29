## Description

A color constancy filter that applies color correction based on the grayworld assumption.

For more [info](https://www.researchgate.net/publication/275213614_A_New_Color_Correction_Method_for_Underwater_Imaging).

This is [a port of the FFmpeg filter grayworld](https://github.com/FFmpeg/FFmpeg/blob/master/libavfilter/vf_grayworld.c).

### Requirements:

- AviSynth+ 3.6 or later

- Microsoft VisualC++ Redistributable Package 2022 (can be downloaded from [here](https://github.com/abbodi1406/vcredist/releases)) (Windows only)

### Usage:

```
grayworld (clip input)
```

### Parameters:

- input\
    A clip to process.\
    Must be in RGB(A) 32-bit planar format and in linear light.

### Building:

- Windows\
    Use solution files.

- Linux
    ```
    Requirements:
        - Git
        - C++17 compiler
        - CMake >= 3.16
    ```
    ```
    git clone https://github.com/Asd-g/AviSynthPlus-grayworld && \
    cd AviSynthPlus-grayworld && \
    mkdir build && \
    cd build && \

    cmake ..
    make -j$(nproc)
    sudo make install
    ```
