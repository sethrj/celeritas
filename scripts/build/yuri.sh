#!/bin/sh -e
BUILDSCRIPT_DIR="$(cd "$(dirname $BASH_SOURCE[0])" && pwd)"
SOURCE_DIR="$(cd "${BUILDSCRIPT_DIR}" && git rev-parse --show-toplevel)"

printf "\e[2;37mBuilding from ${SOURCE_DIR}\e[0m\n"
cd $SOURCE_DIR
mkdir build 2>/dev/null || true
cd build

module load doxygen swig

CELERITAS_ENV=${SPACK_ROOT}/var/spack/environments/celeritas/.spack-env/view
export PATH=$CELERITAS_ENV/bin:${PATH}
export CMAKE_PREFIX_PATH=$CELERITAS_ENV:${CMAKE_PREFIX_PATH}

cmake -G Ninja \
  -DCMAKE_INSTALL_PREFIX:PATH=${SOURCE_DIR}/install \
  -DPython_EXECUTABLE:PATH=/usr/local/anaconda3/envs/exnihilo/bin/python \
  -DDOXYGEN_EXECUTABLE:PATH=/rnsdhpc/code/spack/opt/spack/apple-clang/doxygen/4xm4zof/bin/doxygen \
  ..
ninja -v
ctest --output-on-failure
