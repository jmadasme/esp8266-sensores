# Datasheet — OpenCode Pipelines Export Package

> **Version**: 1.0.0 | **Export date**: 2026-07-01 | **License**: MIT

---

## Table of Contents

- [Package Overview](#package-overview)
- [Technical Specifications](#technical-specifications)
- [System Requirements](#system-requirements)
- [Installation Guide](#installation-guide)
  - [Automated Installation](#automated-installation)
  - [Manual Installation](#manual-installation)
  - [Python Dependencies](#python-dependencies)
- [Usage Guide](#usage-guide)
  - [Using the Pipelines](#using-the-pipelines)
  - [Dashboard — Real-time Monitoring](#dashboard--real-time-monitoring)
  - [CLI pipeline_status.py](#cli-pipeline_statuspy)
- [Configuration Structure](#configuration-structure)
- [Troubleshooting Guide](#troubleshooting-guide)
- [Frequently Asked Questions (FAQ)](#frequently-asked-questions-faq)

---

## Package Overview

**OpenCode Pipelines Export** is a self-contained export package that allows you to back up, transfer, and replicate the complete OpenCode agent ecosystem from one machine to another. It includes:

- **19 AI agents** distributed across 3 specialized pipelines
- **Dashboard web app** (Flask) for real-time pipeline monitoring via Server-Sent Events (SSE)
- **CashDro integration skill** (REST/JSON over HTTPS)
- **Automatic installation script** (`setup_pipeline.sh`) with 8 automated steps
- **Complete documentation** (manifest, changelog, architectural vision)

### What is NOT this package?

- It is NOT a standalone application that runs independently
- It does NOT contain compiled binaries or pre-trained models
- The agents are defined as Markdown files — they only work inside the **OpenCode CLI** environment

### Package contents at a glance

| Component | Files | Count |
|-----------|-------|-------|
| Agents | `agents/*.md` | 19 |
| Dashboard | `dashboard/*.py`, `templates/`, `static/` | 7 |
| Skills | `skills/cashdro-integracion/` | 1 |
| Scripts | `setup_pipeline.sh` | 1 |
| Configuration | `opencode.jsonc`, `pipeline_state.json` | 2 |
| Documentation | `MANIFIESTO_COMPLETO.md`, `CHANGELOG.md`, etc. | 5+ |

---

## Technical Specifications

| Property | Value |
|----------|-------|
| **Package type** | Configuration export / backup |
| **Total agents** | 19 |
| **Pipelines** | 3 (Standard Python, Arduino, LOOP meta-optimization) |
| **Agent models** | `opencode-go/deepseek-v4-flash` (9 agents), `opencode-go/deepseek-v4-pro` (10 agents) |
| **Dashboard framework** | Flask 3.0+ |
| **Dashboard port** | 5050 (configurable) |
| **Dashboard real-time** | Server-Sent Events, 2-second interval |
| **State persistence** | JSON files with `fcntl.flock` thread-safety |
| **State file** | `pipeline_state.json` + `pipeline_state.json.lock` |
| **Session storage** | `proyectos/sesiones/<alias>/status.json` |
| **Max audit iterations** | 7 (implementador -> auditor cycle) |
| **Max LOOP iterations** | 10 (explorador-loop) |
| **Installation script** | Bash, 256 lines, 8 steps |
| **Supported OS** | Ubuntu/Debian Linux, macOS |
| **Required tools** | Node.js + npm, Python 3.8+, Git, GitHub CLI |
| **License** | MIT |

---

## System Requirements

### Minimum requirements (machine where the ecosystem will be installed)

| Requirement | Minimum Version | Purpose |
|-------------|-----------------|---------|
| **Operating System** | Ubuntu 20.04+, Debian 11+, macOS 11+ | Host environment |
| **Node.js** | 18.x or later | OpenCode CLI runtime |
| **npm** | 9.x or later | Package manager for OpenCode |
| **Python** | 3.8 or later | Dashboard Flask server |
| **pip** | 21.x or later | Python package manager |
| **Git** | 2.30 or later | Version control for projects |
| **GitHub CLI (gh)** | 2.0 or later | Repository creation and publishing |
| **Disk space** | ~500 MB | For the package, dependencies, and workspace |
| **Internet connection** | Required | For dependency installation and GitHub publishing |

### Optional but recommended

| Tool | Purpose |
|------|---------|
| **curl** | Used by `setup_pipeline.sh` for verification |
| **arduino-cli** | Required only for the Arduino pipeline |
| **Bash 4.0+** | For the installation script |

---

## Installation Guide

### Automated Installation

This is the recommended method. The script handles everything automatically.

**Step 1: Clone the repository**

```bash
git clone https://github.com/wanderc137/opencode-pipelines-export.git
cd opencode-pipelines-export
```

**Step 2: Configure your GitHub token**

Edit `setup_pipeline.sh` and replace the placeholder:

```bash
# Line 12 — replace this:
TOKEN="[REDACTED]"
# With your actual token:
TOKEN="[REDACTED]"
```

To generate a token: GitHub Settings -> Developer settings -> Personal access tokens -> Fine-grained tokens. Required scopes: `repo` (Contents: Read and write).

**Step 3: Run the installer**

```bash
chmod +x setup_pipeline.sh
bash setup_pipeline.sh
```

**What the script does (8 steps):**

| Step | Action | Details |
|------|--------|---------|
| PASO 0 | Install system dependencies | Detects OS, installs `nodejs`, `npm`, `git`, `python3`, `pip`, `gh`, `curl` |
| PASO 1 | Install OpenCode CLI | `sudo npm install -g @opencode/cli` |
| PASO 2 | Initialize OpenCode | Creates `~/.config/opencode/` directory |
| PASO 3 | Write agent configuration | Generates `~/.config/opencode/opencode.jsonc` with all 5 core agents |
| PASO 4 | Authenticate GitHub | Logs in as `wanderc137` using the PAT token (secure temp file) |
| PASO 5 | Create workspace structure | `~/Documentos/Proyects/OpenCode/proyectos/` |
| PASO 6 | Configure git | Sets `user.name`, `user.email`, `defaultBranch main` |
| PASO 7 | Verify installation | Checks OpenCode, config, GitHub auth, directory structure |
| PASO 8 | Show summary | Displays paths, commands, and next steps |

**Expected output on success:**

```
╔══════════════════════════════════════════════════════╗
║           PIPELINE INSTALLED SUCCESSFULLY          ║
╚══════════════════════════════════════════════════════╝

  Workspace:    ~/Documentos/Proyects/OpenCode
  Config:       ~/.config/opencode/opencode.jsonc
  GitHub:       wanderc137
  Projects:     ~/Documentos/Proyects/OpenCode/proyectos/
  Agents:       analista -> implementador -> auditor -> documentador -> publicador

  To start:
     cd ~/Documentos/Proyects/OpenCode
     opencode
     @analista [technical specification]
```

### Manual Installation

If you prefer to install each component manually:

```bash
# 1. Install OpenCode CLI
sudo npm install -g @opencode/cli

# 2. Initialize OpenCode
mkdir -p ~/.config/opencode
opencode init

# 3. Copy agent configuration
cp opencode.jsonc ~/.config/opencode/

# 4. Authenticate GitHub
# Save your token to a temp file first:
echo "github_pat_your_token" > /tmp/gh-token
gh auth login --with-token < /tmp/gh-token
rm /tmp/gh-token

# 5. Create workspace
mkdir -p ~/Documentos/Proyects/OpenCode/proyectos

# 6. Configure git
git config --global user.name "wanderc137"
git config --global user.email "wanderc137@gmail.com"
git config --global init.defaultBranch main

# 7. Install dashboard dependencies (see Python Dependencies section)
```

### Python Dependencies

The dashboard requires Flask. The dependencies are defined in `requirements.txt` at the project root.

**Full dependency list from `requirements.txt`:**

| Package | Version | Purpose |
|---------|---------|---------|
| `flask` | `>=3.0.0` | Web framework — serves the dashboard HTML, REST API, and SSE stream |

**Installation with a virtual environment (recommended):**

```bash
cd opencode-pipelines-export
python3 -m venv venv
source venv/bin/activate        # Linux/macOS
# venv\Scripts\activate         # Windows
pip install -r requirements.txt
```

**Verification:**

```bash
python3 -c "import flask; print('Flask', flask.__version__)"
# Expected output: Flask 3.x.x
```

**Why use a virtual environment?**

- Isolates dashboard dependencies from system Python packages
- Prevents conflicts with other projects
- Easy to recreate: delete `venv/` and reinstall
- The `venv/` directory is excluded from git via `.gitignore`

---

## Usage Guide

### Using the Pipelines

Once installed, the pipelines are activated through the OpenCode CLI by invoking the orquestador agent.

**Starting a new project (Pipeline Standard):**

```bash
cd ~/Documentos/Proyects/OpenCode
opencode

# Inside the opencode session, send a command to the orquestador:
@analista MyProject, "Create a Python web scraper that extracts product prices from an e-commerce site"
```

The orquestador will:

1. **Register** the project in the dashboard
2. **Clarify** requirements with interactive questions
3. **Delegate** to analista -> implementador -> auditor -> documentador -> publicador
4. **Report** the final GitHub URL

**Using the Pipeline Arduino:**

```bash
@analista TempMonitor, "Arduino project: read DHT22 sensor, display on LCD1602 I2C, log to Serial"
```

The Arduino-specific agents handle pin mapping, FQBN selection, and physical board transfer.

**Using the Pipeline LOOP:**

```bash
@explorador-loop PROYECTO_DIR=~/proyectos/btc-scalping OBJETIVO="Create a BTC/USDT scalping algorithm" META="10% monthly profit" CAPITAL=1000
```

The explorador-loop runs up to 10 iterations, each generating a complete sub-project with backtest results in SQLite.

### Dashboard — Real-time Monitoring

All pipeline progress can be monitored in real time through the web dashboard.

**Starting the dashboard:**

```bash
cd opencode-pipelines-export
source venv/bin/activate
python3 dashboard/app.py 5050
```

**Accessing the dashboard:**

- Open browser: `http://localhost:5050`
- From another machine on the network: `http://<IP>:5050`
- Using hostname (if configured): `http://metagentes:5050`

**Dashboard features:**

| Feature | Description |
|---------|-------------|
| Session cards | Each pipeline appears as a card with name, status dot, progress bar |
| Step tracking | All 6 pipeline steps shown with icons, status (pending/in-progress/completed/error) |
| Progress bar | Gradient progress bar with gleam animation, real-time updates |
| Iteration counter | Shows how many implementador->auditor cycles have occurred |
| Comment log | Displays the latest comment/status message for each step |
| Log preview | Last 10 lines of the session log in a terminal-style box |
| Counters | Header shows total sessions, active (running), and completed |
| Responsive | Adapts to desktop, tablet, and mobile screens |

**SSE (Server-Sent Events) endpoint:**

The dashboard updates automatically every 2 seconds via SSE. If the connection is lost, the frontend automatically reconnects.

```bash
# Test SSE manually:
curl -N http://localhost:5050/stream
# You'll see data: {"sessions":[...]} every 2 seconds
```

### CLI pipeline_status.py

Agents report their state using the `pipeline_status.py` command-line helper. This is typically invoked by the orquestador agent, not manually.

**Location:** `dashboard/pipeline_status.py`

**Syntax:**

```bash
python3 dashboard/pipeline_status.py <project> <action> [comment]
```

**Available actions:**

| Action | Description | Example |
|--------|-------------|---------|
| `start` | Register and start a pipeline | `python3 dashboard/pipeline_status.py MyProject start "Starting analysis"` |
| `complete` | Mark current step as completed | `python3 dashboard/pipeline_status.py MyProject complete "PLAN.md created"` |
| `advance` | Move to the next step | `python3 dashboard/pipeline_status.py MyProject advance` |
| `error` | Mark current step as errored | `python3 dashboard/pipeline_status.py MyProject error "Auditor failed"` |
| `comment` | Add a comment to the current step | `python3 dashboard/pipeline_status.py MyProject comment "Clarification done"` |
| `inc_iterations` | Increment the iteration counter | `python3 dashboard/pipeline_status.py MyProject inc_iterations "Cycle 3 of 7"` |
| `set_step` | Jump to a specific step | `python3 dashboard/pipeline_status.py MyProject set_step implementador` |
| `check_iterations` | Check if iteration limit reached | `python3 dashboard/pipeline_status.py MyProject check_iterations` |
| `state` | Show full pipeline state | `python3 dashboard/pipeline_status.py MyProject state` |

**Example flow:**

```bash
python3 dashboard/pipeline_status.py MiSensor start "Iniciando pipeline"
python3 dashboard/pipeline_status.py MiSensor comment "Requerimiento clarificado"
python3 dashboard/pipeline_status.py MiSensor complete "PLAN.md creado"
python3 dashboard/pipeline_status.py MiSensor advance
# now on step: analista -> implementador
python3 dashboard/pipeline_status.py MiSensor state
# Pipeline: MiSensor
#   Current step: implementador
#   Completed: orquestador, analista
#   Progress: 40%
```

---

## Configuration Structure

### opencode.jsonc

The main OpenCode configuration file. When installed via `setup_pipeline.sh`, it is written to `~/.config/opencode/opencode.jsonc`.

```json
{
  "$schema": "https://opencode.ai/config.json",
  "default_agent": "analista",
  "plugin": ["opencode-dynamic-context-pruning"],
  "skills": {
    "paths": ["/home/<user>/.config/opencode/skills"]
  },
  "agent": {
    "analista": { "... agent definition ..." },
    "implementador": { "... agent definition ..." },
    "auditor": { "... agent definition ..." },
    "documentador": { "... agent definition ..." },
    "publicador": { "... agent definition ..." }
  }
}
```

### Agent file format

Each agent is defined in a Markdown file (`agents/<name>.md`) with YAML frontmatter:

```yaml
---
description: "Agent description"
mode: all
model: opencode-go/deepseek-v4-flash
permission:
  "*": allow
---

# Agent Name

Agent personality, workflow, rules...
```

### pipeline_state.json

Tracks the state of all pipelines. Automatically created and maintained by the system.

### Session status

Each pipeline session stores its state in `proyectos/sesiones/<alias>/status.json`.

---

## Troubleshooting Guide

### Problem: setup_pipeline.sh fails at PASO 0 (Dependencies)

**Symptoms:** Error messages about `apt` or `brew` not found, packages failing to install.

**Solutions:**
1. Verify your OS is supported: `echo $OSTYPE` (should be `linux-gnu` or `darwin`)
2. For Ubuntu/Debian: `sudo apt update && sudo apt install -y nodejs npm git python3 python3-pip gh curl`
3. For macOS: Install Homebrew first: `/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"` then `brew install node git gh python3`
4. For other Linux distributions: Install dependencies manually using your package manager

### Problem: GitHub authentication fails (PASO 4)

**Symptoms:** `gh auth status` shows "not logged in" or incorrect user.

**Solutions:**
1. Verify your token has the correct scopes (`repo` — Contents: Read and write)
2. Check the token is not expired: GitHub Settings -> Developer settings -> Personal access tokens
3. Try manual authentication: `gh auth login --with-token < ~/mi-token.txt`
4. If behind a proxy: `gh auth login --hostname github.com --with-token`

### Problem: Dashboard fails to start

**Symptoms:** `ModuleNotFoundError: No module named 'flask'` or port already in use.

**Solutions:**
1. Install Flask in the virtual environment:
   ```bash
   cd opencode-pipelines-export
   python3 -m venv venv
   source venv/bin/activate
   pip install -r requirements.txt
   ```
2. Check if port 5050 is already in use: `lsof -i :5050` or `ss -tlnp | grep 5050`
3. Kill the existing process: `kill $(lsof -t -i :5050)` or use a different port: `python3 dashboard/app.py 5051`
4. Verify Python version: `python3 --version` (must be 3.8+)

### Problem: OpenCode CLI not found after installation

**Symptoms:** `opencode: command not found`

**Solutions:**
1. Verify npm global bin directory is in PATH:
   ```bash
   npm bin -g
   export PATH="$PATH:$(npm bin -g)"
   ```
2. Reinstall: `sudo npm install -g @opencode/cli`
3. Check installation: `which opencode` or `npm list -g @opencode/cli`

### Problem: pipeline_state.json corrupted

**Symptoms:** Dashboard shows incorrect state, `json.JSONDecodeError` in dashboard logs.

**Solutions:**
1. Stop the dashboard: `kill $(lsof -t -i :5050)`
2. Reset the state file:
   ```bash
   echo '{"pipelines":{}}' > pipeline_state.json
   ```
3. Remove the lock file: `rm pipeline_state.json.lock`
4. Restart the dashboard

### Problem: Agents fail with "PROYECTO_DIR not found"

**Symptoms:** Agent reports that it cannot find the project directory.

**Solutions:**
1. Verify the directory exists: `ls -la <PROYECTO_DIR>`
2. Check the path is absolute (not relative)
3. Verify the workspace structure was created properly by `setup_pipeline.sh`
4. The project directory must be inside `~/Documentos/Proyects/OpenCode/proyectos/`

### Problem: publish step fails (publicador agent)

**Symptoms:** GitHub push fails, repository not created.

**Solutions:**
1. Verify authentication: `gh auth status`
2. Verify the account: should show `wanderc137`
3. Verify git configuration: `git config --global --list`
4. Check if the repository already exists: `gh repo view wanderc137/<project-name>`
5. Verify there are no `PLAN.md` or `AUDIT_REPORT.md` files being included (they should be in `.gitignore`)

---

## Frequently Asked Questions (FAQ)

### General

**Q: Can I use this on Windows?**
A: The installation script (`setup_pipeline.sh`) is designed for Linux and macOS only. For Windows, you must install all components manually using WSL2 (Windows Subsystem for Linux). The dashboard itself works on Windows if Python and Flask are installed.

**Q: Do I need a paid OpenCode subscription?**
A: The configuration uses `deepseek-v4-flash` and `deepseek-v4-pro` models. Check with OpenCode's documentation for pricing. The pipeline architecture is model-agnostic — you can change models in the agent frontmatter.

**Q: How many pipelines can run simultaneously?**
A: Theoretically unlimited. The state manager uses file-level locking (`fcntl.flock`) to serialize writes, ensuring data integrity even with concurrent pipelines. The dashboard monitors all sessions in real time.

**Q: Can I create my own pipelines?**
A: Yes. See [How to add agents](../README.md#c%C3%B3mo-a%C3%B1adir-o-eliminar-agentes) in the README. You can define new agents, create their Markdown files, register them in `opencode.jsonc`, and define a new orchestration flow.

### Dashboard

**Q: Can I access the dashboard from another computer?**
A: Yes. The dashboard binds to `0.0.0.0`, making it accessible from any machine on the local network at `http://<server-ip>:5050`. For remote access, use an SSH tunnel or configure a reverse proxy with authentication.

**Q: Does the dashboard require a database?**
A: No. All state is persisted in JSON files (`pipeline_state.json` and `proyectos/sesiones/<alias>/status.json`). No PostgreSQL, MySQL, or MongoDB required.

**Q: What happens if I close the browser?**
A: Nothing — the dashboard is just a viewer. Pipelines continue running. Reopen the browser at `http://localhost:5050` to reconnect. The SSE stream reconnects automatically.

**Q: Can I customize the dashboard port?**
A: Yes. Pass the port as an argument: `python3 dashboard/app.py 8080`. The port is configurable per launch.

### Security

**Q: Is the dashboard secure?**
A: The dashboard has NO built-in authentication. It is designed for local/private network use only. Do NOT expose it directly to the internet. Use an SSH tunnel (`ssh -L 5050:localhost:5050 user@server`) for remote access, or put it behind a reverse proxy (nginx) with basic auth.

**Q: What should I do if I accidentally committed my GitHub token?**
A: 1. Immediately revoke the token at GitHub Settings -> Developer settings -> Personal access tokens. 2. Generate a new token. 3. Force-push an amended commit or use `git filter-branch` / `bfg-repo-cleaner` to remove the token from history. 4. Enable branch protection rules on the repository.

### Maintenance

**Q: How do I update the package?**
A: Pull the latest version from GitHub: `git pull origin main`. Then re-run `bash setup_pipeline.sh`. The script handles incremental updates — it backs up existing configuration before overwriting.

**Q: How do I back up my pipeline state?**
A: Copy `pipeline_state.json` and the `proyectos/sesiones/` directory. These contain all pipeline progress and session data. Restore them to resume pipelines after migration.

**Q: Can I change the GitHub user from `wanderc137` to my own?**
A: Yes. Edit `setup_pipeline.sh` lines 12-13 and change `GITHUB_USER` and the git configuration in PASO 6. Also update agent agent definitions that reference `wanderc137` in the `publicador` agents.
