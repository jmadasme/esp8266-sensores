#!/usr/bin/env python3
"""CLI helper for agents to report pipeline status."""
import sys, os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
from dashboard.pipeline_state import (
    register_pipeline, update_step, advance_step, complete_last_step, load, STEPS_ORDER,
    get_pipeline_summary, set_current_step, check_iteration_limit, MAX_ITERATIONS
)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Uso: pipeline_status <proyecto> <accion> [comentario]")
        print("Acciones: start, complete, advance, error, comment, inc_iterations, set_step, check_iterations, state")
        print("Ejemplo: pipeline_status MiProyecto start 'Iniciando analisis'")
        sys.exit(1)

    proyecto = sys.argv[1]
    accion   = sys.argv[2]
    comentario = sys.argv[3] if len(sys.argv) > 3 else ""

    try:
        # Solo 'start' crea pipelines nuevos; el resto requiere que exista
        if accion == "start":
            register_pipeline(proyecto)
        elif accion != "state":
            # Verificar que el pipeline existe antes de operar
            summary = get_pipeline_summary(proyecto)
            if not summary:
                print(f"✗ Pipeline '{proyecto}' no encontrado. Debes usar 'start' primero.")
                sys.exit(1)

        if accion == "state":
            summary = get_pipeline_summary(proyecto)
            if summary:
                print(f"Pipeline: {proyecto}")
                print(f"  Paso actual: {summary['current_step']}")
                print(f"  Completados: {', '.join(summary['completed_steps']) if summary['completed_steps'] else 'ninguno'}")
                print(f"  Errores: {', '.join(summary['errored_steps']) if summary['errored_steps'] else 'ninguno'}")
                print(f"  Progreso: {summary['overall_progress']}%")
                print(f"  Iteraciones implementador: {summary['iterations']}")
                print(f"  Pipeline completado: {summary['completed']}")
            else:
                print(f"Pipeline '{proyecto}' no encontrado. Usa 'start' para crearlo.")
            sys.exit(0)

        data = load()
        pdata = data.get("pipelines", {}).get(proyecto, {})
        current = pdata.get("current_step", "orquestador")

        if accion == "start":
            update_step(proyecto, current, 'in_progress', comentario)
            print(f"✓ {proyecto}/{current}: iniciado")
        elif accion == "complete":
            if current == STEPS_ORDER[-1]:
                complete_last_step(proyecto)
            else:
                update_step(proyecto, current, 'completed', comentario)
            print(f"✓ {proyecto}/{current}: completado")
        elif accion == "advance":
            advance_step(proyecto)
            print(f"✓ {proyecto}: avanzado al siguiente paso")
        elif accion == "error":
            update_step(proyecto, current, 'error', comentario)
            print(f"✗ {proyecto}/{current}: error")
        elif accion == "comment":
            update_step(proyecto, current, None, comentario)
            print(f"→ {proyecto}/{current}: comentario agregado")
        elif accion == "inc_iterations":
            update_step(proyecto, current, None, comentario, inc_iterations=True)
            print(f"→ {proyecto}/{current}: iteracion incrementada")
        elif accion == "set_step":
            if not comentario:
                print(f"? set_step requiere un paso destino: pipeline_status <p> set_step <paso>")
                sys.exit(1)
            if comentario not in STEPS_ORDER:
                print(f"? Paso '{comentario}' no valido. Pasos validos: {', '.join(STEPS_ORDER)}")
                sys.exit(1)
            set_current_step(proyecto, comentario)
            print(f"✓ {proyecto}: current_step = {comentario}")
        elif accion == "check_iterations":
            exceeded, current, maximum = check_iteration_limit(proyecto)
            print(f"Iteraciones implementador: {current}/{maximum}")
            if exceeded:
                print(f"? LIMITE DE {maximum} ITERACIONES ALCANZADO. Requiere decision del usuario.")
            else:
                print(f"? Quedan {maximum - current} iteraciones disponibles.")
        else:
            print(f"? Accion desconocida: {accion}")
            sys.exit(1)
    except Exception as e:
        print(f"✗ ERROR en pipeline_status: {e}")
        sys.exit(2)
