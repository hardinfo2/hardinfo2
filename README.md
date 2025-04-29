[![Test](https://github.com/hardinfo2/hardinfo2/actions/workflows/test.yml/badge.svg)](https://github.com/hardinfo2/hardinfo2/actions/workflows/test.yml)
[![GitHub release](https://img.shields.io/github/v/release/hardinfo2/hardinfo2?display_name=release)](https://hardinfo2.org/github?latest_release)
[![GitHub release](https://img.shields.io/github/v/release/hardinfo2/hardinfo2?include_prereleases&label=PreRelease&color=blue&display_name=release)](https://hardinfo2.org/github?latest_prerelease)

HARDINFO2
=========

Hardinfo2 is based on hardinfo, which has not been released >10 years. Hardinfo2 is the reboot that was needed.

Hardinfo2 offers System Information and Benchmark for Linux Systems. It is able to
obtain information from both hardware and basic software. It can benchmark your system and compare
to other machines online.

Features include:
- Report generation (in either HTML or plain text)
- Online Benchmarking - compare your machine against other machines

Status
------
- Capabilities: Hardinfo2 currently detects most software and hardware detected by the OS.
- Features: Online database for exchanging benchmark results.
- Development: Currently done by community, hwspeedy maintains

Latest Release News: [https://hardinfo2.org/news](https://hardinfo2.org/news)

Server code can be found here: [https://github.com/hardinfo2/server](https://github.com/hardinfo2/server)

Dependencies
------------
- GTK3 >=3.00 or GTK2+ >=2.20 - (GTK2+ DEPRECATED: cmake -DHARDINFO2_GTK3=0 ..)
- GLib >=2.24
- Zlib
- glib JSON
- Libsoup3 >=3.00 or Libsoup24 >=2.42 (LS24: cmake -DHARDINFO2_LIBSOUP3=0 ..)
- Qt5 >=5.10 (disable QT5/OpenGL Benchmark: cmake -DHARDINFO2_QT5=0 ..)
- Vulkan(headers), libdecor-0, glslang (disable Vulkan Benchmark: cmake -DHARDINFO2_VK=0 ..)

Packaging status
--------------
[![Packaging status](https://hardinfo2.org/repology.svg)](https://hardinfo2.org/repology.svg)

Download and installing (Users)
-------------------------------
Please see the user download and install page here:
[https://hardinfo2.org/download](https://hardinfo2.org/download)

Building and installing (Developers / Testers (Thanx))
-----------------------
**Debian/Ubuntu/Mint/PopOS**
- sudo apt install git cmake build-essential gettext curl
- sudo apt install libjson-glib-dev zlib1g-dev libsoup-3.0-dev libgtk-3-dev libglib2.0-dev libqt5opengl5-dev qtbase5-dev
- sudo apt install libdecor-0-dev glslang-tools (vulkan for newer distros)
- git clone https://github.com/hardinfo2/hardinfo2
- cd hardinfo2
- ./tools/git_latest_release.sh (Switch to latest stable release, tools/git_unstable_master.sh for developers)
- mkdir build
- cd build
- cmake ..
- make package -j (Creates package so you do not pollute your distro and it can be updated by distro releases)
- sudo apt install ./hardinfo2_*  (Use reinstall instead of install if already installed)
- sudo apt install lm-sensors sysbench mesa-utils dmidecode udisks2 xdg-utils iperf3 fwupd x11-xserver-utils vulkan-tools gawk
- hardinfo2

**Fedora/CentOS/RedHat/Rocky/Alma/Oracle**
* NOTE: CentOS 7 needs epel-release and cmake3 instead of cmake - use cmake3 instead of cmake
* NOTE: libdecor.. can be in CRB repo
- sudo yum install epel-release  (only CentOS 7)
- sudo yum install git cmake gcc gcc-c++ gettext rpmdevtools curl
- sudo yum install json-glib-devel zlib-devel libsoup3-devel gtk3-devel qt5-qtbase-devel
- sudo yum install libdecor-devel wayland-devel glslang (vulkan for newer distros)
- git clone https://github.com/hardinfo2/hardinfo2
- cd hardinfo2
- ./tools/git_latest_release.sh (Switch to latest stable release, tools/git_unstable_master.sh for developers)
- mkdir build
- cd build
- cmake ..
- make package -j (Creates package so you do not pollute your distro and it can be updated by distro releases)
- sudo yum install ./hardinfo2-*  (Use reinstall instead of install if already installed)
- sudo yum install lm_sensors sysbench glx-utils dmidecode udisks2 xdg-utils iperf3 fwupd xrandr vulkan-tools gawk
- hardinfo2

**openSUSE**: use zypper instead of yum, zypper --no-gpg-checks install ./hardinfo2-*
libqt5-qtbase-devel instead of qt5-qtbase-devel

**ArchLinux/Garuda/Manjaro/Endeavour/Artix/SystemRescue**
 - git clone https://github.com/hardinfo2/hardinfo2
 - cd hardinfo2
 - ./tools/git_latest_release.sh (Switch to latest stable release, tools/git_unstable_master.sh for developers)
 - cp tools/PKGBUILD .
 - cp tools/hardinfo2.install .
 - makepkg -cis --noextract
 - hardinfo2

**Hardinfo2 package installs the following packages as dependencies**
- Depends:
- **gawk**: Used by hardinfo2 service to determine System Type
- **dmidecode**: is needed to provide DMI information.
- **sysbench**: ver 1.0.20 - is needed to run standard sysbench benchmarks.
- **udisks2**: is needed to provide storage information.
- **mesa-utils**: glxinfo is needed to get OpenGL info.
- **lm-sensors**: is needed to provide sensors values.
- **xdg-utils**: xdg_open is used to open your browser for bugs, homepage & links.
- **iperf3**: iperf3 is used to benchmark internal network speed.
- **vulkan-tools**: vulkaninfo is used to display vulcan information.
- **qt5-base**: QT5 Framework for QT5 OpenGL GPU Benchmark
- **xcb wayland libdecor-0** : WSI Framework for Vulkan Benchmark
- **vulkan glslang-tools** : Vulkan Framework/Shader Tool for Vulkan Benchmark
- **Service**: Service loads SPD modules (at24/ee1004/spd5118) to display SPD info for your DIMMs memory. Show addresses for iomem+ioports.
- Recommends/Depends/Optional: (distro choice - prefer installed)
- **xrandr/x11-xserver-utils**: xrandr is used to read monitor setup
- **fwupd**: fwupd is used to read and display information about firmware in system.

**User can install/setup these depending on hardware**
- **hddtemp**: To obtain the hard disk drive temperature, be sure to run hddtemp
in daemon mode, using the default port.
- **apcaccess**: apcaccess is used for ups/battery information.
- **mesa-vulkan-swrast/libvulkan_lvp**: Vulkan Software driver if you have no hardware vulkan driver (eg. Virtual).

License
------
The Project License has been changed in 2024 from GPL2 to **GPL2 or later**

Because we use LGPL2.1+ and GPL3+ code. To future proof the project, lpereira and other developers have agreed to change license of source code also to GPL2+. [530](https://github.com/hardinfo2/hardinfo2/blob/master/tools/LICENSES/github_com_lpereira_hardinfo_issues_530.pdf)  [707](https://github.com/hardinfo2/hardinfo2/blob/master/tools/LICENSES/github_com_lpereira_hardinfo_issues_707.pdf).

It is all about open source and creating together - Read more about GPL license here: https://www.gnu.org/licenses/gpl-faq.html#AllCompatibility

Privacy Policy
---------------
When using the Synchronize feature in Hardinfo2, some information may be stored indefinitely in our servers.

This information is completely anonymous, and is comprised solely from the machine configuration (e.g. CPU manufacturer and model, number of cores, maximum frequency of cores, GPU manufacturer and model, etc.), version of benchmarking tools used, etc. You can opt out by unticking the "Send benchmark results" entry in the Synchronize window.

Both the Hardinfo2 client and its server components are open source GPL2 or Later and can be audited.
