# Trim

## About

**Trim** is a lightweight video trimmer designed for speed and simplicity.  

It runs as a CLI tool, accepting a video file as an argument. Once launched, a GUI window appears, allowing users to visually trim the video.  
- **Instant launch**, even for large videos  
- Acts as a GUI extension for a single `ffmpeg` function  
- Users can preview the exact `ffmpeg` command before exporting, making it easy to integrate into larger workflows  

### Features
- **Precision Seeking**: Stop moving the slider while holding to enter precise seeking mode  
- **Zoomable Timeline**: Adjust zoom level for better navigation  

## Dependencies

Trim requires the **LIBAV** libraries from `ffmpeg-devel` and additional dependencies:

- **Windows**: Prebundled DLLs  
- **Mac**: Install via your preferred package manager  
- **Linux**:  
  - CMake file (`lib/FFMPEG/CMakeLists.txt`) needs updating to include **all UNIX systems**, not just macOS  
  - `.so` dependencies have hardcoded paths, so package manager installation is recommended  

### Required Libraries
- **GLFW**  
- **Dear ImGui**  
- **OpenAL**  
- **FFmpeg development libraries**  

## Building from Source

Clone the repository and initialize submodules:

```sh
git submodule update --init --recursive
cmake -S . -B build
cmake --build build

