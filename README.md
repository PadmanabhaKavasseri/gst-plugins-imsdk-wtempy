**After repository creation:**
- [ ] Update this `README.md`. Update the Project Name, description, and all sections. Remove this checklist.
- [ ] If required, update `LICENSE.txt` and the License section with your project's approved license
- [ ] Search this repo for "REPLACE-ME" and update all instances accordingly
- [ ] Update `CONTRIBUTING.md` as needed
- [ ] Review the workflows in `.github/workflows`, updating as needed. See https://docs.github.com/en/actions for information on what these files do and how they work.
- [ ] Review and update the suggested Issue and PR templates as needed in `.github/ISSUE_TEMPLATE` and `.github/PULL_REQUEST_TEMPLATE`

# Project Name

*\<update with your project name and a short description\>*

Project that does ... implemented in ... runs on Qualcomm® *\<processor\>*

## Branches

**main**: Primary development branch. Contributors should develop submissions based on this branch, and submit pull requests to this branch.

## Requirements

List requirements to run the project, how to install them, instructions to use docker container, etc...

## Installation Instructions

How to install the software itself.

## Usage

Describe how to use the project.

## Development

How to develop new features/fixes for the software. Maybe different than "usage". Also provide details on how to contribute via a [CONTRIBUTING.md file](CONTRIBUTING.md).

## Getting in Contact

How to contact maintainers. E.g. GitHub Issues, GitHub Discussions could be indicated for many cases. However a mail list or list of Maintainer e-mails could be shared for other types of discussions. E.g.

* [Report an Issue on GitHub](../../issues)
* [Open a Discussion on GitHub](../../discussions)
* [E-mail us](mailto:REPLACE-ME@qti.qualcomm.com) for general questions

## License

*\<update with your project name and license\>*

*\<REPLACE-ME\>* is licensed under the [BSD-3-clause License](https://spdx.org/licenses/BSD-3-Clause.html). See [LICENSE.txt](LICENSE.txt) for the full license text.






--PK--

git clone git@github.com:PadmanabhaKavasseri/gst-plugins-imsdk-wtempy.git

apt install cmake

sudo apt-add-repository -y ppa:ubuntu-qcom-iot/qcom-ppa

sudo apt install -y gstreamer1.0-tools gstreamer1.0-tools gstreamer1.0-plugins-good gstreamer1.0-plugins-base gstreamer1.0-plugins-base-apps gstreamer1.0-plugins-qcom-good gstreamer1.0-qcom-sample-apps


git clone -b ubuntu_setup --single-branch https://github.com/rubikpi-ai/rubikpi-script.git 
cd rubikpi-script  
./install_ppa_pkgs.sh 
sudo apt-add-repository -s ppa:ubuntu-qcom-iot/qcom-ppa
sudo apt-get install adreno-dev
sudo apt-get install gstreamer1.0-qcom-sample-apps-utils-dev
sudo apt build-dep gst-plugins-qti-oss
cd /home/ubuntu
sudo apt source gst-plugins-qti-oss
# Add the Qualcomm IoT PPA
sudo apt-add-repository -y ppa:ubuntu-qcom-iot/qcom-ppa

# Install GStreamer / IM SDK
sudo apt update
sudo apt install -y gstreamer1.0-tools gstreamer1.0-tools gstreamer1.0-plugins-good gstreamer1.0-plugins-base gstreamer1.0-plugins-base-apps gstreamer1.0-plugins-qcom-good gstreamer1.0-qcom-sample-apps

# Install Python bindings for GStreamer, and some build dependencies
sudo apt install -y v4l-utils libcairo2-dev pkg-config python3-dev libgirepository1.0-dev gir1.2-gstreamer-1.0


build ±|main ✗|→ cmake ..
-- The C compiler identification is GNU 13.3.0
-- The CXX compiler identification is GNU 13.3.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Found PkgConfig: /usr/bin/pkg-config (found version "1.8.1")
-- Checking for module 'gstreamer-1.0>=1.20.1'
--   Found gstreamer-1.0, version 1.24.2
-- Checking for module 'gstreamer-allocators-1.0>=1.20.1'
--   Found gstreamer-allocators-1.0, version 1.24.2
-- Checking for module 'gstreamer-video-1.0>=1.20.1'
--   Found gstreamer-video-1.0, version 1.24.2
-- Checking for module 'gstreamer-qcom-oss-utils-1.0>=1.0.0'
--   Package 'gstreamer-qcom-oss-utils-1.0', required by 'virtual:world', not found
CMake Error at /usr/share/cmake-3.28/Modules/FindPkgConfig.cmake:619 (message):
  The following required packages were not found:

   - gstreamer-qcom-oss-utils-1.0>=1.0.0

Call Stack (most recent call first):
  /usr/share/cmake-3.28/Modules/FindPkgConfig.cmake:841 (_pkg_check_modules_internal)
  gst-plugin-vcomposer/CMakeLists.txt:18 (pkg_check_modules)


-- Configuring incomplete, errors occurred!
build ±|main ✗|→


build-base ±|main ✗|→ cmake ../gst-plugin-base
CMake Warning at CMakeLists.txt:2 (project):
  VERSION keyword not followed by a value or was followed by a value that
  expanded to nothing.


-- The C compiler identification is GNU 13.3.0
-- The CXX compiler identification is GNU 13.3.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Found PkgConfig: /usr/bin/pkg-config (found version "1.8.1")
-- Checking for module 'gstreamer-1.0>='
--
CMake Error at /usr/share/cmake-3.28/Modules/FindPkgConfig.cmake:619 (message):
  The following required packages were not found:

   - gstreamer-1.0>=

Call Stack (most recent call first):
  /usr/share/cmake-3.28/Modules/FindPkgConfig.cmake:841 (_pkg_check_modules_internal)
  CMakeLists.txt:15 (pkg_check_modules)


-- Configuring incomplete, errors occurred!
build-base ±|main ✗|→

