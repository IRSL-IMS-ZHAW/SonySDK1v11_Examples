# Commits

## in the CMakeList file:

```
### Configure external library directories ###
set(ldir ${CMAKE_CURRENT_SOURCE_DIR}/app)
set(cr_ldir ${ldir}/crsdk)

### Link CRSDK library
find_library(camera_remote Cr_Core HINTS ${cr_ldir})
```

* There is no "CrCore" in "app" folder.
* The ".so" files were found at the "external" folder in the previous version
* Is it necessary to add "libCr_core.so", "libmonitor_protocol_pf.so" and "libmonitor_protocol.so" to the "app".