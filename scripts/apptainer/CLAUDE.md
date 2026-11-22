# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Purpose

This repo packages Claude Code and OpenAI's Codex CLI as [Apptainer](https://apptainer.org/) (formerly Singularity) containers for use on ORNL HPC systems, where Docker is unavailable and network access goes through a proxy (`http://proxy.ftpn.ornl.gov:3128`). Both tools share one base image but get their own container image, persistent home, and launcher scripts.

## Commands

All common tasks are in the `Makefile`. Run `make` with no arguments to list targets.

Every per-tool target comes in three forms: a bare name (e.g. `build`), a `-claude` suffix, and a `-codex` suffix. For non-interactive, order-independent operations (build, the toolchain installs, show-home, clean-home), the bare target runs **both** tools — Make dependency tracking means shared prerequisites like `base.sif` only get built once even when both tool targets depend on it. For interactive operations (`run`, `danger`, `shell`) running both tools at once isn't practical, so the bare target stays single-tool and defaults to Claude Code (`run` == `run-claude`) for backward compatibility; use the `-codex` form explicitly for Codex CLI.

```sh
# Build the base image with system tools (LaTeX, git, ripgrep, uv, etc.)
make build-base

# Build both container images (rebuilds base only if base.def changed)
make build
make build-claude   # Claude Code image only
make build-codex    # Codex CLI image only

# Run interactively, binding a directory to /work inside the container
make run                    # Claude Code (default dir is current directory); same as run-claude
make run-codex DIR=/path    # Codex CLI, bind a specific directory
make danger DIR=/path       # Claude Code with --dangerously-skip-permissions
make danger-codex DIR=/path # Codex CLI with --dangerously-bypass-approvals-and-sandbox

# Open a shell inside a container for debugging
make shell DIR=/path        # Claude Code container
make shell-codex DIR=/path  # Codex CLI container

# Install bundled skills (skills/) into the persistent homes
make install-skills

# Drop the persisted homes (settings, auth tokens, etc.)
make clean-home          # both
make clean-home-claude   # Claude only
make clean-home-codex    # Codex only

# Node-local scratch mounted at /tmp (shared by both tools)
make show-scratch        # print the host dirs in use
make prune-scratch       # delete temp files older than AGENT_TMP_MAX_AGE_DAYS (default 30)
make clean-scratch       # delete /scratch/$USER/agent-tmp
```

The `claude-hpc` script is a minimal wrapper around `apptainer run` without proxy env vars — use it on compute nodes where the proxy may not be needed or available:

```sh
./claude-hpc [dir] [claude args...]
```

`claude-hpc-shell` is the equivalent wrapper for opening a shell in the container (like `make shell`, but without requiring the Makefile):

```sh
./claude-hpc-shell [dir]
```

### Codex CLI

`codex-hpc` / `codex-hpc-shell` are the equivalent launcher scripts, targeting a separate image and persistent home:

```sh
./codex-hpc [dir] [codex args...]
./codex-hpc-shell [dir]
```

`OPENAI_API_KEY`, if set in the environment, is forwarded into the container automatically (both by the Makefile and by `codex-hpc[-shell]`) — needed since `--cleanenv` strips it otherwise. Alternatively, skip the API key and use Codex CLI's own "Sign in with ChatGPT" browser flow on first run.

## Architecture

**`base.def`** — Apptainer definition file for the base image. Bootstraps from `ubuntu:24.04` (not a `node:*` image — see note below), installs Node.js 22 from NodeSource plus system tools (`fd-find`, `ripgrep`, `git`, `texlive-full`, `uv`, etc.) through the ORNL proxy in one `%post` block, then installs the Slurm client (`slurm-smd-client`, PMI libs, `munge`) from the cluster's built packages bind-mounted at `/slurm-install` in a second, separate `%post` block. Rebuild with `make build-base` when system tools need updating.

**`claude-code.def`** — Apptainer definition file for the Claude Code image. Bootstraps from `base.sif` (localimage) and sets the runscript to source `$HOME/.profile` (if present) before `exec claude "$@"`. Claude Code itself is not baked into the image; it is installed into the persistent home via `make install-claude-cli` so it can self-update without rebuilding the container.

**`codex.def`** — Apptainer definition file for the Codex CLI image. Also bootstraps from `base.sif`, but overrides `%environment` to set `HOME=/home/codex` (and matching `PATH`) instead of the base image's baked-in `/home/claude` — needed because that override isn't reliably won by Apptainer's `--home` flag alone. Runscript likewise sources `$HOME/.profile` before `exec codex "$@"`. Codex CLI is likewise installed into the persistent home via `make install-codex-cli`, not baked into the image.

**`base.sif`** / **`claude-code.sif`** / **`codex.sif`** — Compiled container images. Make tracks timestamps so `make build` only rebuilds `base.sif` when `base.def` has changed; `base.sif` itself is shared by both tool images.

**`claude-hpc.sh`** — Thin bash launcher. Resolves `$CLAUDE_IMAGE` (falls back to `$HOME/.containers/images/claude-code.sif`), mounts the target work directory to `/work`, and maps a persistent home at `$HOME/.containers/homes/claude-hpc/home` to `/home/claude` so auth tokens and settings survive across runs. Also bind-mounts node-local scratch at `/tmp` (see Key details).

**`codex-hpc.sh`** / **`codex-hpc-shell.sh`** — Equivalent launchers for Codex CLI. Resolve `$CODEX_IMAGE` (falls back to `$HOME/.containers/images/codex.sif`), map a persistent home at `$HOME/.containers/homes/codex-hpc/home` to `/home/codex`, bind node-local scratch at `/tmp` the same way, and forward `$OPENAI_API_KEY` into the container when set.

**`skills/`** — Agent skills shipped with the repo (currently `slurm-hpc`). `make install-skills[-claude|-codex]` rsyncs them into the persistent homes at `.claude/skills` / `.codex/skills`. Keep them generic, with no user-specific paths, node names or project mappings. The skill has the agent guess each project's cluster-visible path from the mount table (`findmnt`/`/proc/self/mountinfo`), always confirm it with the user, and record confirmed mappings in its own memory, because a reinstall overwrites the skill file.

**`Makefile`** — Task runner that wraps `apptainer build` and `apptainer run` invocations, injecting proxy environment variables and common flags, for both the Claude and Codex image/home pairs.

## Key details

- The container uses `--containall` to isolate the host environment; only the bind-mounted work dir, the synthetic tool home, and the Slurm binds below are visible.
- Both runscripts source `$HOME/.profile` (if it exists) before `exec`ing `claude`/`codex`, so software a user installs into their persistent home (and wires up via `.profile`, e.g. extra `PATH` entries or env vars) is picked up by the tool process itself. Because that sourcing happens once in the runscript rather than per-invocation, and the `claude`/`codex` process's own Bash tool calls are subprocesses of it, exported env vars (like `PATH`) propagate to those Bash tool calls too — unlike the Lmod `module` function below, which needs `BASH_ENV` re-sourcing on every non-interactive invocation because shell functions aren't inherited through the environment the way exported variables are.
- Temp files go to node-local scratch, not NFS: every launcher and `Makefile` flag set bind-mounts `$AGENT_SCRATCH` (default `/scratch/$USER/agent-tmp`, created mode `700`) at `/tmp` inside the container. The directory is shared by both tools. Mounting it at `/tmp` (rather than a new path) covers every tool that writes temp files, not just ones honoring `TMPDIR`, and replaces `--containall`'s small session tmpfs. `claude-code.def` sets `CLAUDE_CODE_TMPDIR=/tmp` and `TMPDIR=/tmp`; `codex.def` sets `TMPDIR=/tmp` (Codex CLI has no `CLAUDE_CODE_TMPDIR` equivalent). If the scratch dir can't be created or written (e.g. a host with no `/scratch`), the launchers fall back to `<persistent home>/tmp` (the Makefile decides once at parse time via `SCRATCH_OK`), so the in-container paths never change. Scratch is local to the machine, so temp files aren't visible from other nodes or Slurm jobs. Because `/tmp` is always bound, the `install-*` targets no longer pass `--bind /tmp` (it would conflict). Rebuild the tool images (`make build`) to pick up the `.def` changes.
- Old temp files are pruned on every launch, since scratch is node-local and no cron/systemd timer can be relied on there: each launcher (and the Makefile's `claude-tmp`/`codex-tmp` order-only prerequisites, via the `prune_tmp` macro) runs a backgrounded, `nice`d `find -xdev -mtime +$AGENT_TMP_MAX_AGE_DAYS -delete` (default 30; `0` disables) over the `/tmp` bind source, so startup isn't delayed. It uses mtime, not atime, because scratch is often mounted `noatime`. Files go first, then empty dirs that are themselves older than the cutoff, so a fresh, still-empty session dir survives (a dir emptied by the prune gets its mtime bumped and goes on a later run). `make prune-scratch` runs the same prune in the foreground. Keep the launcher snippet and the `prune_tmp` macro in sync.
- Claude home state lives at `$HOME/.containers/homes/claude-hpc/home` on the host (mounted at `/home/claude`); Codex home state lives at `$HOME/.containers/homes/codex-hpc/home` (mounted at `/home/codex`) — separate homes, so the two tools' auth/config never collide. Delete them with `make clean-home` / `make clean-home-claude` / `make clean-home-codex`.
- To rebuild after updating any `.def` file, proxy access from the build host is required.
- `apptainer build` runs `%post` in an isolated mount namespace, so host paths aren't visible unless bind-mounted in. The `$(BASE_IMAGE)` rule in `Makefile` passes `--bind /auto/software/slurm/slurm-ubuntu:/slurm-install:ro` to `apptainer build` so `base.def`'s Slurm `%post` block can read the cluster's `.deb`s from `/slurm-install` — without that bind, the glob in `base.def` matches nothing and `apt-get install` fails with `Unsupported file ... given on commandline`.
- Bind-mount destinations must already exist inside the image at build time — unlike at run time, `apptainer build` can't auto-create them without overlay/underlay support. `base.def`'s `%setup` section (`mkdir -p "$APPTAINER_ROOTFS/slurm-install"`) pre-creates `/slurm-install` on the host side before any `%post` section runs, so the `--bind` target exists.
- The cluster's Slurm `.deb`s are built on Ubuntu (`Build-Origin: Ubuntu` in their `.buildinfo`) and require glibc >= 2.38, which doesn't exist in Debian bookworm's archive at all. That's why `base.def` bootstraps from `ubuntu:24.04` instead of a Debian-based `node:*` image — no official `node` image targets Ubuntu, so Node.js 22 is installed from NodeSource in `%post` instead of coming from the base image.
- `base.def` only installs `slurm-smd`/`slurm-smd-client` (client tools), not `slurmd`/`slurmctld` — so nothing creates the local `slurm` system account that `slurmd`'s packaging normally would. `slurm.conf`'s `SlurmUser=slurm` still has to resolve locally or `sinfo`/`squeue`/etc. abort config parsing (`Invalid user for SlurmUser slurm` → `fatal: Unable to process configuration file`), so the Slurm `%post` block creates that account with `adduser --system`.
- Slurm commands work inside the container because `/etc/slurm` (cluster config, read-only) and `/var/run/munge` (the host's running `munged` socket) are bind-mounted at run time in `Makefile`, `claude-hpc.sh`, and `claude-hpc-shell.sh`. The container never holds its own munge key or talks to a `munged` of its own — it authenticates through the host's real one. Network access to `slurmctld`/`slurmdbd` works because `--containall` does not isolate the network namespace.
- `base.def` also installs Lmod and points `MODULEPATH` at the ORNL software tree's Ubuntu 24.04 modulefiles (`/auto/software/swtree/ubuntu24.04/x86_64/modulefiles`), so `module load ...` works from both interactive shells and Claude's own Bash tool calls. Lmod normally only wires the `module` function into login shells via `/etc/profile.d/lmod.sh`, but Claude's Bash tool runs each command as a fresh non-interactive `bash -c` that never sources `/etc/profile.d`; `base.def`'s `%environment` instead sets `BASH_ENV=/etc/lmod-bashenv.sh` (bash's non-interactive-shell init hook) to source Lmod's init script on every invocation.
- The `/auto/software/swtree` bind that makes `MODULEPATH` resolve to anything, along with the `/etc/slurm` and `/var/run/munge` binds above, is present in both tools' flags — `Makefile`'s `CLAUDE_APPTAINER_FLAGS`/`CODEX_APPTAINER_FLAGS`, `claude-hpc.sh`/`codex-hpc.sh`, and `claude-hpc-shell.sh`/`codex-hpc-shell.sh` — so `module load` and Slurm commands work the same way from a Codex CLI container as from Claude Code. `codex.sif` inherits the `lmod`/Slurm-client packages and `MODULEPATH`/`BASH_ENV` env vars from `base.sif` (codex.def only overrides `HOME`/`PATH`).
