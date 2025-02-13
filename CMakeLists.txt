cmake_minimum_required(VERSION 3.10)

project(RemoteCli LANGUAGES CXX)

### Enumerate project files ###

### Enumerate RemoteCli header files ###
message("[${PROJECT_NAME}] Indexing header files..")

set(__cli_hdr_dir ${CMAKE_CURRENT_SOURCE_DIR}/app/include)
set(__cli_hdrs
    ${__cli_hdr_dir}/CameraDevice.h
    ${__cli_hdr_dir}/ConnectionInfo.h
    ${__cli_hdr_dir}/PropertyValueTable.h
    ${__cli_hdr_dir}/Text.h
    ${__cli_hdr_dir}/MessageDefine.h
)

## Use cli_srcs in project CMakeLists
set(cli_hdrs ${__cli_hdrs})

### Enumerate RemoteCli source files ###
message("[${PROJECT_NAME}] Indexing source files..")

set(__cli_src_dir ${CMAKE_CURRENT_SOURCE_DIR}/app/src)
set(__cli_srcs
    ${__cli_src_dir}/CameraDevice.cpp
    ${__cli_src_dir}/ConnectionInfo.cpp
    ${__cli_src_dir}/PropertyValueTable.cpp
    ${__cli_src_dir}/RemoteCli.cpp
    ${__cli_src_dir}/Text.cpp
    ${__cli_src_dir}/MessageDefine.cpp
)

## Use cli_srcs in project CMakeLists
set(cli_srcs ${__cli_srcs})

### Enumerate CameraRemote SDK public header files ## referenced by RemoteCli ###
message("[${PROJECT_NAME}] Indexing CameraRemote SDK public header files..")

set(__crsdk_hdr_dir ${CMAKE_CURRENT_SOURCE_DIR}/app/crsdk)
set(__crsdk_hdrs
    ${__crsdk_hdr_dir}/CameraRemote_SDK.h
    ${__crsdk_hdr_dir}/CrCommandData.h
    ${__crsdk_hdr_dir}/CrDefines.h
    ${__crsdk_hdr_dir}/CrDeviceProperty.h
    ${__crsdk_hdr_dir}/CrError.h
    ${__crsdk_hdr_dir}/CrImageDataBlock.h
    ${__crsdk_hdr_dir}/CrTypes.h
    ${__crsdk_hdr_dir}/ICrCameraObjectInfo.h
    ${__crsdk_hdr_dir}/IDeviceCallback.h
)

set(crsdk_hdrs ${__crsdk_hdrs})

source_group("CRSDK" FILES ${crsdk_hdrs})

set(crsdk_hdr_dir ${__crsdk_hdr_dir})


### Define output target ###
set(remotecli "${PROJECT_NAME}")

add_executable(${remotecli}
    ${cli_hdrs}
    ${cli_srcs}
    ${crsdk_hdrs}
)

set(sdk_test "sdk_test")

add_executable(${sdk_test}
    ${crsdk_hdrs}
    ${__cli_hdr_dir}/test_sdk.h
    ${__cli_src_dir}/test_sdk.cpp
    ${__cli_src_dir}/test_sdk2.cpp
)

# add other excutables here if needed ^^^^^

set_target_properties(${remotecli} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)

set_target_properties(${sdk_test} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)

## Specify char is signed-char to fix mismatch with Raspbian
target_compile_options(${remotecli}
    PRIVATE -fsigned-char
)

target_compile_options(${sdk_test}
    PRIVATE -fsigned-char
)

target_include_directories(${remotecli}
    PUBLIC ${__cli_hdr_dir}
    PUBLIC ${__crsdk_hdr_dir} # defined in enum script
)

target_include_directories(${sdk_test}
    PUBLIC ${__cli_hdr_dir}
    PUBLIC ${__crsdk_hdr_dir} # defined in enum script
)

### Configure external library directories ###
set(ldir ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(cr_ldir ${ldir}/crsdk)

### Link CRSDK library
find_library(camera_remote Cr_Core HINTS ${cr_ldir})

target_link_libraries(${remotecli}
    PRIVATE ${camera_remote}
)

target_link_libraries(${sdk_test}
    PRIVATE ${camera_remote}
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8)
        # Must use std::experimental namespace if older than GCC8
        message("[${PROJECT_NAME}] GCC version less than 8. Using std::experimental namespace.")
        target_compile_definitions(${remotecli} PRIVATE USE_EXPERIMENTAL_FS)
        target_compile_definitions(${sdk_test} PRIVATE USE_EXPERIMENTAL_FS)
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        # Must explicitly link separate std::filesystem if older than GCC9
        message("[${PROJECT_NAME}] GCC version less than 9. Explicitly linking separate std::filesystem library.")
        target_link_libraries(${remotecli} PRIVATE stdc++fs)
        target_link_libraries(${sdk_test} PRIVATE stdc++fs)
    endif()
endif()


add_custom_command(TARGET ${remotecli} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${cr_ldir} $<TARGET_FILE_DIR:${remotecli}>
)

## Install application
## '.' means, install to the root directory of CMAKE_INSTALL_PREFIX
install(TARGETS ${remotecli} DESTINATION .)
install(TARGETS ${my_rest} DESTINATION .)
install(DIRECTORY ${cr_ldir}/ DESTINATION .)
