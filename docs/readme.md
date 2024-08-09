# Sony SDK Application Documentation

This application utilizes Sony SDK features to implement several useful functions, including "LiveView", "Sequential Shooting", and "Streaming".

## Overview

The application is designed based on the concept of infinite loop execution with threading. The execution of various functions is controlled by flags, which are defined as `std::atomic<bool>`.

### Flag Manipulation

The manipulation of these flags occurs in two different ways:

1. **By Time**: 
   - A counter in the main function triggers the `toggle_flag` function. As of the latest version (August 8th, 2024), the counter enables the stream after 7 seconds. This delay ensures that initialization is fully completed before streaming begins.

2. **By Keyboard Activation**:
   - **'Q' Key**: Stops the execution properly, cleaning up memory for both camera and SDK controller objects.
   - **'T' Key**: Triggers a sequential shooting routine with a specified number of shots and intervals between them.

### Execution and Control

The routines `SequentialShooting` and `LiveView` are independent of any class since temporal control is managed through the aforementioned flags. This approach ensures better control over the timing of function execution, aligning with the concept of maintaining precise control over the "clock" governing the functions.

## To Do

### 1. Stream Implementation
* Investigate the cause of slow performance in the GStreamer implementation, or determine if it is a limitation of the protocol.
* Identify the best approach to handle the "hDev" (camera object) across different classes responsible for capturing data and streaming
   - Refer to previous versions that include extensive debug output, displaying the memory address of the object, to evaluate whether it was "recreated" during execution.
   - For instance, commented out commands similar to: ```printf("[MAIN] [2] GetDeviceHandle = %ld\n", sonyCamera.debug_streamGetHande())```

### 2. Application Routine
* Explore potential improvements to the execution routines to eliminate the need for flags and independent functions.
* The use of keys is intended to mimic an interruption or another event that might trigger the functions. It may be helpful to visualize when or how these functions would be called.

### 3. Set and Get Attributes
* The function for getting attributes is already working properly, counting and listing the available parameters that can be changed.
   - Check ```void GetCamProperties()``` in the ```Camera``` class.
* However, for setting these attributes, it is still necessary to determine the expected values for each parameter. Additionally, you need to clarify the following:
   - What is the expected data type for each parameter? For example, should the ISO value be set using `int32` or `int16`?
   - What is the valid range of values for each parameter? For instance, should the Focus range be from 0 to 100 or from 0 to 255?

## Directory Structure

The main modifications and key parts of the application are concentrated in the following files:

```plaintext
.
├── app
│   ├── include
│   │   └── sony_app_lib.h
│   ├── src
│   │   ├── sony_app_lib.c
│   │   └── sony_app.c
│   └── CRSDK
│       └── <Public headers>
```
- `sony_app_lib.h`: Header file containing the declarations for the functions and structures used in the application.
- `sony_app_lib.c`: Source file where the core functionality and logic of the application are implemented.
- `sony_app.c`: The main source file that orchestrates the application flow, including the initialization and handling of flags.

## Note
This document is intended to help you navigate the key components of the application and understand the structure and functionality provided in this repository. For any questions or further assistance, please feel free to reach out.
