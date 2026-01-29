cmake_minimum_required(VERSION 3.18)
project(GST_PLUGIN_QTI_OSS_TEMPY
  VERSION ${GST_PLUGINS_QTI_OSS_VERSION}
  LANGUAGES C CXX
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(GST_PLUGINS_QTI_OSS_INSTALL_LIBDIR /usr/lib/aarch64-linux-gnu)

# Common compiler flags.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fPIC -Wno-unused-parameter -Wno-narrowing")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fPIC -Wno-error -fexceptions -Wno-unused-parameter -Wno-narrowing")

if(NOT DEFINED GST_VERSION_REQUIRED)
  set(GST_VERSION_REQUIRED "1.20.1" CACHE STRING "GStreamer minimum required version")
endif()

if(NOT DEFINED GST_PLUGINS_QTI_OSS_VERSION)
  set(GST_PLUGINS_QTI_OSS_VERSION "1.0.0" CACHE STRING "GST Plugins QTI OSS version")
endif()

if(NOT DEFINED GST_PLUGINS_QTI_OSS_LICENSE)
  set(GST_PLUGINS_QTI_OSS_LICENSE "BSD-3-Clause-Clear")
endif()

if(NOT DEFINED GST_PLUGINS_QTI_OSS_SUMMARY)
  set(GST_PLUGINS_QTI_OSS_SUMMARY "Qualcomm open-source GStreamer Plug-ins")
endif()

if(NOT DEFINED GST_PLUGINS_QTI_OSS_ORIGIN)
  set(GST_PLUGINS_QTI_OSS_ORIGIN "http://www.qualcomm.com")
endif()

if(NOT DEFINED GST_PLUGINS_QTI_OSS_PACKAGE)
  set(GST_PLUGINS_QTI_OSS_PACKAGE "gstreamer1.0-plugins-qcom-oss")
endif()

include_directories(${SYSROOT_INCDIR})
link_directories(${SYSROOT_LIBDIR})

find_package(PkgConfig)

# Get the pkgconfigs exported by the automake tools
set(GST_VERSION_REQUIRED "1.19.0")

pkg_check_modules(GST
  REQUIRED gstreamer-1.0>=${GST_VERSION_REQUIRED})
pkg_check_modules(GST_ALLOC
  REQUIRED gstreamer-allocators-1.0>=${GST_VERSION_REQUIRED})
pkg_check_modules(GST_VIDEO
  REQUIRED gstreamer-video-1.0>=${GST_VERSION_REQUIRED})
pkg_check_modules(GST_BASE
  REQUIRED gstreamer-base-1.0>=${GST_VERSION_REQUIRED})

pkg_check_modules(PYTHON3
  REQUIRED python3)

pkg_check_modules(PY3EMBED REQUIRED python3-embed)

# Generate configuration header file.
configure_file(config.h.in config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${Python3_INCLUDE_DIRS}
                    /usr/lib/python3/dist-packages/numpy/core/include)

# Precompiler definitions.
add_definitions(-DHAVE_CONFIG_H)

# Common compiler flags.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fPIC")

# GStreamer source plugin.
set(GST_QTI_TEMPY qtitempy)

add_library(${GST_QTI_TEMPY} SHARED
  src/qtitempy.c
  src/ovld.c 
  src/ppfuncs.c
  src/py_related.c
)

# Ensure position-independent code for the shared library
set_target_properties(${GST_QTI_TEMPY} PROPERTIES POSITION_INDEPENDENT_CODE ON)

message(STATUS "Python3 NumPy include path: ${Python3_NumPy_INCLUDE_DIR}")

target_include_directories(${GST_QTI_TEMPY}
  PUBLIC
    ${GST_INCLUDE_DIRS}
    ${PYTHON3_NUMPY_INCLUDE_DIRS}
    ${PY3EMBED_INCLUDE_DIRS}
    ${PYTHON3_INCLUDE_DIRS}
  PRIVATE
    ${KERNEL_BUILDDIR}/usr/include
    inc
)

target_link_libraries(${GST_QTI_TEMPY}
  PRIVATE
    ${GST_LIBRARIES}
    ${GST_ALLOC_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${GST_BASE_LIBRARIES}
    ${PY3EMBED_LIBRARIES}
    ${PYTHON3_LIBRARIES}
    gstqtimlbase
    gstqtivideobase
    gstqtiutilsbase
    gstqtiallocatorsbase
)

install(
  TARGETS ${GST_QTI_TEMPY}
  LIBRARY DESTINATION ${GST_PLUGINS_QTI_OSS_INSTALL_LIBDIR}/gstreamer-1.0
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
              GROUP_EXECUTE GROUP_READ
              WORLD_EXECUTE WORLD_READ
)

# Specify the file to install
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/opencv_processor.py
    DESTINATION /root
)

