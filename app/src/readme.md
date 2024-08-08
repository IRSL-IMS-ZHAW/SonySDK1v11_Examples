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