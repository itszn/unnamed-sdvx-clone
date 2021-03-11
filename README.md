# Unnamed SDVX Clone Event Client (usc-ec) ![language: C/C++](https://img.shields.io/badge/language-C%2FC%2B%2B-green.svg) [![Build](https://github.com/itszn/unnamed-sdvx-clone/workflows/Build/badge.svg)](https://github.com/itszn/unnamed-sdvx-clone/actions)

This is the Event Client for USC. This allows you to see two players in a multiplayer match next to each other.

## Should I use this?

There are two reasons to use the event client:
1. If you are running some kind of event and want to stream the competitors
2. If you are playing in some kind of an event where they are using the Event Client to stream

Other than that this client functions as a normal [USC client](https://github.com/Drewol/unnamed-sdvx-clone)

## Setup

#### Download the latest build from [here (needs a github account)]((https://github.com/itszn/unnamed-sdvx-clone/actions) 

USC-ec runs just as the normal USC client runs, you can just copy `usc-event.exe` into your main usc directory

If you are streaming the client, make sure to copy the `PlaybackDefault` skin as well.

You can also play out of the Event Client directory. Just make sure to copy over your config and update the song path

## Streaming How To

Using USC-ec create a new multiplayer room. This room will only allow other USC-ec users to join. Then open the chat with `f8` and type `/ref`. This will mark you as a ref and not a player. Finally hit `f10` to start the stream client. This will automatically connect to the room and wait for players.

__The client only supports 2 players for now, any others will be ignored__

The stream client will automatically handle results screen navigation, so you should just be able to leave it alone and let it do its thing.

## How to build:
Clone the project and then run `git submodule update --init --recursive` to download the required submodules.

### Windows:
It is not required to build from source. A download link to a pre-built copy of the game is located at the beginning of this README.
The recommended Visual Studio version is 2017, if you want to use a different version then you
will need to edit the 'GenerateWin64ProjectFiles.bat' if you want to follow the guide below.

1. Install [CMake](https://cmake.org/download/)
2. Install [vcpkg](https://github.com/microsoft/vcpkg)
3. Install the packages listed in 'build.windows'
4. Run 'GenerateWin64ProjectFiles.bat' from the root of the project
    * If this fails, try using the `-DCMAKE_TOOLCHAIN_FILE=[VCPKG_ROOT]\scripts\buildsystems\vcpkg.cmake` flag that vcpkg should give you on install
5. Build the generated Visual Studio project 'FX.sln'
6. Run the executable made in the 'bin' folder

To run from Visual Studio, go to Properties for Main > Debugging > Working Directory and set it to '$(OutDir)' or '..\\bin'

### Linux:
1. Install [CMake](https://cmake.org/download/)
2. Check 'build.linux' for libraries to install
3. Run `cmake -DCMAKE_BUILD_TYPE=Release .` and then `make` from the root of the project
4. Run the executable made in the 'bin' folder

### macOS:
1. Install dependencies
	* [Homebrew](https://github.com/Homebrew/brew): `brew install cmake freetype libvorbis sdl2 libpng jpeg libarchive libiconv`
2. Run `mac-cmake.sh` and then `make` from the root of the project.
3. Run the executable made in the 'bin' folder.

### Embedded (Raspberry Pi):
1. Install the libraries listed in 'build.linux'
	* For things that are not in the package manager repository or too low of a version you have to download and build yourself
	* SDL2 Can be installed using the instructions found [here](https://wiki.libsdl.org/Installation#Raspberry_Pi)
2. Run `cmake -DEMBEDDED=ON .`
3. If cmake completes succesfully run `make`
4. Run the executable made in the 'bin' folder
