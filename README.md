# Autonomous self-driving RC Car 🚗🕹️

![Overview](docs/Overview.png)

Custom RC Car that only uses a camera to navigate a road course. It recognizes and reacts to dynamically placed traffic signs and junctions fully autonomously.

This is a group project for my Autonomous Vehicles University Course (This project recieved a grade of 100%)

## Live Demo 💻
<a href="https://youtu.be/AEA1OsS4_2U" target=_blank><img src="docs/youtube.jpg" width=500></a>

Click <a href="https://youtu.be/AEA1OsS4_2U" target=_blank>here</a> or on the Video.

## Build and Run it yourself (Simulation) 🔨

![Simulation](docs/simulation.png)

You can also follow this [Link](https://doc.cup.ostfalia.de/) to the Ostfalia Cup Documentation

### Developers Environment Setup Guide

#### Introduction
This guide helps you to setup your own develop environment. Our primary development platforms are Debian based Linux distributions like Ubuntu or Mint. Please make sure, that you have such an OS installed natively on your computer or in a virtual machine.

- Download Ubuntu: https://ubuntu.com/download/desktop
- Download Linux Mint: https://linuxmint.com/download.php

If you use a virtual machine be sure that you have activated all options for accelerating the graphics. We recommend VMware's "Workstation Player" in version 16 or above.

- VMware Workstation Player (recommended): [Download](https://customerconnect.vmware.com/de/downloads/info/slug/desktop_end_user_computing/vmware_workstation_player/17_0)
- Virtual Box (alternative): [Download](https://www.virtualbox.org/wiki/Downloads)

#### Install Software Packages

After a fresh installation you'll need some more packages on your system. First update the apt packet lists and upgrade your systems on the newest updates.

```
sudo apt update && sudo apt upgrade -y
```
Then you can install the additional packages.
```
sudo apt install -y htop git build-essential g++-12 clang-14 cmake \
                    ffmpeg libavcodec-dev libavformat-dev libavutil-dev \
                    libswscale-dev ocl-icd-opencl-dev libgtk2.0-dev \
                    python3-dev python3-numpy lsb-core nano \
                    opencl-headers freeglut3-dev libpocl2
```

#### Create Working Directory
Find a place to put the code repository. For example in a dedicated Git folder:
```
cd ~/
mkdir git
cd git
```

#### Change Standard Compliler
If you have multiple versions of a program, you can use the update-alternatives tool to switch between them. You also have to use it when installing a specific compiler versions as we did here.

First, make the program known to update-alternative by installing them, e.g.:
```
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 12
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-14 14
```

Note that the last number is not a version but the priority, we just happen to use the same number for convenience.

Next we make both compilers known as c++ so we can switch between GCC and Clang whenever we want.
```
sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 1
sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 2
```

Then for switching compilers, use:
```
sudo update-alternatives --config c++
```
This will print a list of alternatives and ask which one to use.

#### Install OpenCV Framework
For normal installation clone the OpenCV repository.
```
cd ~/git
git clone --depth 1 --branch 4.9.0 https://github.com/opencv/opencv.git
cd opencv
mkdir build
cd build
```
##### Build OpenCV
To build the OpenCV framework use the Cmake tool with following flags. After this step build the OpenCV with make. You can use the flag -j with the number of your CPU cores you want to give the building process. In the last step you have to install the build OpenCV framework. The whole process take some time, so now its the best point to drink a coffee. ☕
```
cmake -D CMAKE_BUILD_TYPE=RELEASE -D WITH_GTK=ON -D WITH_FFMPEG=ON ..
```
```
make -j<numberOfCpuCores>
```
```
sudo make install
```

#### Install RC-Car Codebase
##### Clone Repository
```
cd ~/git
git clone https://github.com/YanSchw/Self-Driving-RC-Car.git
```

##### Build Codebase
The build process is very easy and all steps are summarized in just one shell script. Just navigate to the script folder
```
cd ~/git/Self-Driving-RC-Car/scripts
```
and run this script
```
./renew_software.sh
```
Normally the setup should be working now.

#### Simulation
##### Additional Drivers
By installing ```libpocl2```, you should already have an OpenCL driver that can run the simulation. While this works with most CPUs, be they from Intel, AMD, ARM or Apple, there are some CPUs where it doesn't work. In that case, you'll have to look for OpenCL drivers specific for your CPU or GPU.

##### Start Simulation
With the simulation compiled and the drivers installed, you should now be able to run the simulation through the dedicated script:
```
cd ~/git/Self-Driving-RC-Car/scripts
./full_simulation.sh
```
A bunch of windows should appear, and you should see the track, the car, etc.