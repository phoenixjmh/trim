##Video trimmer with the sole purpose of being as lightweight as possible. 


###Dependencies
The project requires the LIBAV libraries provided by ffmpeg-devel.
* Windows: DLL's are prebundled with the repo


*Mac and Linux: 
use the package manager of your choice to download the libav libraries provided by ffmpeg.
  


Cmake build:
  After cloning the repo, pull git submodules:
  """git submodule update --init --recursive"""

from the root directory of the project:
    cmake -S . -B build
    

Builds on mac, windows, linux.
  
