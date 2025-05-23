# Prototype CPP Coro Repository

# Configure and Build

## Requirements

 - CMake 3.28+

## Configure Step

To configure the project with tests:

```
cmake --preset=<preset_name>
```

The `<preset_name>` placeholder should be an available preset defined in
CMakePresets.json:

- windows-x86-debug
- windows-x86-release
- windows-x64-debug
- windows-x64-release
- linux-x86-debug
- linux-x86-release
- linux-x64-debug
- linux-x64-release

Available cmake cache variables are described below:

### Global Variables

 - `WARNINGS_AS_ERRORS`: Treats compiler warnings as errors. `ON` by default. This 
   variable is "global" in that it affects the default value of all subprojets that
   utilize this template.
 - `PROJECT_OUTPUT_PATH`: Specifies the output path of built binaries. The default is 
   the project's `.bin` directory. This variable is "global" in that it affects the 
   default value of all subprojects that utilize this template.
   
### PROTOTYPE_CPP_CORO Variables

 - `PROTOTYPE_CPP_CORO_OUTPUT_PATH`: Specifies the output path of this project's 
   built binaries. Unlike `PROJECT_OUTPUT_PATH`, this variable affects only this
   project. Defaults to `${PROJECT_OUTPUT_PATH}`.
 - `PROTOTYPE_CPP_CORO_WARNINGS_AS_ERRORS`: Treats compiler warnings as errors.
   Set to `${WARNINGS_AS_ERRORS}` by default.


## Build Step

To build the project:

```
cmake --build --preset=<preset_name>
```

Alternatively you may open the project in Visual Studio or Visual Studio Code and 
build from the menu after selecting your desire configuration. Other IDEs may 
work, but they have not been tested.

By default, output binaries will be put into the project's `.bin` directory.
