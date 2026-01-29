# This CMake file is included solely to enable building the entire project as a single unit on Ubuntu.

cmake_minimum_required(VERSION 3.10)
project(gst-plugins-qti-oss LANGUAGES C CXX)

# Set default values for build configuration
if(NOT DEFINED GST_VERSION_REQUIRED)
  set(GST_VERSION_REQUIRED "1.20.1" CACHE STRING "GStreamer minimum required version")
endif()

if(NOT DEFINED GST_PLUGINS_QTI_OSS_VERSION)
  set(GST_PLUGINS_QTI_OSS_VERSION "1.0.0" CACHE STRING "GST Plugins QTI OSS version")
endif()

if(NOT DEFINED SYSROOT_INCDIR)
  set(SYSROOT_INCDIR "/usr/include" CACHE PATH "System root include directory")
endif()

if(NOT DEFINED SYSROOT_LIBDIR)
  set(SYSROOT_LIBDIR "/usr/lib" CACHE PATH "System root library directory")
endif()

if(NOT DEFINED GST_PLUGINS_QTI_OSS_INSTALL_LIBDIR)
  set(GST_PLUGINS_QTI_OSS_INSTALL_LIBDIR "/usr/lib/aarch64-linux-gnu" 
      CACHE PATH "Installation directory for libraries")
endif()

if(NOT DEFINED GST_PLUGINS_QTI_OSS_INSTALL_BINDIR)
  set(GST_PLUGINS_QTI_OSS_INSTALL_BINDIR "/usr/bin" CACHE PATH "Installation directory for binaries")
endif()

if(NOT DEFINED GST_PLUGINS_QTI_OSS_INSTALL_INCDIR)
  set(GST_PLUGINS_QTI_OSS_INSTALL_INCDIR "/usr/include" CACHE PATH "Installation directory for headers")
endif()

if(NOT DEFINED GST_PLUGINS_QTI_OSS_INSTALL_CONFIG)
  set(GST_PLUGINS_QTI_OSS_INSTALL_CONFIG "/etc/configs" CACHE PATH "Installation directory for configuration")
endif()

# Set package metadata variables for configure_file substitution
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

# Few plugins have been disabled for now due to unavailable dependent packages.
# === Core Plugins ===
#add_subdirectory(gst-plugin-qmmfsrc)
add_subdirectory(gst-plugin-vcomposer)
add_subdirectory(gst-plugin-tools)
add_subdirectory(gst-plugin-batch)
add_subdirectory(gst-plugin-metamux)
add_subdirectory(gst-plugin-socket)
add_subdirectory(gst-plugin-vsplit)
add_subdirectory(gst-plugin-vtransform)
add_subdirectory(gst-plugin-voverlay)
add_subdirectory(gst-plugin-restricted-zone)
add_subdirectory(gst-plugin-rtspbin)
add_subdirectory(gst-plugin-redissink)
add_subdirectory(gst-plugin-smartvencbin)
add_subdirectory(gst-plugin-base)
#add_subdirectory(gst-plugin-camreproc)
#add_subdirectory(gst-plugin-jpegenc)
add_subdirectory(gst-plugin-videotemplate)

# === ML Plugins ===
add_subdirectory(gst-plugin-mlaconverter)
add_subdirectory(gst-plugin-mlaclassification)
add_subdirectory(gst-plugin-mldemux)
add_subdirectory(gst-plugin-mlvconverter)
add_subdirectory(gst-plugin-mlvclassification)
add_subdirectory(gst-plugin-mlvsuperresolution)
add_subdirectory(gst-plugin-mlvdetection)
add_subdirectory(gst-plugin-mlvpose)
add_subdirectory(gst-plugin-mlvsegmentation)
add_subdirectory(gst-plugin-mltools)
add_subdirectory(gst-plugin-mltflite)
add_subdirectory(gst-plugin-mlsnpe)
add_subdirectory(gst-plugin-mlqnn)
add_subdirectory(gst-plugin-mlmetaparser)
add_subdirectory(gst-plugin-metatransform)
add_subdirectory(gst-plugin-objtracker)
add_subdirectory(gst-plugin-mlmetaextractor)
add_subdirectory(gst-plugin-mlpostprocess)

# === Sample Apps and Test Framework ===
add_subdirectory(gst-sample-apps/gst-sample-apps-utils)
add_subdirectory(gst-sample-apps)
add_subdirectory(gst-test-framework)
add_subdirectory(gst-python-examples)

# === Messaging Plugins ===
#add_subdirectory(gst-plugin-msgbroker)

#TODO: Disable based on pkgconfig/header file checks
if(DEFINED ENABLE_NON_QLI)
  add_subdirectory(gst-plugin-codec2)
  add_subdirectory(gst-plugin-cv-imgpyramid)
  add_subdirectory(gst-plugin-cv-optclflow)
  add_subdirectory(gst-plugin-dfs)
  add_subdirectory(gst-plugin-drmdecryptor)
  add_subdirectory(gst-plugin-mlaic)
  add_subdirectory(gst-plugin-examples)
endif()
