# libhwio

* Library for unified access for accelerators


## features

* Extraction of informations about devices from device-tree, XML, or remote server

* Local access to memory mapped and RSoC devices

* Remote access to devices

* plugins


For simple examples take look at tests.

## Hwio configs

libhwio does not contains any hardcoded information about hardware as it used to be in HWIO v0.4 for RSoC.
Instead configuration is loaded from xml config files.
These files are ~/.hwio/config.xml /etc/hwio/config.xml
This files currently contains definitions of busses where libhwio should search for devices.
(Example configs are in src/test_samples) Configuration, and configuration file can be also overloaded from CLI.


## Hwio server and it's plugins

Hwio server plugin is .so file which has function void hwio_server_on_load(HwioServer * server) defined
This function is called on start of hwio server. In this function it is possible to register functions for remote calls by HwioServer.install_plugin_fn.
After this remote functions can be called on devices by it's name.

### Dangers of HWIO remote

* arguments to hwio plugin functions can be passed only by value (= do not use pointers, references, etc.)
* use types with constant size because types like int can differ between client and server (int -> int32_t) 
* blocking in plugin function will freeze whole server.
* exceptions are passed, but different exception is raised on client.
