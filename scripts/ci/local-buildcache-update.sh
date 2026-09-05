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

export CELER_SPACK_VIEW=false
