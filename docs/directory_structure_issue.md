# Directory Structure Issue
It appears that the directory structure was modified before being added to GitHub. Please check if this issue has already been resolved.

### In the `CMakeLists.txt` file:

```cmake
### Configure external library directories ###
set(ldir ${CMAKE_CURRENT_SOURCE_DIR}/app)
set(cr_ldir ${ldir}/crsdk)

### Link CRSDK library
find_library(camera_remote Cr_Core HINTS ${cr_ldir})
```

* The "CrCore" file is missing from the "app" folder.
* In the previous version, the ".so" files were located in the "external" folder.
* Should `libCr_core.so`, `libmonitor_protocol_pf.so`, and `libmonitor_protocol.so` be moved to the "app" folder?
