cmake_minimum_required(VERSION 3.10)
project(gst-qtighost)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find GStreamer packages
pkg_check_modules(GST REQUIRED 
    gstreamer-1.0>=1.0
    gstreamer-base-1.0>=1.0
    gstreamer-video-1.0>=1.0
)

# Find Python and NumPy
find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)

# Get GStreamer version separately for display
pkg_get_variable(GST_VERSION gstreamer-1.0 version)

# Include directories
include_directories(${GST_INCLUDE_DIRS})
include_directories(${Python3_INCLUDE_DIRS})
include_directories(${Python3_NumPy_INCLUDE_DIRS})

# Add compiler flags
add_definitions(${GST_CFLAGS})

# Define package variables as compiler definitions
add_definitions(
    "-DPACKAGE=\"gst-qtighost\""
    "-DPACKAGE_VERSION=\"1.0.0\""
    "-DPACKAGE_NAME=\"GStreamer QTI Ghost Plugin\""
    "-DGST_LICENSE=\"LGPL\""
    "-DGST_PACKAGE_NAME=\"GStreamer QTI Ghost Plugin\""
    "-DGST_PACKAGE_ORIGIN=\"https://github.com/your-repo/gst-qtighost\""
)

# Create the plugin library
add_library(gstqtighost SHARED
    qtighost.cpp
)

# Link libraries
target_link_libraries(gstqtighost 
    ${GST_LIBRARIES}
    ${Python3_LIBRARIES}
)

# Set library properties
set_target_properties(gstqtighost PROPERTIES
    PREFIX ""  # Remove 'lib' prefix
    SUFFIX ".so"
)

# Installation
set(PLUGIN_INSTALL_DIR /usr/lib/aarch64-linux-gnu/gstreamer-1.0)
install(TARGETS gstqtighost LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} )

# Print build information
message(STATUS "Building GStreamer QTI Ghost plugin with Python support")
message(STATUS "GStreamer version: ${GST_VERSION}")
message(STATUS "Python version: ${Python3_VERSION}")
message(STATUS "Python include: ${Python3_INCLUDE_DIRS}")
message(STATUS "NumPy include: ${Python3_NumPy_INCLUDE_DIRS}")
message(STATUS "Install destination: ${PLUGIN_INSTALL_DIR}")