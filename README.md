# Dynamic control system for distributed sensing

## Dependencies
For building the control application and the mqtt client, msgpack-c and libmosquitto have to be installed. Refer to these links:  
https://github.com/msgpack/msgpack-c  
https://mosquitto.org/  
  
Before building NDEE_dynamic_control_system, the steps on this page have to be carried out:  
https://docs.espressif.com/projects/esp-idf/en/latest/get-started/

## Building
All three applications can be built with
```
make
```
in the respective folder.  
For building, flashing and monitoring the NDEE_dynamic_control_system, connect the ESP32 via USB to your PC and run:  
```
make flash monitor
```

## Additional information
The configuration for Wi-Fi connection and socket connection of the ESP32 can be found in:  
NDEE_dynamic_control_system/NDEE/components/control-system/configuration.h  
  
NDEE_dynamic_control_system is based on the NDEE created by Fiona Guerin.