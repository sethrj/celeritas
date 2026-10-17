#------------------------------ -*- dockerfile -*- ----------------------------#
# Copyright Celeritas contributors: see top-level COPYRIGHT file for details
# SPDX-License-Identifier: (Apache-2.0 OR MIT)
#-----------------------------------------------------------------------------#
# Reproduce the GitHub ubuntu-24.04 runner toolchain used by the setup-spack
# action, to build and push the Spack buildcache with
# scripts/ci/update-spack-buildcache.sh. Build both stages with the
# pinned Spack revisions using:
#   scripts/docker/buildcache/build.sh
# See scripts/docker/README.md for usage.
#-----------------------------------------------------------------------------#

#-----------------------------------------------------------------------------#
# Runner toolchain with pinned and bootstrapped Spack
#-----------------------------------------------------------------------------#
FROM ubuntu:24.04 AS spack-base-ubuntu24
LABEL description="Ubuntu 24.04 GitHub runner toolchain with pinned Spack"

# System externals must match the versions in scripts/spack/ext-ubuntu24.yaml.
# Like the GitHub runner, provide build-essential: some autotools packages
# probe for the unversioned c++ and /lib/cpp even when they only need C. The
# runner also has GCC 14, whose libstdc++ headers clang-18 selects.
ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8
RUN <<'EOF'
set -ex
apt-get -qq update
apt-get install -y --no-install-recommends \
  autoconf bash build-essential clang-18 flex git libfl-dev m4 make perl \
  bzip2 ca-certificates curl file gzip libstdc++-14-dev patch python3 \
  tar unzip xz-utils zstd
rm -rf /var/lib/apt/lists/*
EOF


# Match the environment of .github/actions/setup-spack. Keep the Spack user
# cache (including the bootstrap store) independent of HOME so that it is
# shared by the celeritas and root users.
ENV SPACK_ROOT=/opt/spack \
    SPACK_PACKAGES_ROOT=/opt/spack-packages \
    SPACK_USER_CACHE_PATH=/opt/spack-cache \
    CELER_SPACK_OPT=/scratch/celeritas/opt-ci

# Build as an unprivileged user like the GitHub runner, since some autotools
# packages refuse to configure as root. The update script installs into
# /scratch/celeritas/opt-ci when it exists: mount a volume there to keep
# installations between runs. Rootless podman (e.g. podman-hpc) must run as
# container root to access bind-mounted files owned by the host user: allow
# root to use the Spack checkouts.
RUN <<'EOF'
set -ex
useradd --create-home --shell /bin/bash celeritas
for d in $SPACK_ROOT $SPACK_PACKAGES_ROOT $SPACK_USER_CACHE_PATH $CELER_SPACK_OPT /work; do
  mkdir -p $d
  chown celeritas:celeritas $d
done
for d in $SPACK_ROOT $SPACK_PACKAGES_ROOT; do
  git config --system --add safe.directory $d
done
EOF
USER celeritas

#-------------------------------------#
# Spack configuration layers
#-------------------------------------#

# Github repositories and spack refs
ARG SPACK_REPO
ARG SPACK_REF

LABEL org.spack.spack-repo="${SPACK_REPO}" \
      org.spack.spack-ref="${SPACK_REF}"
ENV IMAGE_SPACK_REF=${SPACK_REF} \
    SPACK_DISABLE_LOCAL_CONFIG=1 \
    SPACK_COLOR=always \
    PATH=${SPACK_ROOT}/bin:$PATH

# Check out the pinned spack version and bootstrap (which doesn't require packages repo)
RUN <<'EOF'
git init -q "$SPACK_ROOT"
git -C "$SPACK_ROOT" fetch -q --depth 1 \
  "https://github.com/${SPACK_REPO}.git" \
  "${SPACK_REF}"
git -C "$SPACK_ROOT" checkout -q --detach FETCH_HEAD
spack repo set --destination "${SPACK_REF}" builtin
EOF

#-------------------------------------#
# Spack package config layers
#-------------------------------------#

ARG SPACK_PACKAGES_REPO
ARG SPACK_PACKAGES_REF

LABEL org.spack.spack-packages-repo="${SPACK_PACKAGES_REPO}" \
      org.spack.spack-packages-ref="${SPACK_PACKAGES_REF}"
ENV IMAGE_SPACK_PACKAGES_REF=${SPACK_PACKAGES_REF}

RUN <<'EOF'
git init -q "$SPACK_PACKAGES_ROOT"
git -C "$SPACK_PACKAGES_ROOT" fetch -q --depth 1 \
  "https://github.com/${SPACK_PACKAGES_REPO}.git" \
  "${SPACK_PACKAGES_REF}"
git -C "$SPACK_PACKAGES_ROOT" checkout -q --detach FETCH_HEAD
spack bootstrap now
EOF

#-----------------------------------------------------------------------------#
# Buildcache update
#-----------------------------------------------------------------------------#
FROM spack-base-ubuntu24 AS buildcache

LABEL description="Celeritas Spack buildcache builder for GitHub CI"

# Allow container root (see above) to configure autotools packages
ENV FORCE_UNSAFE_CONFIGURE=1 \
    CELER_SOURCE_DIR=/celeritas \
    CELER_ENTRYPOINT_DIR=/opt/celeritas

USER celeritas
WORKDIR /work

COPY entrypoint.sh ${CELER_ENTRYPOINT_DIR}/
ENTRYPOINT ["${CELER_ENTRYPOINT_DIR}/entrypoint.sh"]
CMD ["${CELER_SOURCE_DIR}/scripts/ci/update-spack-buildcache.sh"]
