PROXY      := http://proxy.ftpn.ornl.gov:3128
BASE_IMAGE := base.sif
DIR        ?= .

CLAUDE_IMAGE    := claude-code.sif
CLAUDE_HOME_DIR := $(HOME)/.containers/homes/claude-hpc/home
CLAUDE_PROFILE  := $(CLAUDE_HOME_DIR)/.profile

CODEX_IMAGE    := codex.sif
CODEX_HOME_DIR := $(HOME)/.containers/homes/codex-hpc/home
CODEX_PROFILE  := $(CODEX_HOME_DIR)/.profile

SKILLS_DIR        := skills
CLAUDE_SKILLS_DIR := $(CLAUDE_HOME_DIR)/.claude/skills
CODEX_SKILLS_DIR  := $(CODEX_HOME_DIR)/.codex/skills

# Node-local scratch, bind-mounted at /tmp in both containers (shared by
# Claude Code and Codex CLI). Falls back to each tool's persistent home
# (<home>/tmp) on hosts where the scratch dir can't be created or written.
AGENT_SCRATCH ?= /scratch/$(USER)/agent-tmp
SCRATCH_OK    := $(shell mkdir -p "$(AGENT_SCRATCH)" 2>/dev/null && [ -w "$(AGENT_SCRATCH)" ] && echo yes)
CLAUDE_TMP_SRC := $(if $(SCRATCH_OK),$(AGENT_SCRATCH),$(CLAUDE_HOME_DIR)/tmp)
CODEX_TMP_SRC  := $(if $(SCRATCH_OK),$(AGENT_SCRATCH),$(CODEX_HOME_DIR)/tmp)

# Temp files untouched for this many days are pruned (0 disables). Empty
# dirs must be equally old, so a fresh, still-empty session dir survives.
AGENT_TMP_MAX_AGE_DAYS ?= 30
prune_tmp = $(if $(filter-out 0,$(AGENT_TMP_MAX_AGE_DAYS)),\
	nice -n 19 find "$(1)" -xdev -mindepth 1 ! -type d -mtime +$(AGENT_TMP_MAX_AGE_DAYS) -delete; \
	nice -n 19 find "$(1)" -xdev -mindepth 1 -depth -type d -empty -mtime +$(AGENT_TMP_MAX_AGE_DAYS) -delete,true)

# Use = (not :=) so $(realpath $(DIR)) re-evaluates when DIR is overridden
CLAUDE_APPTAINER_FLAGS = \
	--cleanenv \
	--env https_proxy=$(PROXY) \
	--env HTTPS_PROXY=$(PROXY) \
	--containall \
	--home "$(CLAUDE_HOME_DIR):/home/claude" \
	--bind "$(realpath $(DIR)):/work" \
	--bind /auto/software/swtree:/auto/software/swtree:ro \
	--bind /etc/slurm:/etc/slurm:ro \
	--bind /var/run/munge:/var/run/munge \
	--bind "$(CLAUDE_TMP_SRC):/tmp" \
	--pwd /work

CODEX_APPTAINER_FLAGS = \
	--cleanenv \
	--env https_proxy=$(PROXY) \
	--env HTTPS_PROXY=$(PROXY) \
	--containall \
	--home "$(CODEX_HOME_DIR):/home/codex" \
	--bind "$(realpath $(DIR)):/work" \
	--bind /auto/software/swtree:/auto/software/swtree:ro \
	--bind /etc/slurm:/etc/slurm:ro \
	--bind /var/run/munge:/var/run/munge \
	--bind "$(CODEX_TMP_SRC):/tmp" \
	--pwd /work

ifdef OPENAI_API_KEY
CODEX_APPTAINER_FLAGS += --env OPENAI_API_KEY=$(OPENAI_API_KEY)
endif

.PHONY: build build-claude build-codex build-base \
	run run-claude run-codex remote \
	danger danger-claude danger-codex \
	shell shell-claude shell-codex \
	install-uv install-uv-claude install-uv-codex \
	install-rust install-rust-claude install-rust-codex \
	install-claude-cli install-codex-cli \
	install-skills install-skills-claude install-skills-codex \
	install install-claude install-codex \
	show-home show-home-claude show-home-codex \
	clean-home clean-home-claude clean-home-codex \
	claude-tmp codex-tmp show-scratch prune-scratch clean-scratch \
	help
.DEFAULT_GOAL := help

# --- Build ------------------------------------------------------------

$(BASE_IMAGE): base.def
	https_proxy=$(PROXY) HTTPS_PROXY=$(PROXY) \
	apptainer build --force --bind /auto/software/slurm/slurm-ubuntu:/slurm-install:ro $@ $<

$(CLAUDE_IMAGE): claude-code.def $(BASE_IMAGE)
	https_proxy=$(PROXY) HTTPS_PROXY=$(PROXY) \
	apptainer build --force $@ $<

$(CODEX_IMAGE): codex.def $(BASE_IMAGE)
	https_proxy=$(PROXY) HTTPS_PROXY=$(PROXY) \
	apptainer build --force $@ $<

build-claude: $(CLAUDE_IMAGE) ## Build the Claude Code image (builds base first if needed)

build-codex: $(CODEX_IMAGE) ## Build the Codex CLI image (builds base first if needed)

build: build-claude build-codex ## Build both the Claude Code and Codex CLI images

build-base: $(BASE_IMAGE) ## Build the base image with system tools only

# --- Persistent homes ---------------------------------------------------

$(CLAUDE_HOME_DIR):
	mkdir -p $@

# bash -l (used by `make shell`) sources /etc/profile, which resets PATH and
# drops the /home/claude/.local/bin entry that base.def's %environment set.
# Seed ~/.profile to restore it so tools installed under .local/bin (uv,
# claude, etc.) stay reachable in interactive login shells.
$(CLAUDE_PROFILE): | $(CLAUDE_HOME_DIR)
	printf '%s\n' 'export PATH="$$HOME/.local/bin:$$PATH"' > $@

$(CODEX_HOME_DIR):
	mkdir -p $@

# See the note above $(CLAUDE_PROFILE): bash -l drops the .local/bin PATH
# entry, so seed ~/.profile in the Codex home too (codex.def's
# %environment sets /home/codex/.local/bin, which is where npm/uv/rustup
# install their tools).
$(CODEX_PROFILE): | $(CODEX_HOME_DIR)
	printf '%s\n' 'export PATH="$$HOME/.local/bin:$$PATH"' > $@

# Create the /tmp bind source right before use (the scratch dir is kept
# private since /scratch is typically world-readable at the top level),
# then prune old temp files in the background so startup isn't delayed.
claude-tmp:
	@mkdir -p "$(CLAUDE_TMP_SRC)" && chmod 700 "$(CLAUDE_TMP_SRC)"
	@( $(call prune_tmp,$(CLAUDE_TMP_SRC)) ) >/dev/null 2>&1 &

codex-tmp:
	@mkdir -p "$(CODEX_TMP_SRC)" && chmod 700 "$(CODEX_TMP_SRC)"
	@( $(call prune_tmp,$(CODEX_TMP_SRC)) ) >/dev/null 2>&1 &

# --- Run (interactive; running both tools at once isn't practical, so the
# bare targets stay single-tool and default to Claude Code for backward
# compatibility) --------------------------------------------------------

run-claude: $(CLAUDE_IMAGE) | $(CLAUDE_PROFILE) claude-tmp ## Run Claude Code (override work dir with DIR=/path)
	apptainer run $(CLAUDE_APPTAINER_FLAGS) $(CLAUDE_IMAGE)

run: run-claude ## Run Claude Code (alias for run-claude)

run-codex: $(CODEX_IMAGE) | $(CODEX_PROFILE) codex-tmp ## Run Codex CLI (override work dir with DIR=/path)
	apptainer run $(CODEX_APPTAINER_FLAGS) $(CODEX_IMAGE)

remote: $(CLAUDE_IMAGE) | $(CLAUDE_PROFILE) claude-tmp ## Run Claude Code with --remote-control (no Codex CLI equivalent)
	apptainer run $(CLAUDE_APPTAINER_FLAGS) $(CLAUDE_IMAGE) --remote-control

danger-claude: $(CLAUDE_IMAGE) | $(CLAUDE_PROFILE) claude-tmp ## Run Claude Code with --dangerously-skip-permissions
	apptainer run $(CLAUDE_APPTAINER_FLAGS) $(CLAUDE_IMAGE) --dangerously-skip-permissions

danger: danger-claude ## Run Claude Code with --dangerously-skip-permissions (alias for danger-claude)

danger-codex: $(CODEX_IMAGE) | $(CODEX_PROFILE) codex-tmp ## Run Codex CLI with --dangerously-bypass-approvals-and-sandbox
	apptainer run $(CODEX_APPTAINER_FLAGS) $(CODEX_IMAGE) --dangerously-bypass-approvals-and-sandbox

shell-claude: $(CLAUDE_IMAGE) | $(CLAUDE_PROFILE) claude-tmp ## Open a bash shell inside the Claude container
	apptainer exec $(CLAUDE_APPTAINER_FLAGS) $(CLAUDE_IMAGE) bash -l

shell: shell-claude ## Open a bash shell inside the Claude container (alias for shell-claude)

shell-codex: $(CODEX_IMAGE) | $(CODEX_PROFILE) codex-tmp ## Open a bash shell inside the Codex container
	apptainer exec $(CODEX_APPTAINER_FLAGS) $(CODEX_IMAGE) bash -l

# --- Toolchain installs into the persistent homes (non-interactive, so the
# bare targets run both) -------------------------------------------------

install-uv-claude: $(CLAUDE_IMAGE) | $(CLAUDE_PROFILE) claude-tmp ## Install uv into the persistent Claude home
	apptainer exec $(CLAUDE_APPTAINER_FLAGS) $(CLAUDE_IMAGE) \
	  sh -c 'curl -LsSf https://astral.sh/uv/install.sh | sh; \
	         grep -qxF "export UV_PROJECT_ENVIRONMENT=.venv-claude" "$$HOME/.profile" || \
	         echo "export UV_PROJECT_ENVIRONMENT=.venv-claude" >> "$$HOME/.profile"'

install-uv-codex: $(CODEX_IMAGE) | $(CODEX_PROFILE) codex-tmp ## Install uv into the persistent Codex home
	apptainer exec $(CODEX_APPTAINER_FLAGS) $(CODEX_IMAGE) \
	  sh -c 'curl -LsSf https://astral.sh/uv/install.sh | sh; \
	         grep -qxF "export UV_PROJECT_ENVIRONMENT=.venv-codex" "$$HOME/.profile" || \
	         echo "export UV_PROJECT_ENVIRONMENT=.venv-codex" >> "$$HOME/.profile"'

install-uv: install-uv-claude install-uv-codex ## Install uv into both persistent homes

install-rust-claude: $(CLAUDE_IMAGE) | $(CLAUDE_PROFILE) claude-tmp ## Install Rust toolchain into the persistent Claude home
	apptainer exec $(CLAUDE_APPTAINER_FLAGS) $(CLAUDE_IMAGE) \
	  sh -c 'curl --proto "=https" --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y'

install-rust-codex: $(CODEX_IMAGE) | $(CODEX_PROFILE) codex-tmp ## Install Rust toolchain into the persistent Codex home
	apptainer exec $(CODEX_APPTAINER_FLAGS) $(CODEX_IMAGE) \
	  sh -c 'curl --proto "=https" --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y'

install-rust: install-rust-claude install-rust-codex ## Install Rust toolchain into both persistent homes

# --- Agent skills (copied into the persistent homes; re-run to update. Only
# skills shipped in skills/ are overwritten, user-added skills are kept) ---

install-skills-claude: | $(CLAUDE_HOME_DIR) ## Install bundled skills (e.g. slurm-hpc) into the persistent Claude home
	mkdir -p $(CLAUDE_SKILLS_DIR)
	rsync -rh $(SKILLS_DIR)/ $(CLAUDE_SKILLS_DIR)/

install-skills-codex: | $(CODEX_HOME_DIR) ## Install bundled skills (e.g. slurm-hpc) into the persistent Codex home
	mkdir -p $(CODEX_SKILLS_DIR)
	rsync -rh $(SKILLS_DIR)/ $(CODEX_SKILLS_DIR)/

install-skills: install-skills-claude install-skills-codex ## Install bundled skills into both persistent homes

# --- Install the CLIs and launchers -------------------------------------

install-claude-cli: $(CLAUDE_IMAGE) | $(CLAUDE_PROFILE) claude-tmp ## Install Claude Code into the persistent home (user-updatable)
	apptainer exec $(CLAUDE_APPTAINER_FLAGS) $(CLAUDE_IMAGE) \
	  sh -c '[ -f "$$HOME/.local/bin/claude" ] && echo "Claude Code already installed, skipping." && exit 0; npm config set prefix "$$HOME/.local" && https_proxy=$(PROXY) HTTPS_PROXY=$(PROXY) npm install -g @anthropic-ai/claude-code'

install-codex-cli: $(CODEX_IMAGE) | $(CODEX_PROFILE) codex-tmp ## Install Codex CLI into the persistent home (user-updatable)
	apptainer exec $(CODEX_APPTAINER_FLAGS) $(CODEX_IMAGE) \
	  sh -c '[ -f "$$HOME/.local/bin/codex" ] && echo "Codex CLI already installed, skipping." && exit 0; npm config set prefix "$$HOME/.local" && https_proxy=$(PROXY) HTTPS_PROXY=$(PROXY) npm install -g @openai/codex'

install-claude: install-claude-cli ## Install claude-hpc launcher to ~/bin and image to ~/.containers/images
	mkdir -p $(HOME)/bin
	rsync -ah claude-hpc.sh $(HOME)/bin/claude-hpc
	chmod +x $(HOME)/bin/claude-hpc
	rsync -ah claude-hpc-shell.sh $(HOME)/bin/claude-hpc-shell
	chmod +x $(HOME)/bin/claude-hpc-shell
	mkdir -p $(HOME)/.containers/images
	rsync -ah --progress $(CLAUDE_IMAGE) $(HOME)/.containers/images/$(CLAUDE_IMAGE)

install-codex: install-codex-cli ## Install codex-hpc launcher to ~/bin and image to ~/.containers/images
	mkdir -p $(HOME)/bin
	rsync -ah codex-hpc.sh $(HOME)/bin/codex-hpc
	chmod +x $(HOME)/bin/codex-hpc
	rsync -ah codex-hpc-shell.sh $(HOME)/bin/codex-hpc-shell
	chmod +x $(HOME)/bin/codex-hpc-shell
	mkdir -p $(HOME)/.containers/images
	rsync -ah --progress $(CODEX_IMAGE) $(HOME)/.containers/images/$(CODEX_IMAGE)

install: install-claude install-codex ## Install both launchers to ~/bin and images to ~/.containers/images

# --- Persisted home management (non-interactive, so the bare targets run
# both) --------------------------------------------------------------

show-home-claude: ## Print the path to the persisted Claude home
	@echo $(CLAUDE_HOME_DIR)

show-home-codex: ## Print the path to the persisted Codex home
	@echo $(CODEX_HOME_DIR)

show-home: show-home-claude show-home-codex ## Print the paths to both persisted homes

clean-home-claude: ## Delete the persisted Claude home (resets auth and settings)
	rm -rf $(CLAUDE_HOME_DIR)

clean-home-codex: ## Delete the persisted Codex home (resets auth and settings)
	rm -rf $(CODEX_HOME_DIR)

clean-home: clean-home-claude clean-home-codex ## Delete both persisted homes (resets auth and settings)

# --- Node-local scratch (shared by both tools, so no -claude/-codex forms) --

show-scratch: ## Print the host dirs mounted at /tmp in the containers
	@echo "claude: $(CLAUDE_TMP_SRC)"
	@echo "codex:  $(CODEX_TMP_SRC)"

prune-scratch: ## Delete temp files older than AGENT_TMP_MAX_AGE_DAYS (default 30) now
	$(foreach d,$(sort $(CLAUDE_TMP_SRC) $(CODEX_TMP_SRC)),[ ! -d "$(d)" ] || { $(call prune_tmp,$(d)); };)

clean-scratch: ## Delete the node-local scratch dir (temp files only; stop running sessions first)
	rm -rf "$(AGENT_SCRATCH)"

help: ## Show available targets
	@grep -E '^[a-zA-Z_-]+:.*## ' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*## "}; {printf "  %-22s %s\n", $$1, $$2}'
