## Description

A color constancy filter that applies color correction based on the grayworld assumption.

For more [info](https://www.researchgate.net/publication/275213614_A_New_Color_Correction_Method_for_Underwater_Imaging).

This is [a port of the FFmpeg filter grayworld](https://github.com/FFmpeg/FFmpeg/blob/master/libavfilter/vf_grayworld.c).

### Requirements:

- AviSynth+ 3.6 or later

- Microsoft VisualC++ Redistributable Package 2022 (can be downloaded from [here](https://github.com/abbodi1406/vcredist/releases)) (Windows only)

### Usage:

```
grayworld (clip input, int "opt", int "cc")
```

### Parameters:

- input\
    A clip to process.\
    Must be in RGB(A) 32-bit planar format and in linear light.

- opt\
    Sets which cpu optimizations to use.\
    -1: Auto-detect.\
    0: Use C++ code.\
    1: Use SSE2 code.\
    2: Use AVX2 code.\
    3: Use AVX512 code.\
    Default: -1.

- cc\
    Color correction mode.\
    0: Mean.\
    1: Median. This mode is not affected by extreme values in luminance or chrominance.\
    Default: 0.

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
