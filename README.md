# NM180100EVB Simultaneous LoRaWAN and BLE Operation

This application demonstrates simultaneous operation of the LoRaWAN stack and
the BLE stack.  The BLE OTA firmware update profile is used to demonstate the BLE operation.

# Hardware Requirement and Setup

* A NM180100EVB
* An Android Smartphone

# Software Requirement

This example utilizes the NMSDK.  Please follow the SDK build instructions.

To update device firmware over the air, you will need to install the Ambiq's BLE
Android application.  At the time of this writing, the application is located
at

    AmbiqSuite-R2.5.1/tools/apollo3_amota/Application-debug.apk

Additional information regarding how to build an OTA binary and the mobile application
are located under

    AmbiqSuite-R2.5.1/tools/apollo3_amota/scripts
    AmbiqSuite-R2.5.1/docs/app_notes/amota

# Description

The application creates a task for the LoRaWAN application framework and
another task for the BLE OTA framework.  The intend is to demonstrate how to
operate multiple radio stacks simultaneously.

# Build Instructions

There are two build configurations: one for debug and another for release.  The
configuration to be build is defined by the variable DEBUG.  When DEBUG is defined,
the debug configuration is selected and if it is left undefined, then the release
configuration is selected.  The output target will be located in either the debug or
the release directory.

## Debug Configuration
* make DEBUG=1
* make DEBUG=1 clean

## Release Configuration
* make
* make clean
