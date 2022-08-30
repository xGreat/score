#!/bin/bash -eu
echo "Running on OSTYPE: '$OSTYPE'"
DISTRO=""
CODENAME=""

command -v git >/dev/null 2>&1 || { echo >&2 "Please install git."; exit 1; }

if [[ -d ../.git ]]; then
  cd ..
elif [[ -d score/.git ]]; then
  cd score
elif [[ ! -d .git ]]; then
  git clone --recursive -j16 https://github.com/ossia/score
  cd score
fi

detect_deps_script() {
    case "$CODENAME" in
      bionic)
        DEPS=bionic
        QT=5
        return 0;;
      buster)
        DEPS=buster
        QT=5
        return 0;;
      bullseye)
        DEPS=bullseye
        QT=5
        return 0;;
      focal)
        DEPS=focal
        QT=5
        return 0;;
      jammy)
        DEPS=jammy
        QT=5
        return 0;;
      leap)
        DEPS=suse-leap
        QT=6
        return 0;;
      tumbleweed)
        DEPS=suse-tumbleweed
        QT=6
        return 0;;
    esac

    case "$DISTRO" in
      arch)
        DEPS=archlinux-qt6
        QT=6
        return 0;;
      debian)
        DEPS=jammy
        QT=5
        return 0;;
      ubuntu)
        DEPS=jammy
        QT=5
        return 0;;
      fedora)
        DEPS=fedora-qt6
        QT=6
        return 0;;
      centos)
        DEPS=fedora
        QT=5
        return 0;;
      suse)
        DEPS=suse-leap
        QT=6
        return 0;;
    esac

    echo "[developer.sh] Could not detect the build script to use."
    return 1
}

detect_linux_qt_version() {
    QT=6
    case "$DISTRO" in
      arch)
        pacman -Syyu
        return 0;;
      debian)
        apt update
        (apt-cache show qt6-base-dev 2>/dev/null | grep 'Version: 6.[23456789]' > /dev/null) || QT=5
        return 0;;
      ubuntu)
        apt update
        (apt-cache show qt6-base-dev 2>/dev/null | grep 'Version: 6.[23456789]' > /dev/null) || QT=5
        return 0;;
      fedora)
        dnf update
        (dnf info qt6-qtbase-devel 2>/dev/null | grep 'Version.*: 6.[23456789]') || QT=5
        return 0;;
      centos)
        dnf update
        (dnf info qt6-qtbase-devel 2>/dev/null | grep 'Version.*: 6.[23456789]') || QT=5
        return 0;;
      suse)
        zypper update
        return 0;;
    esac

    echo "[developer.sh] Could not detect the Qt version to install."
    return 1
}

detect_linux_distro() {
    DISTRO=$(awk -F'=' '/^ID=/ {gsub("\"","",$2); print tolower($2) } ' /etc/*-release 2> /dev/null)
    CODENAME=""
    QT=6
    case "$DISTRO" in
      arch)
        return 0;;
      debian)
        CODENAME=$(cat /etc/*-release  | grep CODENAME | head -n 1 | cut -d= -f2)
        if [[ "$CODENAME" == "" ]]; then
          CODENAME=sid
        fi
        return 0;;
      ubuntu)
        CODENAME=$(cat /etc/*-release | grep CODENAME | head -n 1 | cut -d= -f2)
        return 0;;
      fedora)
        return 0;;
      centos)
        return 0;;
      opensuse-leap)
        DISTRO=suse
        CODENAME=leap
        return 0;;
      opensuse-tumbleweed)
        DISTRO=suse
        CODENAME=tumbleweed
        return 0;;
      *suse*)
        DISTRO=suse
        CODENAME=leap
        return 0;;
    esac

    if command -v pacman >/dev/null 2>&1; then
      DISTRO=arch
      return 0
    fi
 
    if command -v dnf >/dev/null 2>&1; then
      DISTRO=fedora
      return 0
    fi

    if command -v yum >/dev/null 2>&1; then
      DISTRO=centos
      return 0
    fi

    if command -v zypper >/dev/null 2>&1; then
      DISTRO=suse
      CODENAME=leap
      return 0
    fi

    if command -v apt >/dev/null 2>&1; then
      DISTRO=debian
      CODENAME=$(cat /etc/*-release | grep CODENAME | head -n 1 | cut -d= -f2)
      if [[ "$CODENAME" == "" ]]; then
        CODENAME=sid
      fi
      return 0
    fi

    echo "[developer.sh] Could not detect the Linux distribution."
    return 1
}

if [[ "$OSTYPE" == "darwin"* ]]; then
(
  command -v brew >/dev/null 2>&1 || { echo >&2 "Please install Homebrew."; exit 1; }
  (brew list | grep qt@5 >/dev/null) && { echo >&2 "Please remove qt@5 as it is incompatible with the required Homebrew Qt 6 package"; exit 1; }
  
  echo "[developer.sh] Installing dependencies"
  brew update
  brew upgrade
  brew install ninja qt boost cmake ffmpeg fftw portaudio sdl lv2 lilv freetype
  brew cleanup
  
  echo "[developer.sh] Configuring"
  mkdir -p build-developer
  cd build-developer
  
  if [[ ! -f ./score ]]; then
    cmake -Wno-dev \
        .. \
        -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DSCORE_PCH=1 \
        -DSCORE_DYNAMIC_PLUGINS=1 \
        -DQT_VERSION="Qt6;6.2"
  fi
  
  echo "[developer.sh] Building in $PWD/build-developer"
  cmake --build .

  if [[ -f ossia-score ]]; then
    echo "[developer.sh] Build successful, you can run $PWD/score"
  fi
)
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
(
  echo "[developer.sh] Installing dependencies"
  detect_linux_distro
  detect_linux_qt_version
  detect_deps_script
  source "ci/$DEPS.deps.sh"
  
  echo "[developer.sh] Configuring"
  mkdir -p build-developer
  cd build-developer

  if [[ "$QT" == 5 ]]; then
    QT_CMAKE_FLAG=''
  else
    QT_CMAKE_FLAG='-DQT_VERSION=Qt6;6.2'
  fi

  echo "ls: " 
  ls

  if [[ ! -f ./ossia-score ]]; then
    cmake -Wno-dev \
        .. \
        -GNinja \
        $QT_CMAKE_FLAG \
        -DCMAKE_BUILD_TYPE=Debug \
        -DSCORE_PCH=1 \
        -DSCORE_DYNAMIC_PLUGINS=1
  fi
  
  echo "[developer.sh] Building in $PWD"
  cmake --build .

  if [[ -f ossia-score ]]; then
    echo "[developer.sh] Build successful, you can run $PWD/ossia-score"
  fi
)

else
(
    tools/fetch-sdk.sh
)
fi