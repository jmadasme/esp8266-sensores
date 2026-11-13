"""Dashboard web - Unified command center for all opencode sessions."""
import sys, json, time, os
from flask import Flask, render_template, Response, jsonify

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from dashboard.pipeline_state import STEPS_ORDER, STEP_LABELS, STEP_ICONS, get_all_sessions

app = Flask(__name__)
app.config["SECRET_KEY"] = "pipeline-dashboard-opencode"

def _format_session_for_template(sessions):
    items = []
    for alias, ses in sessions.items():
        steps_list = []
        steps_data = ses.get("steps", {})
        if not steps_data:
            for step in STEPS_ORDER:
                steps_list.append({
                    "key": step,
                    "label": STEP_LABELS.get(step, step),
                    "icon": STEP_ICONS.get(step, "bi-circle"),
                    "status": "pending",
                    "iterations": 0,
                    "comment": "",
                    "last_update": "",
                    "active": False,
                })
        else:
            current_step = ses.get("current_step", "")
            for step in STEPS_ORDER:
                s = steps_data.get(step, {})
                steps_list.append({
                    "key": step,
                    "label": STEP_LABELS.get(step, step),
                    "icon": STEP_ICONS.get(step, "bi-circle"),
                    "status": s.get("status", "pending"),
                    "iterations": s.get("iterations", 0),
                    "comment": s.get("comment", ""),
                    "last_update": s.get("last_update", ""),
                    "active": step == current_step,
                })
        items.append({
            "alias": alias,
            "estado": ses.get("estado", "desconocido"),
            "pid": ses.get("pid", ""),
            "inicio": ses.get("inicio", ""),
            "etapa": ses.get("etapa", ""),
            "spec": ses.get("spec", ""),
            "log_preview": ses.get("log_preview", ""),
            "progress": ses.get("pipeline_progress", 0),
            "completed": ses.get("pipeline_completed", False),
            "current_step": ses.get("current_step", ""),
            "steps": steps_list,
            "updated_at": ses.get("updated_at", ""),
        })
    return {"sessions": items}

@app.route("/")
def index():
    sessions = get_all_sessions()
    return render_template("index.html", **_format_session_for_template(sessions))

@app.route("/api/state")
def api_state():
    return jsonify(_format_session_for_template(get_all_sessions()))

@app.route("/stream")
def stream():
    def event_stream():
        while True:
            try:
                new_data = _format_session_for_template(get_all_sessions())
                yield f"data: {json.dumps(new_data)}\n\n"
            except Exception:
                yield "data: {}\n\n"
            time.sleep(2)
    return Response(event_stream(), mimetype="text/event-stream",
                    headers={"Cache-Control": "no-cache", "Connection": "keep-alive"})

@app.route("/health")
def health():
    return jsonify({"status": "ok", "sessions": len(get_all_sessions())})

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 5050
    print(f"\n  ╔══════════════════════════════════════════╗")
    print(f"  ║   \033[1;35mOpenCode Command Center\033[0m              ║")
    print(f"  ║                                          ║")
    print(f"  ║  \033[1;36mhttp://metagentes:{port}\033[0m                  ║")
    print(f"  ║  \033[1;36mhttp://localhost:{port}\033[0m              ║")
    print(f"  ╚══════════════════════════════════════════╝")
    app.run(host="0.0.0.0", port=port, debug=False, threaded=True)
