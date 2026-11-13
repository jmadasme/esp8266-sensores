# CHANGELOG — Pipeline Multi-Agente + Dashboard Unificado

## 1. Suscripción Go para todos los agentes

**Archivo modificado:** `/home/jma/.config/opencode/opencode.jsonc`
**Archivos modificados:** `/home/jma/.config/opencode/agents/*.md`

Todos los 7 agentes (`analista`, `implementador`, `auditor`, `documentador`, `publicador`, `orquestrador`, `dashboard`) ahora usan el modelo `opencode-go/deepseek-v4-flash` (suscripción Go) en lugar de `opencode/big-pickle`.

---

## 2. Dashboard unificado como Command Center

**URL:** `http://metagentes:5050` | `http://localhost:5050`

### `dashboard/app.py`
- Ahora lee de **dos fuentes**: `~/sesiones/<alias>/status.json` (estado de cada sesión opencode) y `pipeline_state.json` (progreso de pipelines)
- Endpoints: `/` (HTML), `/api/state` (JSON), `/stream` (SSE en tiempo real), `/health`
- SSE envía actualizaciones cada 2 segundos al navegador

### `dashboard/pipeline_state.py`
- Nueva función `get_all_sessions()`: lee `~/sesiones/<alias>/status.json` de cada sesión activa
- `_write_sesion_status()`: cada vez que se actualiza un pipeline, sincroniza los datos a `~/sesiones/<nombre>/status.json`
- Esto asegura que **ambos sistemas** (el dashboard y el orquestrador) vean los mismos datos

### `dashboard/templates/_session_card.html`
- Nueva tarjeta por sesión opencode con:
  - Nombre de sesión + dot de estado animado (running: amarillo pulsante, completed: verde, error: rojo)
  - Status tag (corriendo, lanzando, completado, error)
  - PID badge
  - Fecha de inicio / última actualización
  - Barra de progreso con gradiente + efecto gleam
  - Paso actual destacado
  - Spec del proyecto (truncada a 120 chars)
  - Los 5 steps del pipeline: Análisis, Implementación, Auditoría, Documentación, Publicación
  - Cada step muestra: icono, estado (Hecho/Ejecutando/Error/Pendiente), iteraciones, comentarios
  - Log preview (últimas 10 líneas) en caja tipo terminal

### `dashboard/templates/index.html`
- Header: "OpenCode Command Center" con icono CPU
- 3 contadores: sesiones totales, activas, completadas
- Footer con branding

### `dashboard/static/js/dashboard.js`
- Conexión SSE con reconexión automática cada 3 segundos si se pierde
- Renderizado dinámico de tarjetas de sesión
- Efectos visuales: just-created (slide-in), just-updated (flash-pulse)

### `dashboard/static/css/style.css`
- Diseño glassmorphism oscuro con fondo degradado `#0f0c29 → #1a1a3e → #24243e`
- Animaciones premium: shimmer en bordes y barras, pulse en dots activos, glow en step activo, breathe en iconos, shake en errores
- Scrollbar personalizada
- Responsive para móvil

---

## 3. Flujo multi-sesión interactivo

### `agents/orquestrador.md`
El orquestrador ahora sigue este flujo para CADA `@analista`:

```
Fase A: CLARIFICACIÓN INTERACTIVA
  └─ Presenta preguntas al usuario (funcionalidad, hardware, librerías, etc.)

Fase B: LANZAMIENTO BACKGROUND
  └─ Guarda spec + Q&A en ~/sesiones/<alias>/
  └─ Ejecuta: opencode run --agent analista --dir ~/sesiones/<alias>/ "..."
  └─ Muestra confirmación + dashboard URL

Fase C: SIGUIENTE ANALISTA
  └─ Repite Fase A-B para el siguiente @analista
  └─ El usuario responde preguntas del #2 mientras pipeline #1 ya corre

Fase D: PANTALLA FINAL
  └─ Muestra resumen con todas las sesiones lanzadas
```

### `agents/analista.md`
Se añadió el comando PSTATUS para reportar al dashboard en cada etapa:
- `$PSTATUS <proyecto> start 'mensaje'` → inicia etapa
- `$PSTATUS <proyecto> complete 'mensaje'` → completa etapa
- `$PSTATUS <proyecto> advance` → avanza al siguiente step
- `$PSTATUS <proyecto> inc_iterations 'mensaje'` → incrementa iteraciones

---

## 4. Arquitectura general

```
Usuario → [orquestrador (sesión principal)]
                │
                ├─ @analista 1 → preguntas → respuestas
                │     └─ opencode run --agent analista (background)
                │           └─ pipeline_state.json ←→ ~/sesiones/alias1/status.json
                │
                ├─ @analista 2 → preguntas → respuestas
                │     └─ opencode run --agent analista (background)
                │           └─ pipeline_state.json ←→ ~/sesiones/alias2/status.json
                │
                └─ Dashboard Web http://metagentes:5050
                      └─ Lee de ~/sesiones/*/status.json + pipeline_state.json
                      └─ SSE en vivo cada 2 segundos
```

---

## 5. Cómo iniciar el dashboard

```bash
# El dashboard ya está corriendo en puerto 5050.
# Si se detiene, reiniciar con:
cd /home/jma/Documents/Proyects/OpenCode/AG.CreadorContenido
python3 dashboard/app.py 5050 &
```

## 6. Cómo usar el orquestrador

En una sesión de opencode, invoca al agente `orquestrador` con:

```
@analista poblador, "Crear un programa Python que..."
@analista kiosco, "Crear una página web que..."
```

El orquestrador te hará preguntas de clarificación para cada uno, y lanzará pipelines independientes en background.

---

## 7. Exportación completa del ecosistema de pipelines (2026-07-01)

**Nuevo proyecto:** `opencode-pipelines-export`

Se ha creado un paquete de exportación auto-contenido que incluye:
- **19 agentes** de los 3 pipelines (Standard, Arduino, LOOP)
- **Dashboard** web completo (Flask, SSE, pipeline_state, pipeline_status)
- **Configuración** OpenCode (opencode.jsonc)
- **Skill** CashDro integración
- **Script de instalación** (`setup_pipeline.sh`) sanitizado
- **Documentación** completa (README, MANIFIESTO, CHANGELOG, VISION)
- Publicado en GitHub como respaldo y mecanismo de transferencia.
