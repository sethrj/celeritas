#!/bin/sh -ex
#-------------------------------- -*- sh -*- ---------------------------------#
# Copyright Celeritas contributors: see top-level COPYRIGHT file for details
# SPDX-License-Identifier: (Apache-2.0 OR MIT)
#-----------------------------------------------------------------------------#
# Run on excl (or any ubuntu24 system) to build and upload
#-----------------------------------------------------------------------------#

if [ -z "${GITHUB_TOKEN}" ]; then
  echo "error: GITHUB_USER and GITHUB_TOKEN must be set (see scripts/spack/reqs-ci.yaml)"
  exit 1
fi

CELER_BASE_IMAGE=ubuntu:24.04
CELER_BUILDCACHE=sethrj


export CELER_SPACK_VIEW=false

SCRIPT_DIR=$(cd "$(dirname $0)" && pwd)
export CELER_SOURCE_DIR=$(cd $SCRIPT_DIR/../.. && pwd)

# Each line is: CXXSTD, followed by the spack packages to add, based on the
# matrix (and its "include" entries) from .github/workflows/build-spack.yml
matrix="
CXXSTD=20 vecgeom@2.1.0 geant4@11.4 g4vg root covfie
CXXSTD=20 vecgeom@2.0.0-rc.7 geant4@11.3 g4vg root covfie
CXXSTD=20 vecgeom@1.2.11 geant4@11.4 g4vg root covfie py-gcovr
CXXSTD=20 vecgeom@1.2.11 geant4@11.0 g4vg root covfie
CXXSTD=20 vecgeom@1.2.11 geant4@11.1 g4vg root covfie
CXXSTD=20 vecgeom@1.2.11 geant4@11.2 g4vg root covfie
CXXSTD=20 vecgeom@1.2.11 geant4@11.3 g4vg root covfie
CXXSTD=17 vecgeom@1.2.11 geant4@10.5 g4vg
CXXSTD=17 vecgeom@1.2.11 geant4@10.6 g4vg
CXXSTD=20 vecgeom@1.2.11 geant4@10.7 g4vg root covfie
"

echo "$matrix" | while read -r line; do
  [ -z "$line" ] && continue
  # Pop and export CXXSTD
  set -- $line
  export "$1"
  shift

  # Create temporary directory
  envdir="spack-${CXXSTD}-$(echo "$*" | tr ' @' '--')"
  mkdir -p "$envdir"
  (
    cd "$envdir"
    # Delete stale spack yaml if any
    rm -f spack.yaml 2>/dev/null
    # Create environment
    "${SCRIPT_DIR}/setup-spack-ci-env.sh" "$@"
    spack -e . -v concretize --non-defaults --fresh
    spack -e . install
    spack -e . buildcache push \
          --base-image $CELER_BASE_IMAGE --update-index \
          $CELER_BUILDCACHE
  )
done
