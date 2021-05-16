#!/bin/sh -e

if [ -z "${SOURCE_DIR}" ]; then
  BUILDSCRIPT_DIR="$(cd "$(dirname $BASH_SOURCE[0])" && pwd)"
  SOURCE_DIR="$(cd "${BUILDSCRIPT_DIR}" && git rev-parse --show-toplevel)"
else
  SOURCE_DIR="$(cd "${SOURCE_DIR}" && pwd)"
fi
if [ -z "${BUILD_DIR}" ]; then
  : ${BUILD_SUBDIR:=build}
  BUILD_DIR=${SOURCE_DIR}/${BUILD_SUBDIR}
fi
: ${CTEST_ARGS:=--output-on-failure}
CTEST_ARGS="-j$(grep -c processor /proc/cpuinfo) ${CTEST_ARGS}"

printf "\e[2;37mBuilding in ${BUILD_DIR}\e[0m\n"
mkdir ${BUILD_DIR} 2>/dev/null \
  || printf "\e[2;37m... from existing cache\e[0m\n"
cd ${BUILD_DIR}

set -x
export CXXFLAGS="-Wall -Wextra -pedantic -Werror"

git -C ${SOURCE_DIR} fetch -f --tags

# Note: cuda_arch must match the spack.yaml file for the docker image, which
# must match the hardware being used.
cmake -G Ninja \
  -DBUILD_DEMOS:BOOL=ON \
  -DBUILD_TESTS:BOOL=ON \
  -DUSE_CUDA:BOOL=ON \
  -DUSE_Geant4:BOOL=ON \
  -DUSE_HepMC3:BOOL=ON \
  -DUSE_MPI:BOOL=ON \
  -DUSE_ROOT:BOOL=ON \
  -DUSE_VecGeom:BOOL=ON \
  -DDEBUG:BOOL=ON \
  -DCMAKE_BUILD_TYPE:STRING="Debug" \
  -DCMAKE_CUDA_ARCHITECTURES:STRING="70" \
  -DCMAKE_EXE_LINKER_FLAGS:STRING="-Wl,--no-as-needed" \
  -DCMAKE_SHARED_LINKER_FLAGS:STRING="-Wl,--no-as-needed" \
  -DMPIEXEC_PREFLAGS:STRING="--allow-run-as-root" \
  -DMPI_CXX_LINK_FLAGS:STRING="-pthread" \
  ${SOURCE_DIR}
ninja -v -k0
ctest $CTEST_ARGS
