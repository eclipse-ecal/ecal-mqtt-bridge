


# MQTT2eCAL
A generic gateway for MQTT to / from eCAL


## Principle
MQTT2eCAL provides a functionality to bridge between eCAL and MQTT in bidirectional. This fully configurable gateway is able to manage the message transmission from eCAL to MQTT and vice versa using a yaml configuration file. 

There are 3 types of messages that can be routed:
*  message that contains the payload that needs to be shifted
*  message that contains the type information of the payload<sup>1</sup>
*  message that contains the descriptor string  

<sup>1</sup> By using `static_ecal_type_name`  parameter, the type information for the eCAL protobuf message is not received via `mqtt_ecal_type_name`, but via this setting.

## Current state of development
Due to the lack of thread support of the mosquitto library on Windows, we decided to keep **only Linux support** until it is fixed.


## Build instructions for Linux
* Install [eCAL](https://continental.github.io/ecal/getting_started/setup.html)
* Add the `mosquitto-dev PPA`  to your sources list :
```
sudo apt-add-repository ppa:mosquitto-dev/mosquitto-ppa 
sudo apt-get update
```
* Install mosquitto, libmosquittopp-dev, libmosquitto-dev using:
`apt-get install mosquitto, libmosquittopp-dev, libmosquitto-dev`


### 1. Clone the repository to a folder on your local machine
### 2. Change current directory to  `build_scripts` and run `make_all.sh` 

## Usage
Simply run the `MqttEcalBridge` application.
Parameters:
* -h, --help   Display help
* -c=PATH, --config=PATH  Use the path to a yaml file to load the configuration, otherwise place the `settings.yaml` file next to the executable
* -v, --verbose  Print all logging information from MQTT

Note: If the `MqttEcalBridge` is provided as a .deb file, make sure you have installed `mosquitto, libmosquittopp-dev, libmosquitto-dev` at least version 2.0 .

