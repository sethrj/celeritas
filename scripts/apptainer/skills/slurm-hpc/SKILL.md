---
name: slurm-hpc
description: Use whenever submitting or diagnosing Slurm jobs (sbatch/srun/squeue/sacct), especially one-off diagnostic scripts on a specific node, GPU allocation, or jobs that need `module load`. Triggers on "sbatch", "srun", "squeue", "sacct", "slurm", "submit a job", "run this on node X". Encodes mount-path (guess from the mount table, then confirm with the user), container-environment-leak, GPU-visibility, and Lmod gotchas for running Slurm from inside a container.
---

# Slurm job submission and diagnostics

## Mount path mismatch: submission host vs. compute node

Your shell and the Slurm compute nodes can see the same shared storage at
**different paths**. Inside the `claude-hpc`/`codex-hpc` container, for
example, the project is bind-mounted at `/work`, but compute nodes only
know it by its real host path (e.g. `/home/<user>/projects/foo`). The
mapping is **per-project**: it changes whenever the container is launched
on a different directory, so it can't be carried over from a previous
project.

**Always get the user's confirmation of the cluster-visible directory
before the first `sbatch` call in a session that needs `-D`.** Wrong paths
don't fail loudly; they fail with the silent "every job dies in 1s"
signature below, which burns several submit/wait/inspect cycles before the
path turns out to be the problem. Asking costs one turn; a wrong path costs
many. Once the user has confirmed a path in this conversation, reuse it
without asking again.

Don't ask open-ended, though: **work out a best guess first and ask the
user to confirm or correct it.**

1. If memory or CLAUDE.md records a mapping for this exact project, that is
   the guess. Still confirm it, since mappings change.
2. Otherwise derive a guess from the mount table (no job submission or
   probing of the cluster needed):

   ```bash
   findmnt -no SOURCE,FSTYPE,FSROOT --target /work
   # or: grep ' /work ' /proc/self/mountinfo
   ```

   - **NFS** (`SOURCE` is `server:/export/path`): the export path is the
     server-side path, which usually differs from the client mount point
     by a prefix (e.g. `nfs-server:/pool/home/u/proj` is mounted as
     `/home/u/proj` on nodes). Compare with other mounts whose cluster path
     you already know, such as the tool home (`findmnt --target "$HOME"`),
     to work out the prefix swap.
   - **Local disk / bind** (`FSROOT` is a subdirectory such as
     `/u/proj`): `FSROOT` is the path *within* that filesystem; where the
     host mounts the filesystem isn't visible from inside the container.
     The device name (e.g. `lv_scratch`) is only a hint, and node-local
     disks aren't visible to other nodes at all.
3. Ask, showing the evidence: e.g. "`/work` is `nfs-server:/pool/home/u/proj`
   over NFS, so I think compute nodes see it as `/home/u/proj`. Is that
   right?"

Once confirmed, **record the mapping in your memory** (keyed by project,
e.g. "sandbox `/work` → cluster `/home/<user>/projects/foo` for the foo
project") so future sessions start from a good guess. Do not edit this
skill file to store it: the skill is shared and gets overwritten on
reinstall.

The mapping is keyed by **project, not node**: the same node backs
different projects at different paths, so recognizing a node name is not
grounds to reuse another project's mapping.

Rules of thumb:

- **Script argument to `sbatch`**: put it under the shared mount (e.g.
  `/work/foo.sh`), not in a session-local scratch directory (e.g.
  `/tmp/claude-*/.../scratchpad/`). `sbatch` copies the script content at
  submit time, but keeping it beside the job's working directory avoids
  surprises and keeps it inspectable afterwards.
- **Anything the job reads at run time** (input files, helper scripts the
  batch script calls, build outputs) must also live under the shared
  mount. The container's `/tmp`, `$TMPDIR` and the session scratchpad may
  be node-local scratch disk, which compute nodes cannot see at all.
- **`-D <dir>` (working directory)**: must be the *cluster-visible* path
  the compute node can resolve at run time, not the sandbox path
  (`/work`). This also determines where `%j.out`/`%j.err` land.
- Read the resulting output files back via the sandbox path
  (`/work/name.<jobid>.out`) — they're the same underlying files.
- If a cluster-visible path passed as the *script* argument errors with
  "Unable to open file", that's this mismatch — use the sandbox-visible
  path for the script instead.
- **Don't trust `hostname` as a proxy for "same filesystem view."** A
  container can report the same hostname as the node you're targeting
  while having a different mount namespace. A matching hostname is not
  evidence that `/work` is visible to jobs on that node.

### Failure signature when `-D` is wrong (read this before assuming Slurm itself is broken)

If `-D` (or the default `WorkDir`, when `-D` is omitted and your
shell's cwd is the sandbox path) points to a directory the compute node
**can't resolve**, the job fails almost instantly and silently:

- `sacct` shows a `RunTime` of ~1 second and `ExitCode 0:53`
  (`Reason=RaisedSignal:53` in `scontrol show job`).
- **No output file is created at all**, because the job never got far
  enough to open it.
- It happens **on every node** — even a trivial `sbatch --wrap='echo hi'`
  with no `-D` fails the same way if the submission cwd isn't resolvable
  cluster-side. That node-independence makes it *look* like Slurm is
  systemically broken rather than one bad path.

Diagnostic: check
`sacct -j <id> --format=JobID,State,ExitCode,NodeList,WorkDir%60` — a
`WorkDir` of `/work` (the sandbox path) rather than a cluster-visible path
is the tell. Re-submit with the correct `-D` before concluding anything
else is wrong.

## Container environment leaks into jobs

`sbatch` and `srun` pass the submitting shell's whole environment to the job
by default (`--export=ALL`). Submitted from inside the container, a job on
the compute node inherits container-only values:

| Variable | Container value (typical) | Problem on the compute node |
|---|---|---|
| `HOME` | `/home/claude` or `/home/codex` | Doesn't exist there. Tools that read `~` fail or write to the wrong place |
| `PATH` | starts with `$HOME/.local/bin` | Tools installed in the container home aren't found |
| `MODULEPATH` | the container's modulefile tree | May not match the node's Lmod setup (see the Lmod section) |
| `BASH_ENV` | a container-only init script | Missing on the node; bash silently skips it, so `module` stays undefined |
| `TMPDIR` | `/tmp` | Harmless: the node's own `/tmp` |

If the job needs a home directory, set it explicitly at the top of the job
script (`export HOME=<cluster-visible tool home>`). The tool home is
bind-mounted too, so find its cluster path the same way as `-D` above (guess
from `findmnt --target "$HOME"`, then confirm with the user and record it
in memory). Put job-side tools and scratch under that home or the project
directory, never in the user's own home unless they say so.

## Standard invocation pattern

```bash
sbatch -D <cluster-visible-cwd> --nodelist <node> <script-path-visible-at-submit-time>
```

Pin to a specific node with `--nodelist` when diagnosing node-specific
issues (driver/package state, GPU visibility, etc.) rather than letting the
scheduler pick.

## GPU visibility

Landing on a node with GPUs is not enough — Slurm's cgroup hides device
nodes unless the job explicitly requests them:

```
#SBATCH --gres=gpu:<gpu_type>:<n>
```

Without it, GPU-aware tools (e.g. `nvidia-smi`, `rocm-smi`, `clinfo`) may
report the driver present but **0 devices**. Check available GRES types
with:

```bash
sinfo -N -o "%N %G" -n <node>
```

## Lmod (`module load`) inside jobs

`sbatch`/`srun` run the job script as a non-login, non-interactive shell,
so Lmod's init (normally from `/etc/profile.d`) may never run.
`module load ...` — directly or inside a sourced environment-setup script —
then fails with `module: command not found` or silently no-ops.

**Failure signature is often a false pass**: the build/job *succeeds*
because CMake or the build falls back to whatever generic compiler is on
`PATH` (e.g. `/usr/bin/g++`) instead of the module-provided toolchain, so
the code path you meant to test never ran.

Fix: initialize Lmod explicitly at the top of the job script, before any
`module load` or setup script that uses it. Unsetting `MODULEPATH` first
discards the value inherited from the submitting shell (see "Container
environment leaks into jobs"), so the node's own Lmod init sets it:

```bash
unset MODULEPATH_ROOT
unset MODULEPATH
source /etc/profile.d/lmod.sh
```

Always verify the tool that actually got picked up (`which <compiler>`,
`module list`, or `CMAKE_CXX_COMPILER_ID` in the configure log) rather
than trusting a green exit code.

## Diagnostic-script pattern

For one-off investigation (not production jobs), write a throwaway sbatch
script with `set -x` so every command is traced, a short `--time`,
`--ntasks=1`, and a single `--output` using `%j`. Omit `--error` — Slurm
then sends stderr to the same file as `--output`, giving one combined,
interleaved log instead of two files to read:

```bash
#!/bin/bash
#SBATCH --job-name=diag
#SBATCH --output=diag.%j.out
#SBATCH --nodelist=<node>
#SBATCH --time=00:05:00
#SBATCH --ntasks=1

set -x
# diagnostic commands here
```

Clean up throwaway scripts and `.out` files from the project directory
when done.

## Waiting for completion

Don't sleep-poll indefinitely or guess timing. Block until the job leaves
the queue, then check the result:

```bash
while squeue -j $JOBID -h 2>/dev/null | grep -q .; do sleep 2; done
sacct -j $JOBID --format=JobID,State,ExitCode
```

Then read the `.out` file (via the sandbox-visible path) rather than
re-running `sacct` repeatedly.

## Other failure modes worth recognizing

- **`error: TaskProlog failed status=1`** with no other detail: usually a
  node-specific prolog script breaking, not your job or Slurm itself.
  Check `scontrol show config | grep -i prolog` and whether the failing
  node differs from nodes where the job works.
- **PMIx `psec`/`munge` "component not found" warnings** are commonly
  benign noise — check whether the job actually completed before treating
  them as the cause.

## Iterating

Diagnostics are cheap (seconds to tens of seconds per submission) — prefer
submitting a new small script that tests one specific hypothesis over one
large script that dumps everything, once the previous result has narrowed
down what to check next.
