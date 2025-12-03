## Description

A color constancy filter that applies color correction based on the grayworld assumption.

For more [info](https://www.researchgate.net/publication/275213614_A_New_Color_Correction_Method_for_Underwater_Imaging).

This is [a port of the FFmpeg filter grayworld](https://github.com/FFmpeg/FFmpeg/blob/master/libavfilter/vf_grayworld.c).

### Requirements:

- AviSynth+ 3.6 or later and/or VapourSynth R55 or later

- Microsoft VisualC++ Redistributable Package 2022 (can be downloaded from [here](https://github.com/abbodi1406/vcredist/releases)) (Windows only)

### AviSynth+ usage:

```
grayworld (clip input, int "opt", int "cc")
```

### VapourSynth usage:

```
grwrld.grayworld(clip input, int "opt", int "cc")
```

### Parameters:

- input<br>
    A clip to process.<br>
    Must be in RGB(A) 32-bit planar format and in linear light.

- opt\
    Sets which cpu optimizations to use.<br>
    -1: Auto-detect.<br>
    0: Use C++ code.<br>
    1: Use SSE2 code.<br>
    2: Use AVX2 code.<br>
    3: Use AVX512 code.<br>
    Default: -1.

- cc\
    Color correction mode.<br>
    0: Mean.<br>
    1: Median. This mode is not affected by extreme values in luminance or chrominance.<br>
    Default: 0.

### Building:

#### Prerequisites
- **Git**
- **CMake** >= 3.25
- A **C++17 capable compiler**

1.  Clone the repository:

    ```
    git clone --depth 1 https://github.com/Asd-g/AviSynthPlus-grayworld
    cd AviSynthPlus-grayworld
    ```

2.  Configure and build the project:

    ```
    CMake options:

    -DBUILD_AVS_LIB=ON  # Build library for AviSynth+.
    -DBUILD_VS_LIB=ON   # Build library for VapourSynth.
    ```

    ```
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j$(nproc)
    ```

3.  (Linux) Install the plugin (optional):

    ```
    sudo cmake --install build
    ```
