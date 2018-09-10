# libhwio

* Library for unified access for accelerators


## features
* Extraction of informations about devices from device-tree, json, or remote server
* Local access to memory mapped and RSoC devices
* Remote access to devices
* plugins


For simple examples take look at tests.

## Hwio configs

libhwio does not contains any hardcoded information about hardware as it used to be in HWIO v0.4 for RSoC.
Instead configuration is loaded from xml config files.
These files are ~/.hwio/config.json /etc/hwio/config.json
This files currently contains definitions of busses where libhwio should search for devices.
(Example configs are in src/test_samples) Configuration, and configuration file can be also overloaded from CLI.


## Simlar opensource projects
* https://github.com/OPAE/opae-sdk - sdk for sharing of FPGA accelerators on Intel Xeon processors (only local, userspace)
* https://github.com/open-power/snap - framework for passing actions to hardware accelerators, CAPI, HLS, userspace, IBM


## installation
```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
sudo make install
```

## Uninstallation
```
cd build
sudo xargs rm < install_manifest.txt
```
