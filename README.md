# trim
## About: Video trimmer with the sole purpose of being as lightweight as possible. 

The program is called as a CLI tool, recieving the desired video to edit as an argument.
Calling the tool will launch a GUI window, allowing you to trim the video.
Launch time is instantaneous even for large videos.
Under the hood, the program simply acts as a GUI extension of one single function of ffmpeg, allowing a visual based trim operation.
In the case that the user wants to use this tool as the 'trimming' part of a larger ffmpeg command, There is the provided option to display exactly what ffmpeg command will be sent before finalizing with the export button.



How it works:
  User can stop moving the slider while holding to enter a more precise seeking mode
  The timeline is zoomable.
  


### Dependencies
The project requires the LIBAV libraries provided by ffmpeg-devel.
* Windows: DLL's are prebundled with the repo


* (Mac)use the package manager of your choice to download the libav libraries provided by ffmpeg.
  
* (Linux): The cmake file is currently under construction, bundling the libav libraries here is more difficult, as the .so's have hardcoded dependency paths. Simple fix, remove the dependency and transfer it to the package manager. I'm lacking a linux machine at the moment. lib/FFMPEG/CMakeLists.txt => Changing this to include ALL unix and not just apple is likely the solution.

The program uses OpenGL, DearImGui,OpenAL, and ffmpeg developement libraries.

Cmake build:
  After cloning the repo, pull git submodules:
  """git submodule update --init --recursive"""

from the root directory of the project:
    cmake -S . -B build
    

Builds on mac, windows, linux.
  
