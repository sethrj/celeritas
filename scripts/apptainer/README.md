# Claude Code / Codex CLI Quick Start

Claude Code and OpenAI's Codex CLI are agentic coding assistants that can read, edit, test, and debug code from a terminal. On ExCL systems, run them inside an isolated environment rather than directly from your normal home directory. This repo packages both the same way, sharing one base image but with separate container images, launchers, and persistent homes.

See [Claude Code | ExCL User Docs](https://docs.excl.ornl.gov/quick-start-guides/claude-code) for the Claude Code documentation.

## Recommended ExCL setup

Use an Apptainer container with:

- A throwaway home directory
- Only the current repository bind-mounted
- No access to your real `~/.ssh`, `~/.aws`, `~/.kube`, `~/.docker`, or `~/.gnupg`
- Claude Code permissions or Auto mode as a second safety layer

Claude’s own sandboxing can restrict Bash commands, but Anthropic notes that the built-in Bash sandbox does not cover every tool, MCP server, or hook. Running the whole Claude Code process inside a container provides a broader boundary.

## Build the Claude Code container

The definition file is [`claude-code.def`](claude-code.def). Build the image on a system where Apptainer builds are allowed:

```bash
make build-claude
```

(Plain `make build` builds both the Claude Code and Codex CLI images — see [Codex CLI](#codex-cli) below.)

To rebuild the base image (system tools, LaTeX, ripgrep, etc.) separately:

```bash
make build-base
```

## Install the launcher

Copy [`claude-hpc.sh`](claude-hpc.sh) to `~/bin` and the image to `~/.containers/images`:

```bash
make install-claude
```

(Plain `make install` installs both launchers.)

## Run Claude Code

From the repository you want Claude to work on:

```bash
cd /path/to/repo
claude-hpc .
```

This gives Claude access to `/work`, which maps to the current repository, and a separate container home at:

```bash
~/.containers/homes/claude-hpc/home
```

## Available make targets

```
make build-claude   Build the Claude Code image (builds base first if needed)
make build-base     Build the base image with system tools only
make run            Run Claude Code (override work dir with DIR=/path)
make remote         Run Claude Code with --remote-control
make danger         Run Claude Code with --dangerously-skip-permissions
make shell          Open a bash shell inside the container
make install-claude Install claude-hpc launcher to ~/bin and image to ~/.containers/images
make install-uv-claude   Install uv into the persistent Claude home
make install-rust-claude Install Rust toolchain into the persistent Claude home
make install-skills-claude Install bundled skills (e.g. slurm-hpc) into the persistent Claude home
make show-home-claude    Print the path to the persisted Claude home
make clean-home-claude   Delete the persisted Claude home (resets auth and settings)
make show-scratch        Print the host dirs mounted at /tmp in the containers
make prune-scratch       Delete temp files older than AGENT_TMP_MAX_AGE_DAYS (default 30) now
make clean-scratch       Delete the node-local scratch dir (temp files only)
```

Bundled agent skills live in `skills/` (currently `slurm-hpc`: Slurm submission, mount-path, GPU and Lmod gotchas when running `sbatch` from inside the container). `make install-skills` copies them into the persistent homes (`~/.claude/skills` and `~/.codex/skills`). Re-running it updates the bundled skills and leaves skills you added yourself alone. Don't store your own notes in a bundled skill, since a reinstall overwrites them.

`build`, `install`, `install-uv`, `install-rust`, `install-skills`, `show-home`, and `clean-home` (no suffix) run the same operation for **both** Claude Code and Codex CLI — see [Codex CLI](#codex-cli) below. `run`, `danger`, and `shell` stay Claude-only when unsuffixed (running two interactive sessions from one `make` invocation isn't practical); use the `-codex` forms for Codex CLI.

Override the working directory with `DIR`:

```bash
make run DIR=/path/to/repo
```

## First-time login

The first time you run Claude Code, authenticate inside the container. Authentication state will be stored in the throwaway Claude home, not in your real home directory.

```bash
claude-hpc .
```

## Safer usage pattern

Start Claude normally:

```bash
claude-hpc .
```

Then ask it to inspect, edit, test, and explain changes.

For more autonomy, prefer Claude Code’s sandbox or Auto mode when available. Anthropic describes sandboxing as combining filesystem isolation and network isolation, and recommends isolation as defense-in-depth when Claude is allowed to act with fewer prompts.

## Dangerous mode

Do not run this directly on the host:

```bash
claude --dangerously-skip-permissions
```

Only consider it inside the Apptainer wrapper:

```bash
claude-hpc . --dangerously-skip-permissions
```

Or via make:

```bash
make danger DIR=/path/to/repo
```

Even then, use it only for disposable branches, clean worktrees, or repositories that do not contain secrets.

## Remote control mode

To allow a remote client such as a Claude Code IDE extension to connect and send commands, start Claude Code with `--remote-control`:

```bash
claude-hpc . --remote-control
```

Or via make:

```bash
make remote DIR=/path/to/repo
```

The container home persists the session state, so the remote client can reconnect across invocations.

## Optional: use a temporary worktree

For risky edits, create a separate worktree:

```bash
git worktree add /tmp/claude-worktree my-claude-branch
cd /tmp/claude-worktree
claude-hpc .
```

Review the diff before merging:

```bash
git diff
git status
```

## Slurm and environment modules

The container includes the cluster's Slurm client tools (`sbatch`, `srun`, `salloc`, `squeue`, `sinfo`, `scancel`, `scontrol`, `sacct`, PMI libs), `munge`, and Lmod. No extra setup is needed when launching through `make run`/`make shell`/`claude-hpc`/`claude-hpc-shell` (or the `-codex` equivalents) — they already bind-mount:

- `/etc/slurm` (read-only) — the cluster's `slurm.conf` and related config
- `/var/run/munge` — the host's running `munged` socket, used for authentication
- `/auto/software/swtree` (read-only) — the ORNL software tree, so Lmod's `MODULEPATH` resolves to real modulefiles and `module load ...` works

The container never holds a copy of the munge key or runs its own `munged`; it authenticates through the host's already-running daemon. `module load` works from Claude's own Bash tool calls, not just interactive shells, because the image sources Lmod's init script via `BASH_ENV` rather than relying on `/etc/profile.d` (which non-interactive shells never source). If you construct your own `apptainer run`/`exec` invocation instead of using the provided scripts, add all three binds:

```bash
apptainer run \
  --cleanenv \
  --containall \
  --home "$HOME/.containers/homes/claude-hpc/home:/home/claude" \
  --bind "$PWD:/work" \
  --bind /auto/software/swtree:/auto/software/swtree:ro \
  --bind /etc/slurm:/etc/slurm:ro \
  --bind /var/run/munge:/var/run/munge \
  --pwd /work \
  "$HOME/.containers/images/claude-code.sif"
```

The same three binds are present in the Codex CLI launchers and `CODEX_APPTAINER_FLAGS` — substitute `codex-hpc/home:/home/codex` and `codex.sif` to build the equivalent invocation for Codex CLI.

## Optional: read-only data bind

If Claude needs reference data but should not modify it, bind it read-only:

```bash
apptainer run \
  --cleanenv \
  --containall \
  --no-home \
  --home "$HOME/.containers/homes/claude-hpc/home:/home/claude" \
  --bind "$PWD:/work" \
  --bind "$SCRATCH/example-data:/data:ro" \
  --pwd /work \
  "$HOME/.containers/images/claude-code.sif"
```

## What not to bind

Do not bind your real home directory or credential directories into the container:

```text
~/.ssh
~/.aws
~/.kube
~/.docker
~/.config/gcloud
~/.gnupg
```

Also avoid binding large shared project roots unless Claude specifically needs them.

## Recommended defaults

Use this for normal ExCL development:

```bash
claude-hpc .
```

Use this only for isolated, disposable work:

```bash
claude-hpc . --dangerously-skip-permissions
```

Use a temporary worktree when you want easy cleanup:

```bash
git worktree add /tmp/claude-worktree claude-test
cd /tmp/claude-worktree
claude-hpc .
```

## Notes

Inside both containers, `/tmp` (and `TMPDIR`, plus `CLAUDE_CODE_TMPDIR` for Claude Code) is node-local scratch: the launchers and `make` targets bind-mount `/scratch/$USER/agent-tmp` there, one directory (mode `700`) shared by Claude Code and Codex CLI. Set `AGENT_SCRATCH=/some/dir` to use a different location. On hosts where that directory can't be created or written, they fall back to `tmp/` inside the tool's persistent home. Because scratch is local to the machine, temp files (including Claude Code's session scratchpad) are not visible from other nodes or from Slurm jobs, so put anything a job needs under the bound work directory. Files left untouched for 30 days are deleted automatically: each launch prunes them in the background (by modification time; empty directories must be equally old). Set `AGENT_TMP_MAX_AGE_DAYS` to change the age, or `0` to disable pruning. `make show-scratch` prints the directories in use, `make prune-scratch` runs the prune immediately, and `make clean-scratch` deletes the whole scratch directory.

Claude Code permissions and sandboxing are complementary. Permissions decide whether Claude may run an action. Sandboxing limits what the action can access after it runs. Anthropic’s documentation states that `--dangerously-skip-permissions` removes per-action review, so it should be used only inside a container, VM, or full-process sandbox.

## Codex CLI

Codex CLI follows the same pattern as Claude Code above, with its own image, launcher, and persistent home so the two tools never share auth/config state.

### Build the Codex CLI container

The definition file is [`codex.def`](codex.def); it reuses `base.sif`, so `make build-base` does not need to be run again if you've already built it for Claude Code.

```bash
make build-codex
```

### Install the launcher

Copy [`codex-hpc.sh`](codex-hpc.sh) and [`codex-hpc-shell.sh`](codex-hpc-shell.sh) to `~/bin` and the image to `~/.containers/images`, installing Codex CLI into the persistent home:

```bash
make install-codex
```

### Run Codex CLI

```bash
cd /path/to/repo
codex-hpc .
```

This gives Codex access to `/work` (the current repository) and a separate container home at:

```bash
~/.containers/homes/codex-hpc/home
```

### Available make targets

```
make build-codex          Build the Codex CLI image (builds base first if needed)
make run-codex             Run Codex CLI (override work dir with DIR=/path)
make danger-codex          Run Codex CLI with --dangerously-bypass-approvals-and-sandbox
make shell-codex           Open a bash shell inside the Codex container
make install-codex         Install codex-hpc launcher to ~/bin and image to ~/.containers/images
make install-codex-cli     Install Codex CLI into the persistent home (user-updatable)
make install-uv-codex      Install uv into the persistent Codex home
make install-rust-codex    Install Rust toolchain into the persistent Codex home
make install-skills-codex  Install bundled skills (e.g. slurm-hpc) into the persistent Codex home
make show-home-codex       Print the path to the persisted Codex home
make clean-home-codex      Delete the persisted Codex home (resets auth and settings)
```

Override the working directory with `DIR`, same as the Claude Code targets:

```bash
make run-codex DIR=/path/to/repo
```

### First-time login

Codex CLI supports two authentication methods; either works from inside the container:

- **ChatGPT sign-in (default):** run `codex-hpc .` with no key set and follow the "Sign in with ChatGPT" prompt on first use. Authentication state is stored under `~/.codex` inside the persistent Codex home, not your real home directory.
- **API key:** export `OPENAI_API_KEY` before launching; both `codex-hpc`/`codex-hpc-shell` and the `make *-codex` targets forward it into the container automatically (it is otherwise stripped by `--cleanenv`).

  ```bash
  export OPENAI_API_KEY=sk-...
  codex-hpc .
  ```

### Dangerous mode

Codex CLI's equivalent of `--dangerously-skip-permissions` is `--dangerously-bypass-approvals-and-sandbox`. As with Claude Code, only use it inside the Apptainer wrapper, never directly on the host:

```bash
codex-hpc . --dangerously-bypass-approvals-and-sandbox
```

Or via make:

```bash
make danger-codex DIR=/path/to/repo
```

The read-only data bind, worktree, and "what not to bind" guidance above apply identically to Codex CLI — just substitute `codex-hpc`/`codex.sif`/`~/.containers/homes/codex-hpc/home` for the Claude equivalents.
