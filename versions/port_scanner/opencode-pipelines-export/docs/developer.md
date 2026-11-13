# Developer Guide — OpenCode Pipelines Export

> **Audience**: Developers extending, modifying, or understanding the codebase
> **Last updated**: 2026-07-01

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Agent System](#agent-system)
  - [Agent file format](#agent-file-format)
  - [Model assignment strategy](#model-assignment-strategy)
  - [How agents communicate](#how-agents-communicate)
  - [Agent lifecycle in a pipeline](#agent-lifecycle-in-a-pipeline)
- [Dashboard Architecture](#dashboard-architecture)
  - [app.py — Flask application](#apppy--flask-application)
  - [pipeline_state.py — State manager](#pipeline_statepy--state-manager)
  - [pipeline_status.py — CLI helper](#pipeline_statuspy--cli-helper)
  - [Frontend (HTML/CSS/JS)](#frontend-htmlcssjs)
- [Pipeline Flow in Detail](#pipeline-flow-in-detail)
  - [Standard Pipeline flow](#standard-pipeline-flow)
  - [Arduino Pipeline flow](#arduino-pipeline-flow)
  - [LOOP Pipeline flow](#loop-pipeline-flow)
- [State Management](#state-management)
  - [Data structures](#data-structures)
  - [Thread safety](#thread-safety)
  - [Progress calculation](#progress-calculation)
- [How to Extend](#how-to-extend)
  - [Adding a new agent](#adding-a-new-agent)
  - [Creating a new pipeline](#creating-a-new-pipeline)
  - [Adding a new dashboard feature](#adding-a-new-dashboard-feature)
- [API Reference](#api-reference)
  - [Dashboard REST API](#dashboard-rest-api)
  - [pipeline_state.py API](#pipeline_statepy-api)
  - [pipeline_status.py CLI API](#pipeline_statuspy-cli-api)
- [Codebase Map](#codebase-map)

---

## Architecture Overview

The OpenCode Pipelines Export package follows a **layered architecture** with three main subsystems:

```
┌─────────────────────────────────────────────────────────────┐
│                     LAYER 1: AGENTS                          │
│  agents/*.md  (19 Markdown files with YAML frontmatter)      │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐               │
│  │ Standard │  │ Arduino  │  │    LOOP      │               │
│  │ 6 agents │  │ 6 agents │  │  7 agents    │               │
│  └──────────┘  └──────────┘  └──────────────┘               │
├─────────────────────────────────────────────────────────────┤
│                   LAYER 2: DASHBOARD                          │
│  dashboard/*.py  (Flask app + state manager + CLI)            │
│  ┌──────────┐  ┌──────────────────┐  ┌───────────────────┐  │
│  │ app.py   │  │ pipeline_state.py│  │ pipeline_status.py│  │
│  │ Flask    │  │ State mgmt       │  │ Agent CLI helper  │  │
│  └──────────┘  └──────────────────┘  └───────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Frontend: index.html + _session_card.html           │   │
│  │            style.css (glassmorphism)                 │   │
│  │            dashboard.js (SSE client)                 │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                  LAYER 3: INFRASTRUCTURE                      │
│  ┌─────────────────┐  ┌────────────────┐  ┌───────────────┐ │
│  │ opencode.jsonc  │  │ setup_pipeline │  │ skills/       │ │
│  │ Config          │  │ .sh (installer)│  │ cashdro/      │ │
│  └─────────────────┘  └────────────────┘  └───────────────┘ │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Data: pipeline_state.json + sesiones/*/status.json  │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

**Key design principles:**

1. **Agents are configuration, not code** — Each agent is a Markdown file interpreted by OpenCode CLI at runtime
2. **File-based communication** — Agents communicate through files on disk (`PLAN.md`, `AUDIT_REPORT.md`, source code)
3. **Single source of truth** — `pipeline_state.json` is the canonical state store for all pipelines
4. **Thread-safe by design** — All state mutations use `fcntl.flock` for cross-process atomicity

---

## Agent System

### Agent file format

Each agent is defined in a Markdown file with **YAML frontmatter** and a Markdown body. The frontmatter is parsed by OpenCode CLI; the body is the agent's system prompt.

**Frontmatter schema:**

```yaml
---
description: >
  Multi-line description of the agent's role.
mode: all                           # all = full access mode
model: opencode-go/deepseek-v4-pro  # or deepseek-v4-flash
permission:
  "*": allow                        # unrestricted permissions
---
```

**Body structure** (example from `orquestador.md`):

```markdown
# Agent Name

Eres el **agente principal**. Your mission is to...

## Pipeline steps

### 1 Registration
### 2 Clarification
### 3 Analysis
...

## Fundamental rules
- Rule 1
- Rule 2
```

**Agent files inventory:**

| Pipeline | Agents | Files |
|----------|--------|-------|
| Standard | orquestador, analista, implementador, auditor, documentador, publicador | `agents/orquestador.md`, `agents/analista.md`, ... |
| Arduino | orquestador-ar, analista-ar, ... | `agents/orquestador-ar.md`, ... |
| LOOP | explorador-loop, orquestador-loop, analista-loop, ... | `agents/explorador-loop.md`, ... |

### Model assignment strategy

The model assignment follows a clear pattern:

| Role type | Model | Rationale |
|-----------|-------|-----------|
| **Orchestrators** (3) | `deepseek-v4-flash` | Coordination tasks — fast, low-cost, deterministic |
| **Implementers** (3) | `deepseek-v4-flash` | Code generation — needs speed over deep reasoning |
| **Publishers** (3) | `deepseek-v4-flash` | Git/GitHub automation — straightforward commands |
| **Analysts** (3) | `deepseek-v4-pro` | Requirement analysis — needs deep reasoning and structured output |
| **Auditors** (3) | `deepseek-v4-pro` | Code review — needs thorough static analysis |
| **Documenters** (3) | `deepseek-v4-pro` | Documentation — needs comprehensive understanding |
| **Explorer** (1) | `deepseek-v4-pro` | Meta-optimization — needs strategic thinking across iterations |

**Total: 9 flash + 10 pro = 19 agents**

### How agents communicate

Agents do NOT communicate directly. All inter-agent communication flows through:

1. **Files on disk**:
   - `PLAN.md` — Written by the analyst, read by the implementer and auditor
   - `AUDIT_REPORT.md` — Written by the auditor, read by the orchestrator
   - Source code files — Written by the implementer, read by the auditor and documenter
   - `README.md`, `docs/datasheet.md`, `docs/developer.md` — Written by the documenter

2. **OpenCode `task` tool**:
   - The orchestrator invokes other agents using the `task` tool
   - Each `task` call includes `PROYECTO_DIR=<path>` and specific instructions
   - The invoked agent reads files from `PROYECTO_DIR`, performs its task, and returns

3. **Dashboard CLI** (`pipeline_status.py`):
   - Agents call `$PSTATUS` commands to report state
   - This writes to `pipeline_state.json` and `proyectos/sesiones/<alias>/status.json`

**Communication diagram:**

```
ORCHESTRATOR (flash)
    │
    ├── task("analista", "Crea PLAN.md...")
    │       └── analista (pro) writes → PLAN.md
    │
    ├── reads PLAN.md, verifies it exists
    │
    ├── task("implementador", "Implementa segun PLAN.md...")
    │       └── implementador (flash) reads PLAN.md → writes code files
    │
    ├── task("auditor", "Audita el codigo...")
    │       └── auditor (pro) reads code → writes AUDIT_REPORT.md
    │
    ├── reads AUDIT_REPORT.md → decision (approved / needs fixes)
    │
    ├── task("documentador", "Genera documentacion...")
    │       └── documentador (pro) reads code → writes README + docs/
    │
    └── task("publicador", "Sube a GitHub...")
            └── publicador (flash) → git init, commit, push
```

### Agent lifecycle in a pipeline

```
1. ORCHESTRATOR receives user request → starts pipeline registration
2. ORCHESTRATOR asks clarifying questions → user responds
3. ORCHESTRATOR invokes ANALYST → ANALYST creates PLAN.md → returns
4. ORCHESTRATOR verifies PLAN.md → invokes IMPLEMENTER
5. IMPLEMENTER reads PLAN.md → creates code → returns
6. ORCHESTRATOR invokes AUDITOR → AUDITOR reviews → creates AUDIT_REPORT.md → returns
7. ORCHESTRATOR reads AUDIT_REPORT.md:
   - If APPROVED → invokes DOCUMENTER
   - If INCIDENTS → updates PLAN.md, goes back to step 4 (max 7 cycles)
8. DOCUMENTER reads code → creates README + docs/ → returns
9. ORCHESTRATOR invokes PUBLISHER → git push to GitHub → returns
10. ORCHESTRATOR reports final result to user
```

---

## Dashboard Architecture

The dashboard is a **three-component Flask application**:

```
dashboard/
├── app.py              ← Entry point, HTTP routes, SSE stream
├── pipeline_state.py   ← State management, file I/O, thread safety
├── pipeline_status.py  ← CLI for agents to report state
├── __init__.py         ← Python package marker
├── templates/
│   ├── index.html      ← Main page layout
│   └── _session_card.html ← Reusable session card component
└── static/
    ├── css/style.css   ← Glassmorphism dark theme
    └── js/dashboard.js ← SSE client + dynamic rendering
```

### app.py — Flask application

**File**: `dashboard/app.py` (92 lines)

**Purpose**: HTTP server that serves the dashboard UI and exposes REST/SSE endpoints.

**Key design decisions:**

- **No persistent DB** — All data comes from JSON files via `pipeline_state.py`
- **SSE for real-time** — Uses Flask's streaming responses to push data every 2 seconds
- **Threaded mode** — `app.run(threaded=True)` for concurrent SSE connections
- **Bind all interfaces** — `host="0.0.0.0"` for network accessibility

**Routes:**

```python
@app.route("/")
def index():
    """Renders the main dashboard HTML with all session cards."""
    sessions = get_all_sessions()  # from pipeline_state.py
    return render_template("index.html", **_format_session_for_template(sessions))

@app.route("/api/state")
def api_state():
    """Returns JSON with all session states."""
    return jsonify(_format_session_for_template(get_all_sessions()))

@app.route("/stream")
def stream():
    """SSE endpoint — pushes state every 2 seconds."""
    def event_stream():
        while True:
            new_data = _format_session_for_template(get_all_sessions())
            yield f"data: {json.dumps(new_data)}\n\n"
            time.sleep(2)
    return Response(event_stream(), mimetype="text/event-stream",
                    headers={"Cache-Control": "no-cache", "Connection": "keep-alive"})

@app.route("/health")
def health():
    """Simple health check."""
    return jsonify({"status": "ok", "sessions": len(get_all_sessions())})
```

**`_format_session_for_template(sessions)`** — The core transformation function:
- Takes raw session data from `get_all_sessions()`
- Maps each session into a template-friendly structure with:
  - `alias`, `estado`, `pid`, `inicio`, `etapa`, `spec`, `log_preview`
  - `progress` (0-100), `completed` (boolean), `current_step`
  - `steps[]` — each step with `key`, `label`, `icon`, `status`, `iterations`, `comment`, `active`

**Startup:**

```bash
python3 dashboard/app.py 5050
# Port is optional — defaults to 5050 in the code
```

### pipeline_state.py — State manager

**File**: `dashboard/pipeline_state.py` (373 lines)

**Purpose**: The single source of truth for all pipeline state. Manages JSON persistence with thread-safe file locking.

**Core constants:**

```python
STEPS_ORDER = ["orquestador", "analista", "implementador", "auditor", "documentador", "publicador"]

STEP_WEIGHTS = {
    "orquestador": 10,     # 10% of total progress
    "analista": 15,        # 15%
    "implementador": 30,   # 30% (heaviest — actual coding)
    "auditor": 15,         # 15%
    "documentador": 15,    # 15%
    "publicador": 15       # 15%
}

MAX_ITERATIONS = 7  # Max implementer->auditor cycles
```

**Locking mechanism:**

```python
def _lock_fd():
    """Creates and acquires an exclusive file lock."""
    LOCK_FILE.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(str(LOCK_FILE), os.O_CREAT | os.O_RDWR, 0o644)
    fcntl.flock(fd, fcntl.LOCK_EX)  # Exclusive lock — blocks until acquired
    return fd

def _unlock_fd(fd):
    """Releases the lock and closes the file descriptor."""
    try:
        fcntl.flock(fd, fcntl.LOCK_UN)
    finally:
        os.close(fd)
```

**Context manager pattern:**

```python
@contextmanager
def _locked_access():
    """Context manager that acquires the lock, yields data, releases lock."""
    fd = _lock_fd()
    try:
        yield _read_raw()  # Read inside the lock
    finally:
        _unlock_fd(fd)     # Always release
```

**Key functions:**

| Function | Signature | Side effects | Thread-safe? |
|----------|-----------|-------------|--------------|
| `register_pipeline(name)` | Registers a new pipeline | Creates entry in `pipeline_state.json` + `status.json` | Yes (lock) |
| `update_step(pipeline, step, status, comment, inc_iterations)` | Updates a single step | Modifies step entry, recalculates progress | Yes (lock) |
| `advance_step(pipeline)` | Moves to next step | Marks current complete, next in-progress | Yes (lock) |
| `complete_last_step(pipeline)` | Atomic completion | Marks final step complete, sets 100% | Yes (lock) |
| `set_current_step(pipeline, target_step)` | Jumps to any step | Resets downstream steps to pending | Yes (lock) |
| `check_iteration_limit(name)` | Returns `(exceeded, current, max)` | Read-only | Yes (lock) |
| `get_pipeline_summary(name)` | Returns structured summary | Read-only | Yes (lock) |
| `get_all_sessions()` | Returns all sessions merged | Reads from dirs + state file | Cached (1s TTL) |

**Dual-write pattern:**

Every mutation writes to TWO locations:
1. `pipeline_state.json` — Global state (via `_write_state()` inside the lock)
2. `proyectos/sesiones/<name>/status.json` — Session-specific state (via `_write_sesion_status()` outside the lock)

This ensures both the global dashboard view and individual session files stay synchronized.

### pipeline_status.py — CLI helper

**File**: `dashboard/pipeline_status.py` (93 lines)

**Purpose**: Provides a simple CLI that agents invoke to report their state. The orquestador agent defines a shell alias:

```bash
PSTATUS='python3 dashboard/pipeline_status.py'
```

Then agents use it like:

```bash
$PSTATUS MiProyecto start 'Iniciando pipeline'
$PSTATUS MiProyecto complete 'Codigo implementado'
$PSTATUS MiProyecto advance
```

**Command dispatch:**

```python
if accion == "start":
    register_pipeline(proyecto)
    update_step(proyecto, current, 'in_progress', comentario)
elif accion == "complete":
    if current == STEPS_ORDER[-1]:
        complete_last_step(proyecto)  # Atomic 100% completion
    else:
        update_step(proyecto, current, 'completed', comentario)
elif accion == "advance":
    advance_step(proyecto)
elif accion == "error":
    update_step(proyecto, current, 'error', comentario)
# ... etc
```

### Frontend (HTML/CSS/JS)

**Templates:**

- `index.html` — Main page with Jinja2 templating. Contains header (counters), session grid, footer. Uses Bootstrap 5 + Bootstrap Icons CDN.
- `_session_card.html` — Reusable card component. Receives `session` object. Renders status dot, progress bar, step timeline, log preview.

**`dashboard.js` (160 lines) — SSE client:**

```javascript
function connectSSE() {
    const eventSource = new EventSource('/stream');
    
    eventSource.onmessage = function(e) {
        const data = JSON.parse(e.data);
        if (data && data.sessions) {
            updateDashboard(data);  // Re-render all session cards
        }
    };
    
    eventSource.onerror = function() {
        console.warn('SSE connection lost, retrying in 3s...');
        eventSource.close();
        setTimeout(connectSSE, 3000);  // Auto-reconnect
    };
}
```

The `updateDashboard()` function:
1. Counts total, running, and completed sessions
2. Updates header counters
3. If no sessions: shows empty state with usage hint
4. If sessions exist: renders each session card via `renderSession()`
5. Updates "last update" timestamp

**`style.css` (525 lines) — Glassmorphism theme:**

```css
body {
    background: linear-gradient(135deg, #0f0c29 0%, #1a1a3e 40%, #24243e 100%);
}

.session-card {
    background: rgba(255, 255, 255, 0.04);
    backdrop-filter: blur(10px);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 1.2rem;
}
```

Animations included:
- `shimmer` — Top border gradient sweep
- `pulse-dot` — Status indicator breathing
- `active-glow` / `icon-breathe` — Active step highlighting
- `shake` — Error state vibration
- `badge-pop` — Completion badge entrance
- `progress-gleam` — Progress bar light sweep
- `fadeInUp` — Card entrance
- `slide-in` — New session card animation

---

## Pipeline Flow in Detail

### Standard Pipeline flow

```
User: "@analista MyProject, 'Create a Python web scraper...'"
  │
  ▼
ORCHESTRADOR (orquestador.md, 280 lines)
  │
  ├─ Step 0: REANUDACION — Check if pipeline already exists via $PSTATUS
  │
  ├─ Step 1: REGISTRO — $PSTATUS <project> start
  │
  ├─ Step 2: CLARIFICATION — Ask user interactive questions about:
  │     - Main functionality, inputs, outputs
  │     - Framework, libraries, Python version
  │     - Hardware (pins, protocols: I2C/SPI/UART)
  │     - Edge cases and error handling
  │     - Platform/performance constraints
  │
  ├─ Step 3: ANALYZE — task("analista", "Crea PLAN.md...")
  │     ANALYST (pro) creates:
  │       - Requirement summary
  │       - Functional specification
  │       - Technical design (modules, classes, data flow)
  │       - File structure (concrete list)
  │       - Test cases
  │       - .gitignore
  │     Verification: PLAN.md exists, >100 bytes
  │
  ├─ Step 4: IMPLEMENT — task("implementador", "Implementa...")
  │     IMPLEMENTER (flash) reads PLAN.md, creates:
  │       - Python source files
  │       - venv/ environment
  │       - requirements.txt
  │     Verification: Files match PLAN.md structure
  │
  ├─ Step 5: AUDIT — task("auditor", "Audita...")
  │     AUDITOR (pro) reviews:
  │       - Integrity: Are all planned files present?
  │       - Security: Input validation, secrets, permissions
  │       - Functionality: Logic correctness, edge cases
  │       - Stability: Error handling, resource cleanup
  │       - Performance: Algorithmic complexity
  │     Output: AUDIT_REPORT.md
  │
  ├─ Step 6: DECISION
  │     Reads AUDIT_REPORT.md:
  │       Case A — "APROBADO — SIN INCIDENCIAS" → Go to step 7
  │       Case B — Incidences found:
  │         1. $PSTATUS set_step implementador
  │         2. Update PLAN.md with corrections
  │         3. Go back to step 4 (max 7 cycles)
  │         4. If 7 cycles exceeded → Ask user
  │
  ├─ Step 7: DOCUMENT — task("documentador", "Genera docs...")
  │     DOCUMENTADOR (pro) creates:
  │       - README.md (device explanation, pinout, usage, examples)
  │       - docs/datasheet.md (user guide)
  │       - docs/developer.md (source code explanation)
  │     Verification: README.md exists, >200 bytes
  │
  ├─ Step 8: PUBLISH — task("publicador", "Sube a GitHub...")
  │     PUBLICADOR (flash):
  │       - git init inside PROYECTO_DIR
  │       - git add . (excluding PLAN.md, AUDIT_REPORT.md)
  │       - git commit -m "Initial commit"
  │       - git remote add origin git@github.com:wanderc137/<project>.git
  │       - git push -u origin main
  │     Verification: .git/ exists with at least 1 commit
  │
  └─ Step 9: REPORT — Tell user:
        - GitHub repo URL
        - Files created
        - Summary of implementation
        - Iteration cycles count
```

### Arduino Pipeline flow

The Arduino pipeline mirrors the Standard pipeline but with:

- **Clarification adds**: Board type, FQBN, pin mapping, libraries
- **Implementation adds**: `.ino` sketch creation, `arduino-cli compile`, `arduino-cli upload`
- **Audit adds**: RAM/Flash usage estimation, pin conflict detection, timing analysis
- **Documentation adds**: Pinout table, connection diagram, `arduino-cli` commands
- **Publishing excludes**: `build/`, `.hex`, `.elf` binary files

### LOOP Pipeline flow

```
EXPLORADOR-LOOP (pro) receives OBJETIVO + META + CAPITAL
  │
  ├─ Phase 0: SETUP
  │     mkdir -p $PROYECTO_DIR
  │     python3 -m venv $PROYECTO_DIR/venv
  │     Create resultados.sqlite with tables:
  │       - iterations (id, version, prompt, profit_pct, trades, etc.)
  │       - trades_v{N} (timestamp, action, price, quantity, balances, etc.)
  │
  ├─ Phase 1: LOOP (N = 1 to 10)
  │   │
  │   ├─ 1a. Generate prompt — First iteration: convert goal to technical spec
  │   │                       Later iterations: analyze SQLite results, refine strategy
  │   │
  │   ├─ 1b. Create sub-project & invoke ORCHESTRATOR-LOOP
  │   │     mkdir -p $PROYECTO_DIR/v{N}
  │   │     task("orquestador-loop", "SKIP_CLARIFICATION=true ...")
  │   │     Orchestrator-loop runs Standard pipeline in delegated mode
  │   │     → Generates backtest.py + full code in v{N}/
  │   │
  │   ├─ 1c. Wait for orchestrator-loop to finish
  │   │
  │   ├─ 1d. Execute backtest
  │   │     cd $PROYECTO_DIR/v{N}
  │   │     source ../venv/bin/activate
  │   │     pip install -r requirements.txt
  │   │     python3 backtest.py
  │   │
  │   ├─ 1e. Evaluate results
  │   │     Calculate monthly profit from 6-month backtest data
  │   │     Compare with META target
  │   │     If meets_goal → SUCCESS, break loop
  │   │
  │   └─ 1f. Diagnose & improve
  │         Analyze losing trades, adjust parameters
  │         Vary: interval, buy/sell thresholds, stop-loss, take-profit
  │
  └─ Phase 2: FINAL REPORT
        Table comparing all iterations
        Best strategy highlighted
        SQLite database preserved for further analysis
```

---

## State Management

### Data structures

**`pipeline_state.json` structure:**

```json
{
  "pipelines": {
    "<project_name>": {
      "current_step": "orquestador",
      "steps": {
        "orquestador": {
          "status": "pending|in_progress|completed|error",
          "iterations": 0,
          "comment": "",
          "last_update": "2026-07-01T12:00:00"
        },
        "...": {}
      },
      "overall_progress": 0,
      "completed": false,
      "created_at": "2026-07-01T12:00:00",
      "updated_at": "2026-07-01T12:00:00"
    }
  }
}
```

**`status.json` structure** (per session, in `proyectos/sesiones/<alias>/`):

```json
{
  "alias": "project_alias",
  "estado": "corriendo|lanzando|completado|error|detenido|desconocido",
  "pid": "12345",
  "inicio": "2026-07-01T12:00:00",
  "etapa": "analista",
  "spec": "User's specification text (truncated)",
  "log_preview": "Last 10 lines of output.log",
  "pipeline_progress": 45,
  "pipeline_completed": false,
  "current_step": "implementador",
  "steps": { "... same as pipeline_state.json ..." },
  "updated_at": "2026-07-01T12:00:00"
}
```

### Thread safety

The state manager is designed for **multi-process** access (multiple OpenCode sessions can run pipelines simultaneously). It uses:

1. **`fcntl.flock()` with `LOCK_EX`** (exclusive lock):
   - All read-modify-write operations acquire the lock before reading
   - This prevents TOCTOU (Time-of-check to time-of-use) race conditions
   - Linux/Unix only — `fcntl` is POSIX, not available on Windows

2. **Lock file pattern**:
   - A separate `.lock` file is used (not the data file itself)
   - Lock is released in a `finally` block to prevent deadlocks

3. **Context manager (`_locked_access`)**:
   - Acquires lock, reads data, yields to caller
   - Caller modifies data
   - `_write_state()` is called inside the context
   - Lock is always released

4. **Caching layer** (`get_all_sessions`):
   - Caches results for 1 second (configurable)
   - Invalidates cache if `pipeline_state.json` modification time changes
   - Prevents excessive file I/O from SSE polling

### Progress calculation

The `_recalc_progress()` function computes overall pipeline progress:

```python
def _recalc_progress(pdata):
    total = 0
    for step in STEPS_ORDER:
        s = steps.get(step, {})
        st = s.get("status", "pending")
        w = STEP_WEIGHTS.get(step, 10)
        if st == "completed":
            total += w           # Full weight
        elif st == "in_progress":
            total += w * 0.5     # Half weight for in-progress
    # Bonus for iteration effort (max 15%)
    bonus = steps.get("implementador", {}).get("iterations", 0) * 2
    return min(100, int(total + min(bonus, 15)))
```

**Example progress calculation:**

| Step | Status | Weight | Contribution |
|------|--------|--------|-------------|
| orquestador | completed | 10 | +10 |
| analista | completed | 15 | +15 |
| implementador | in_progress | 30 | +15 (50%) |
| auditor | pending | 15 | +0 |
| documentador | pending | 15 | +0 |
| publicador | pending | 15 | +0 |
| **Subtotal** | | | **40%** |
| Bonus (3 iterations) | | | +6 |
| **Total** | | | **46%** |

---

## How to Extend

### Adding a new agent

1. Create `agents/<new-agent>.md`:

```markdown
---
description: >
  Description of the new agent's role.
mode: all
model: opencode-go/deepseek-v4-flash
permission:
  "*": allow
---

# New Agent Name

Eres el **nuevo agente**. Your mission is to...

## Workflow

1. Step 1
2. Step 2

## Fundamental rules
- Rule 1
- Rule 2
```

2. Register in `opencode.jsonc` under `agent`:

```json
"agent": {
  "... existing agents ...": {},
  "new-agent": {
    "description": "Description",
    "mode": "all",
    "permission": { "*": "allow" }
  }
}
```

3. Update `MANIFIESTO_COMPLETO.md` to document the new agent.

4. If the new agent is part of a pipeline, update the orchestrator agent to invoke it at the correct step.

### Creating a new pipeline

1. Define the pipeline architecture:
   - Which agents are involved?
   - What is the sequence?
   - What models should each agent use?

2. Create agent files following the naming convention `agents/<name>-<pipeline_suffix>.md`

3. Create a new orchestrator agent for the pipeline (e.g., `orquestador-new.md`)

4. If the pipeline needs steps beyond the standard 6, update `STEPS_ORDER` in `pipeline_state.py`

5. Update documentation:
   - `MANIFIESTO_COMPLETO.md` — Add pipeline section
   - `README.md` — Add to pipelines table
   - `CHANGELOG.md` — Record the addition

### Adding a new dashboard feature

1. **New API endpoint**: Add a route in `app.py`:

```python
@app.route("/api/new-feature")
def api_new_feature():
    # Use pipeline_state functions to get data
    return jsonify({"result": "data"})
```

2. **New dashboard UI element**: Modify `templates/index.html` or `templates/_session_card.html`

3. **New CSS animation**: Add to `static/css/style.css`

4. **New JavaScript behavior**: Add to `static/js/dashboard.js`

5. **New state function**: Add to `pipeline_state.py` — remember to wrap with `_locked_access()` for thread safety

---

## API Reference

### Dashboard REST API

| Endpoint | Method | Parameters | Response | Description |
|----------|--------|------------|----------|-------------|
| `/` | GET | — | HTML | Main dashboard UI |
| `/api/state` | GET | — | `{"sessions": [...]}` | All session states as JSON |
| `/stream` | GET | — | `text/event-stream` | SSE stream, 2s interval |
| `/health` | GET | — | `{"status":"ok","sessions":N}` | Health check |

### pipeline_state.py API

```python
# === Constants ===
STEPS_ORDER: list[str]          # ["orquestador", "analista", ...]
STEP_WEIGHTS: dict[str, int]    # {"orquestador": 10, ...}
STEP_LABELS: dict[str, str]     # {"orquestador": "Registro y Clarificacion", ...}
STEP_ICONS: dict[str, str]      # {"orquestador": "bi-puzzle", ...}
MAX_ITERATIONS: int             # 7

# === Core functions ===
def register_pipeline(name: str) -> None
def update_step(pipeline: str, step: str, status: str = None,
                comment: str = None, inc_iterations: bool = False) -> None
def advance_step(pipeline: str) -> None
def complete_last_step(pipeline: str) -> None
def set_current_step(pipeline: str, target_step: str) -> None
def check_iteration_limit(name: str) -> tuple[bool, int, int]
def get_pipeline_summary(name: str) -> dict | None
def get_all_sessions() -> dict[str, dict]
def load() -> dict                # Raw state read with lock
def save(data: dict) -> None      # Raw state write with lock
```

### pipeline_status.py CLI API

```
Usage: pipeline_status.py <project> <action> [comment]

Actions:
  start              Register and start a new pipeline
  complete           Mark current step as completed
  advance            Move to the next step
  error              Mark current step as errored
  comment            Add a comment to the current step
  inc_iterations     Increment the iteration counter
  set_step <step>    Jump to a specific step
  check_iterations   Check if iteration limit has been reached
  state              Show full pipeline state summary
```

---

## Codebase Map

```
opencode-pipelines-export/
│
├── agents/                              [Markdown agent definitions — runtime interpreted]
│   ├── orquestador.md                   Standard orchestrator — 280 lines, full pipeline flow
│   ├── analista.md                      Standard analyst — creates PLAN.md
│   ├── implementador.md                 Standard implementer — creates Python code
│   ├── auditor.md                       Standard auditor — creates AUDIT_REPORT.md
│   ├── documentador.md                  Standard documenter — creates README + docs/
│   ├── publicador.md                    Standard publisher — git push to GitHub
│   ├── orquestador-ar.md                Arduino orchestrator
│   ├── analista-ar.md                   Arduino analyst (pin mapping, FQBN)
│   ├── implementador-ar.md              Arduino implementer (arduino-cli)
│   ├── auditor-ar.md                    Arduino auditor (RAM/Flash, timing)
│   ├── documentador-ar.md               Arduino documenter (pinout table)
│   ├── publicador-ar.md                 Arduino publisher
│   ├── explorador-loop.md               LOOP meta-agent — 240 lines, SQLite evaluation
│   ├── orquestador-loop.md              LOOP orchestrator (delegated mode)
│   ├── analista-loop.md                 LOOP analyst
│   ├── implementador-loop.md            LOOP implementer
│   ├── auditor-loop.md                  LOOP auditor
│   ├── documentador-loop.md             LOOP documenter
│   └── publicador-loop.md               LOOP publisher
│
├── dashboard/                           [Python application — Flask web server]
│   ├── __init__.py                      Package marker — 1 line
│   ├── app.py                           Flask entry point — 92 lines
│   │   ├── _format_session_for_template()  Transforms session data for templates
│   │   ├── index()                      GET / — renders HTML
│   │   ├── api_state()                  GET /api/state — JSON endpoint
│   │   ├── stream()                     GET /stream — SSE with 2s interval
│   │   └── health()                     GET /health — health check
│   ├── pipeline_state.py                State management — 373 lines
│   │   ├── _default_state()             Returns empty step dictionary
│   │   ├── _lock_fd() / _unlock_fd()    fcntl.flock wrappers
│   │   ├── _locked_access()             Context manager for thread-safe reads
│   │   ├── _read_raw() / _write_state() JSON file I/O
│   │   ├── _write_sesion_status()       Syncs to proyectos/sesiones/<name>/status.json
│   │   ├── _recalc_progress()           Weighted progress calculation
│   │   ├── _migrate_pipeline_steps()    Ensures all steps exist in old pipelines
│   │   ├── register_pipeline()          Creates new pipeline entry
│   │   ├── update_step()                Updates a single step
│   │   ├── advance_step()               Moves to next step
│   │   ├── complete_last_step()         Atomic 100% completion
│   │   ├── set_current_step()           Rewinds/jumps current step
│   │   ├── check_iteration_limit()      Returns exceeded/current/max
│   │   ├── get_pipeline_summary()       Structured state summary
│   │   └── get_all_sessions()           Merges sessions from dirs + state file (cached)
│   ├── pipeline_status.py               Agent CLI — 93 lines
│   │   └── if __name__ == "__main__":   Command dispatcher (start/complete/advance/...)
│   ├── templates/
│   │   ├── index.html                   Main page — 78 lines, Jinja2 + Bootstrap 5
│   │   └── _session_card.html           Session card — 89 lines, status dot, progress, steps
│   └── static/
│       ├── css/style.css                Glassmorphism dark theme — 525 lines
│       │   ├── Session cards with glassmorphism
│       │   ├── Progress bars with gleam animation
│       │   ├── Step timeline with connectors
│       │   ├── Status dots (pulse/static)
│       │   ├── Log console styling
│       │   └── Responsive breakpoints
│       └── js/dashboard.js              SSE client — 160 lines
│           ├── connectSSE()             EventSource with auto-reconnect
│           ├── updateDashboard()        Re-renders all session cards
│           ├── renderSession()          Builds session card HTML dynamically
│           └── renderStep()             Builds individual step HTML
│
├── skills/                              [OpenCode skills — loadable at runtime]
│   └── cashdro-integracion/
│       └── SKILL.md                     CashDro REST API skill — 889 lines
│
├── opencode.jsonc                       [Configuration file]
│   └── JSONC format with $schema, default_agent, plugin, skills.paths, agent definitions
│
├── setup_pipeline.sh                    [Installation script — 256 lines]
│   ├── PASO 0: OS detection & system deps
│   ├── PASO 1: OpenCode CLI installation
│   ├── PASO 2: OpenCode initialization
│   ├── PASO 3: Agent configuration write
│   ├── PASO 4: GitHub authentication
│   ├── PASO 5: Workspace structure
│   ├── PASO 6: Git configuration
│   ├── PASO 7: Verification
│   └── PASO 8: Final summary
│
├── pipeline_state.json                  [Data file — JSON]
│   └── {"pipelines": {"<name>": {"current_step": ..., "steps": {...}, ...}}}
│
├── pipeline_state.json.lock             [Lock file — ensures single-writer]
│
├── requirements.txt                     [Python dependencies]
│   └── flask>=3.0.0
│
├── docs/                                [Generated documentation]
│   ├── datasheet.md                     User-facing installation & usage guide
│   └── developer.md                     This file — source code explanation
│
├── MANIFIESTO_COMPLETO.md               [Ecosystem manifest — 305 lines]
├── CHANGELOG.md                         [Changelog — 141 lines]
├── VISION_CREADOR_CURSOS_TECNICOS.md    [Future vision — 105 lines]
├── plan_social_agents.md                [Social media publishing plan — 63 lines]
│
├── .gitignore                           [Git exclusions — 36 lines]
└── README.md                            [Main documentation]
```
