"""Pipeline state manager - shared state for pipeline monitoring.
Unified: writes to pipeline_state.json AND proyectos/sesiones/<name>/status.json.
Cross-process safe via fcntl.flock."""
import json, os, time, fcntl, logging, sys
from contextlib import contextmanager
from pathlib import Path

_BASE_OVERRIDE = os.environ.get("OPF_BASE")
BASE = Path(_BASE_OVERRIDE) if _BASE_OVERRIDE else Path(__file__).resolve().parent.parent
STATE_FILE = BASE / "pipeline_state.json"
LOCK_FILE = STATE_FILE.parent / (STATE_FILE.name + '.lock')
PROYECTOS_DIR = BASE / "proyectos"
SESIONES_DIR = PROYECTOS_DIR / "sesiones"

logger = logging.getLogger("pipeline_state")
if not logger.handlers:
    _handler = logging.StreamHandler(sys.stderr)
    _handler.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] %(name)s: %(message)s"))
    logger.addHandler(_handler)
    logger.setLevel(logging.WARNING)

STEPS_ORDER = ["orquestador", "analista", "implementador", "auditor", "documentador", "publicador"]
STEP_WEIGHTS = {"orquestador": 10, "analista": 15, "implementador": 30, "auditor": 15, "documentador": 15, "publicador": 15}
STEP_LABELS = {
    "orquestador": "Registro y Clarificación",
    "analista": "Análisis",
    "implementador": "Implementación",
    "auditor": "Auditoría",
    "documentador": "Documentación",
    "publicador": "Publicación"
}
STEP_ICONS = {
    "orquestador": "bi-puzzle",
    "analista": "bi-clipboard2-pulse",
    "implementador": "bi-code-slash",
    "auditor": "bi-shield-check",
    "documentador": "bi-file-earmark-text",
    "publicador": "bi-cloud-upload"
}

MAX_ITERATIONS = 7

def _default_state():
    return {
        "orquestador": {"status": "pending", "iterations": 0, "comment": "", "last_update": ""},
        "analista": {"status": "pending", "iterations": 0, "comment": "", "last_update": ""},
        "implementador": {"status": "pending", "iterations": 0, "comment": "", "last_update": ""},
        "auditor": {"status": "pending", "iterations": 0, "comment": "", "last_update": ""},
        "documentador": {"status": "pending", "iterations": 0, "comment": "", "last_update": ""},
        "publicador": {"status": "pending", "iterations": 0, "comment": "", "last_update": ""}
    }

def _lock_fd():
    LOCK_FILE.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(str(LOCK_FILE), os.O_CREAT | os.O_RDWR, 0o644)
    fcntl.flock(fd, fcntl.LOCK_EX)
    return fd

def _unlock_fd(fd):
    try:
        fcntl.flock(fd, fcntl.LOCK_UN)
    finally:
        os.close(fd)

def _migrate_pipeline_steps(pdata):
    """Asegura que todos los pasos de STEPS_ORDER existen en el pipeline."""
    steps = pdata.setdefault("steps", {})
    for step in STEPS_ORDER:
        if step not in steps:
            steps[step] = {"status": "pending", "iterations": 0, "comment": "", "last_update": ""}
    # Asegurar current_step
    if "current_step" not in pdata or pdata["current_step"] not in STEPS_ORDER:
        pdata["current_step"] = "orquestador"
    return pdata

def _read_raw():
    if STATE_FILE.exists():
        try:
            data = json.loads(STATE_FILE.read_text())
            # Migrar pipelines viejos
            for pname, pdata in data.get("pipelines", {}).items():
                _migrate_pipeline_steps(pdata)
            return data
        except (json.JSONDecodeError, OSError) as e:
            logger.warning("Error leyendo state file: %s", e)
            return {"pipelines": {}}
    return {"pipelines": {}}

@contextmanager
def _locked_access():
    fd = _lock_fd()
    try:
        yield _read_raw()
    finally:
        _unlock_fd(fd)

def _write_state(data):
    STATE_FILE.write_text(json.dumps(data, ensure_ascii=False, separators=(',', ':')))

def load():
    fd = _lock_fd()
    try:
        return _read_raw()
    finally:
        _unlock_fd(fd)

def save(data):
    fd = _lock_fd()
    try:
        _write_state(data)
    finally:
        _unlock_fd(fd)

def _write_sesion_status(name, data):
    """Sync to proyectos/sesiones/<name>/status.json"""
    ses_dir = SESIONES_DIR / name
    if not ses_dir.exists():
        return
    ses_status = ses_dir / "status.json"
    try:
        merged = {"alias": name, "estado": "corriendo"}
        if ses_status.exists():
            try:
                existing = json.loads(ses_status.read_text())
                merged.update(existing)
            except (json.JSONDecodeError, OSError):
                pass
        merged["pipeline_progress"] = data.get("overall_progress", 0)
        merged["pipeline_completed"] = data.get("completed", False)
        merged["current_step"] = data.get("current_step", "orquestador")
        merged["steps"] = data.get("steps", {})
        merged["updated_at"] = time.strftime("%Y-%m-%dT%H:%M:%S")
        ses_status.write_text(json.dumps(merged, indent=2, ensure_ascii=False))
    except (OSError, json.JSONDecodeError) as e:
        logger.warning("Error escribiendo status.json para '%s': %s", name, e)

def register_pipeline(name):
    with _locked_access() as data:
        if name not in data["pipelines"]:
            data["pipelines"][name] = {
                "current_step": "orquestador",
                "steps": _default_state(),
                "overall_progress": 0,
                "completed": False,
                "created_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "updated_at": time.strftime("%Y-%m-%dT%H:%M:%S")
            }
            _write_state(data)
        pdata = dict(data["pipelines"][name])
    _write_sesion_status(name, pdata)

def update_step(pipeline, step, status=None, comment=None, inc_iterations=False):
    with _locked_access() as data:
        if pipeline not in data["pipelines"]:
            data["pipelines"][pipeline] = {
                "current_step": "orquestador",
                "steps": _default_state(),
                "overall_progress": 0,
                "completed": False,
                "created_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "updated_at": time.strftime("%Y-%m-%dT%H:%M:%S")
            }
        steps = data["pipelines"][pipeline]["steps"]
        if step not in steps:
            steps[step] = {"status": "pending", "iterations": 0, "comment": "", "last_update": ""}
        if status:
            steps[step]["status"] = status
        if comment:
            steps[step]["comment"] = comment
        if inc_iterations:
            steps[step]["iterations"] += 1
        steps[step]["last_update"] = time.strftime("%Y-%m-%dT%H:%M:%S")
        _recalc_progress(data["pipelines"][pipeline])
        data["pipelines"][pipeline]["updated_at"] = time.strftime("%Y-%m-%dT%H:%M:%S")
        pdata = dict(data["pipelines"][pipeline])
        _write_state(data)
    _write_sesion_status(pipeline, pdata)

def advance_step(pipeline):
    with _locked_access() as data:
        if pipeline not in data["pipelines"]:
            return
        pdata = data["pipelines"][pipeline]
        current = pdata["current_step"]
        current_idx = STEPS_ORDER.index(current) if current in STEPS_ORDER else -1
        if current_idx < len(STEPS_ORDER) - 1:
            pdata["steps"][current]["status"] = "completed"
            next_step = STEPS_ORDER[current_idx + 1]
            pdata["current_step"] = next_step
            pdata["steps"][next_step]["status"] = "in_progress"
            _recalc_progress(pdata)
        else:
            pdata["steps"][current]["status"] = "completed"
            pdata["completed"] = True
            pdata["overall_progress"] = 100
        pdata["updated_at"] = time.strftime("%Y-%m-%dT%H:%M:%S")
        pdata_copy = dict(pdata)
        _write_state(data)
    _write_sesion_status(pipeline, pdata_copy)

def complete_last_step(pipeline):
    """Atomic complete-last-step: marks the final step done + 100% progress."""
    with _locked_access() as data:
        if pipeline not in data["pipelines"]:
            return
        pdata = data["pipelines"][pipeline]
        current = pdata["current_step"]
        if current == STEPS_ORDER[-1]:
            pdata["steps"][current]["status"] = "completed"
            pdata["completed"] = True
            pdata["overall_progress"] = 100
            pdata["updated_at"] = time.strftime("%Y-%m-%dT%H:%M:%S")
            _write_state(data)
            _write_sesion_status(pipeline, dict(pdata))

def set_current_step(pipeline, target_step):
    """Rewind or jump current_step to target_step (for iteration loops).
    Resets all downstream steps to pending."""
    if target_step not in STEPS_ORDER:
        logger.warning("set_current_step: paso '%s' no valido. Pasos: %s", target_step, STEPS_ORDER)
        return
    pdata_copy = {}
    with _locked_access() as data:
        if pipeline not in data["pipelines"]:
            logger.warning("set_current_step: pipeline '%s' no encontrado", pipeline)
            return
        pdata = data["pipelines"][pipeline]
        target_idx = STEPS_ORDER.index(target_step)
        for i, step in enumerate(STEPS_ORDER):
            if i > target_idx:
                prev_its = pdata["steps"].get(step, {}).get("iterations", 0)
                pdata["steps"][step] = {
                    "status": "pending",
                    "iterations": prev_its,
                    "comment": "",
                    "last_update": ""
                }
        pdata["current_step"] = target_step
        pdata["steps"][target_step]["status"] = "in_progress"
        _recalc_progress(pdata)
        pdata["updated_at"] = time.strftime("%Y-%m-%dT%H:%M:%S")
        pdata_copy = dict(pdata)
        _write_state(data)
    _write_sesion_status(pipeline, pdata_copy)

def _recalc_progress(pdata):
    total = 0
    steps = pdata["steps"]
    for step in STEPS_ORDER:
        s = steps.get(step, {})
        st = s.get("status", "pending")
        w = STEP_WEIGHTS.get(step, 10)
        if st == "completed":
            total += w
        elif st == "in_progress":
            total += w * 0.5
    bonus = steps.get("implementador", {}).get("iterations", 0) * 2
    pdata["overall_progress"] = min(100, int(total + min(bonus, 15)))

def check_iteration_limit(name):
    """Retorna (excedido, actual, max) para que el orquestador decida."""
    with _locked_access() as data:
        pdata = data.get("pipelines", {}).get(name, {})
        if not pdata:
            return False, 0, MAX_ITERATIONS
        its = pdata.get("steps", {}).get("implementador", {}).get("iterations", 0)
        return its >= MAX_ITERATIONS, its, MAX_ITERATIONS

def get_pipeline_summary(name):
    """Returns a resume-friendly summary for the orquestador."""
    with _locked_access() as data:
        pdata = data.get("pipelines", {}).get(name, {})
        if not pdata:
            return None
        current = pdata.get("current_step", "orquestador")
        steps = pdata.get("steps", {})
        completed = [s for s in STEPS_ORDER if steps.get(s, {}).get("status") == "completed"]
        errored = [s for s in STEPS_ORDER if steps.get(s, {}).get("status") == "error"]
        return {
            "current_step": current,
            "completed_steps": completed,
            "errored_steps": errored,
            "overall_progress": pdata.get("overall_progress", 0),
            "completed": pdata.get("completed", False),
            "iterations": steps.get("implementador", {}).get("iterations", 0),
        }

_sessions_cache = {"data": {}, "timestamp": 0, "state_mtime": 0}

def get_all_sessions():
    """Returns merged data from ~/sesiones/ and pipeline_state.json"""
    now = time.time()
    try:
        st_mtime = os.path.getmtime(STATE_FILE)
    except OSError:
        st_mtime = 0
    if _sessions_cache["data"] and (now - _sessions_cache["timestamp"]) < 1.0 and st_mtime == _sessions_cache["state_mtime"]:
        return _sessions_cache["data"]

    sessions = {}

    # 1) Leer sesiones desde directorios (si existen)
    if SESIONES_DIR.exists():
        for d in sorted(SESIONES_DIR.iterdir()):
            if d.is_dir():
                status_file = d / "status.json"
                log_file = d / "output.log"
                spec_file = d / "spec.txt"
                alias = d.name
                session = {
                    "alias": alias,
                    "estado": "desconocido",
                    "pid": "",
                    "inicio": "",
                    "etapa": "",
                    "spec": "",
                    "log_preview": "",
                    "pipeline_progress": 0,
                    "pipeline_completed": False,
                    "current_step": "",
                    "steps": {},
                }
                if status_file.exists():
                    try:
                        sdata = json.loads(status_file.read_text())
                        session.update(sdata)
                    except (json.JSONDecodeError, OSError):
                        pass
                if spec_file.exists():
                    session["spec"] = spec_file.read_text().strip()
                if log_file.exists():
                    lines = log_file.read_text().strip().split("\n")
                    session["log_preview"] = "\n".join(lines[-10:]) if lines else ""
                if not session.get("current_step") and not session.get("pipeline_progress"):
                    with _locked_access() as pipe_data_raw:
                        pipe_data = pipe_data_raw.get("pipelines", {}).get(alias, {})
                    if pipe_data:
                        session["pipeline_progress"] = pipe_data.get("overall_progress", 0)
                        session["pipeline_completed"] = pipe_data.get("completed", False)
                        session["current_step"] = pipe_data.get("current_step", "")
                        session["steps"] = pipe_data.get("steps", {})
                sessions[alias] = session

    # 2) Sincronizar pipelines desde pipeline_state.json (si no están ya en sessions)
    with _locked_access() as pipe_data_raw:
        all_pipelines = pipe_data_raw.get("pipelines", {})
    for alias, pdata in all_pipelines.items():
        if alias not in sessions:
            sessions[alias] = {
                "alias": alias,
                "estado": "completado" if pdata.get("completed") else "corriendo",
                "pid": "",
                "inicio": pdata.get("created_at", ""),
                "etapa": pdata.get("current_step", ""),
                "spec": "",
                "log_preview": "",
                "pipeline_progress": pdata.get("overall_progress", 0),
                "pipeline_completed": pdata.get("completed", False),
                "current_step": pdata.get("current_step", ""),
                "steps": pdata.get("steps", {}),
                "updated_at": pdata.get("updated_at", ""),
            }
        elif not sessions[alias].get("pipeline_progress"):
            # si el session de directorio no tiene progreso, lo toma del state
            sessions[alias]["pipeline_progress"] = pdata.get("overall_progress", 0)
            sessions[alias]["pipeline_completed"] = pdata.get("completed", False)
            sessions[alias]["current_step"] = pdata.get("current_step", "")
            sessions[alias]["steps"] = pdata.get("steps", {})

    _sessions_cache["data"] = sessions
    _sessions_cache["timestamp"] = now
    _sessions_cache["state_mtime"] = st_mtime
    return sessions
