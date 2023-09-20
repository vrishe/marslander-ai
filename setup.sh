#!/bin/bash
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
LIB_DIR=$SCRIPTPATH/lib

function lib_dir {
  mkdir -p "${LIB_DIR}"
  echo "${LIB_DIR}"
}

clear
rm -rf $LIB_DIR &> /dev/null

LIB_NAME=googletest
echo "Begin building ${LIB_NAME}"
pushd 3rd-party/${LIB_NAME}
mkdir build
cd build/
cmake .. -DBUILD_GMOCK=OFF
make
mv lib/*.a `lib_dir`
cd ..
rm -rf build/
popd

LIB_NAME=sockpp
echo "Begin building ${LIB_NAME}"
pushd 3rd-party/${LIB_NAME}
cmake -Bbuild -DSOCKPP_BUILD_STATIC=ON -DSOCKPP_BUILD_SHARED=OFF .
cmake --build build/
mv build/lib${LIB_NAME}.a `lib_dir`
rm -rf build/
popd

LIB_NAME=spdlog
echo "Begin building ${LIB_NAME}"
pushd 3rd-party/${LIB_NAME}
mkdir build
cd build/
cmake ..
make -j
mv lib${LIB_NAME}.a `lib_dir`
cd ..
rm -rf build/
popd
