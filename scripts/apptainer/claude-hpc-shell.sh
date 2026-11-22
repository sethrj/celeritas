#!/usr/bin/env bash
set -euo pipefail

IMAGE="${CLAUDE_IMAGE:-$HOME/.containers/images/claude-code.sif}"
PROXY="${CLAUDE_PROXY:-http://proxy.ftpn.ornl.gov:3128}"
CHOME="$HOME/.containers/homes/claude-hpc/home"

if [[ $# -gt 0 && "$1" != -* ]]; then
  WORK="$(realpath "$1")"
  shift
else
  WORK="$(realpath "$PWD")"
fi

mkdir -p "$CHOME"

# /tmp inside the container is node-local scratch (shared by all agent
# containers), falling back to the persistent home where /scratch is absent.
SCRATCH="${AGENT_SCRATCH:-/scratch/${USER:-$(id -un)}/agent-tmp}"
if mkdir -p "$SCRATCH" 2>/dev/null && [[ -w "$SCRATCH" ]]; then
  chmod 700 "$SCRATCH" 2>/dev/null || true
  TMP_SRC="$SCRATCH"
else
  TMP_SRC="$CHOME/tmp"
  mkdir -p "$TMP_SRC"
fi

# Prune temp files untouched for AGENT_TMP_MAX_AGE_DAYS days (0 disables).
# Runs in the background at low priority so startup isn't delayed; empty
# dirs must be equally old so a fresh, still-empty session dir survives.
AGE="${AGENT_TMP_MAX_AGE_DAYS:-30}"
if [[ "$AGE" =~ ^[0-9]+$ && "$AGE" -gt 0 ]]; then
  (
    nice -n 19 find "$TMP_SRC" -xdev -mindepth 1 ! -type d -mtime +"$AGE" -delete
    nice -n 19 find "$TMP_SRC" -xdev -mindepth 1 -depth -type d -empty -mtime +"$AGE" -delete
  ) >/dev/null 2>&1 &
fi

exec apptainer exec \
  --cleanenv \
  --env https_proxy="$PROXY" \
  --env HTTPS_PROXY="$PROXY" \
  --containall \
  --home "$CHOME:/home/claude" \
  --bind "$WORK:/work" \
  --bind /auto/software/swtree:/auto/software/swtree:ro \
  --bind /etc/slurm:/etc/slurm:ro \
  --bind /var/run/munge:/var/run/munge \
  --bind "$TMP_SRC:/tmp" \
  --pwd /work \
  "$IMAGE" bash -l "$@"
