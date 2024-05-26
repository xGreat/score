#!/bin/bash -eux

source ci/common.setup.sh

# Disabling libSDL2-devel until -fPIC problem is sorted...

# removing busybox-which removes git
source ci/common.deps.sh

$SUDO zypper -n remove busybox-which
$SUDO zypper -n install \
   git \
   cmake ninja gcc gcc-c++ \
   llvm-devel \
   libjack-devel alsa-devel \
   portaudio-devel \
   lv2-devel liblilv-0-devel suil-devel \
   libavahi-devel \
   fftw3-devel fftw3-threads-devel \
   bluez-devel \
   qt6-base-devel qt6-base-private-devel  \
   qt6-declarative-devel qt6-declarative-private-devel \
   qt6-tools \
   qt6-scxml-devel qt6-statemachine-devel \
   qt6-shadertools-devel qt6-shadertools-private-devel \
   qt6-websockets-devel qt6-serialport-devel  \
   qt6-qml-devel qt6-qml-private-devel \
   ffmpeg-5-libavcodec-devel ffmpeg-5-libavdevice-devel ffmpeg-5-libavfilter-devel ffmpeg-5-libavformat-devel ffmpeg-5-libswresample-devel

