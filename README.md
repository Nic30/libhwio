# libhwio

libhwio is the library for unified access to hardware resources. This library is suitable for user space tools and libraries which are working with hardware accelerators. Original purpose was to share accelerators in FPGA. This library can also bypass access to hardware using ethernet or TCP. Remote Procedure Calls and simple telemetry is also present.  


## Main features

* intuitive bus-device architecture, simple to use C++14, cmake
* device discovery from device-tree (DTS, /proc/device-tree), json and remote serververs
* local or remote access to hardware (direct mmap, over ethernet/TCP)
* device allocation by compatibility string (address and other properties automatically resolved)
* R/W access, RPC (usefull for server-client mode where server can perform specified functions to minimise communication overhead), IRQ bypass 
* flexible bus architecture which allows to use devices from multiple sources (different bus, different hwio server, simulation ...)

## Typical usecase

### Task

I have an accelerator in FPGA and I need to develop the controll software for it. I can not afford any overhead but I want to automatically detect the address of device from DTS and I would like to run same code on ARM/Microblaze/Nios in FPGA and my x86_64 PC.

### Solution

If you need just FPGA you can just download this library and used it as it is shown in examples.
If you need to bypass hardware access to your PC you have to also download the [libhwio-server](https://github.com/Nic30/libhwio-server) and run it on your FPGA board and configure libhwio config on your PC (add 	{"type": "remote", "host": "IP:PORT"} into bus section).

While developing you can use [libhwio-devmem](https://github.com/Nic30/libhwio-devmem).


## History

* original authors: Korček Pavol, Viktorin Jan, Matoušek Denis
* libhwio v0.5 was complete rewrite of [libhwio 0.4](http://www.fit.vutbr.cz/research/view_product.php?id=274) rewrite was radical, only name has remained. Dropped explicit support for Cesnet, RSoC, SProbe, xtcl. Added support for remote access, json, C++, CMake.



## libhwio configs

libhwio does not contains any hardcoded information about hardware as it used to be in HWIO v0.4 for RSoC.
Instead configuration is loaded from xml config files.
These files are ~/.hwio/config.json /etc/hwio/config.json
This files currently contains definitions of busses where libhwio should search for devices.
(Example configs are in src/test_samples) Configuration, and configuration file can be also overloaded from CLI.


## Simlar opensource projects
* [opae-sdk](https://github.com/OPAE/opae-sdk) - sdk for sharing of FPGA accelerators on Intel Xeon processors (only local, userspace)
* [snap](https://github.com/open-power/snap) - framework for passing actions to hardware accelerators, CAPI, HLS, userspace, IBM


## installation
```
# install dependencies on ubuntu 18.04
sudo apt install build-essential cmake libboost-all-dev

git clone https://github.com/Nic30/libhwio.git && cd libhwio 
mkdir build && cd build
#CMAKE_BUILD_TYPE=Debug allows you to run this library with debuger etc., it is optional
cmake .. -DCMAKE_BUILD_TYPE=Debug 
make
sudo make install
```

## Uninstallation
```
cd build
sudo xargs rm < install_manifest.txt
```


