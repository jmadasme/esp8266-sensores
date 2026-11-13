# MANIFIESTO COMPLETO — Ecosistema de Pipelines OpenCode

> Exportado el: 2026-07-01
> Origen: /home/jma/Documents/Proyects/OpenCode/AG.CreadorContenido/

---

## 📋 RESUMEN DEL ECOSISTEMA

Este equipo contiene **3 pipelines completos** con **19 agentes** en total, más **infraestructura de dashboard**, **skills**, y **scripts de instalación**.

```
Total agentes:     19
Total pipelines:    3 (+ 1 meta-pipeline)
Skills:             1
Dashboard:          1 (Web Flask en puerto 5050)
Scripts instalación: 1
Proyectos realizados: 21
```

---

## 🏗️ PIPELINE 1: STANDARD (Python)

**Propósito**: Pipeline base para proyectos Python. Recibe un requerimiento → analiza → implementa → audita → documenta → publica en GitHub.

**Diagrama**:
```
Usuario → ORQUESTADOR
               ├─ ① REGISTRO
               ├─ ② CLARIFICAR (preguntas al usuario)
               ├─ ③ ANALISTA      → crea PLAN.md
               ├─ ④ IMPLEMENTADOR → crea código Python
               ├─ ⑤ AUDITOR       → crea AUDIT_REPORT.md
               ├─ ⑥ DECISIÓN post-auditoría
               │     ├─ APROBADO → continuar
               │     └─ INCIDENCIAS → volver a ④ (máx 7 ciclos)
               ├─ ⑦ DOCUMENTADOR  → crea README + docs/
               ├─ ⑧ PUBLICADOR    → sube a GitHub (cuenta wanderc137)
               └─ ⑨ INFORMAR al usuario
```

### Agentes del Pipeline Standard:

| Agente | Archivo | Modelo | Rol |
|--------|---------|--------|-----|
| **orquestador** | `agents/orquestador.md` | deepseek-v4-flash | Orquestador principal. Único que habla con usuario. Gestiona todo el flujo. |
| **analista** | `agents/analista.md` | deepseek-v4-pro | Analiza requerimiento → crea PLAN.md con especificación funcional, diseño técnico, estructura de archivos, casos de prueba. |
| **implementador** | `agents/implementador.md` | deepseek-v4-flash | Lee PLAN.md → crea código Python completo + venv + dependencias. Verifica archivos. |
| **auditor** | `agents/auditor.md` | deepseek-v4-pro | Análisis estático del código: integridad, seguridad, funcionamiento, estabilidad, rendimiento. Genera AUDIT_REPORT.md. |
| **documentador** | `agents/documentador.md` | deepseek-v4-pro | Crea README.md + docs/datasheet.md + docs/developer.md. Documentación técnica completa. |
| **publicador** | `agents/publicador.md` | deepseek-v4-flash | Inicializa git en PROYECTO_DIR, sube a GitHub (wanderc137). NO sube PLAN.md ni AUDIT_REPORT.md. |

**Flujo de comunicación entre agentes**: Archivos (PLAN.md, AUDIT_REPORT.md, código fuente) + herramienta `task`.

---

## 🏗️ PIPELINE 2: ARDUINO (Embebidos)

**Propósito**: Pipeline especializado para proyectos Arduino. Compila y transfiere firmware a placa física vía arduino-cli.

**Diagrama**:
```
Usuario → ORQUESTADOR-AR
               ├─ ① REGISTRO
               ├─ ② CLARIFICAR (placa, pines, librerías)
               ├─ ③ ANALISTA-AR      → crea PLAN.md (con mapeo de pines)
               ├─ ④ IMPLEMENTADOR-AR → crea .ino + compila + transfiere a placa
               ├─ ⑤ AUDITOR-AR       → crea AUDIT_REPORT.md
               ├─ ⑥ DECISIÓN post-auditoría
               │     ├─ APROBADO → continuar
               │     └─ INCIDENCIAS → volver a ④ (máx 7 ciclos)
               ├─ ⑦ DOCUMENTADOR-AR  → crea README + docs/ (con pinout)
               ├─ ⑧ PUBLICADOR-AR    → sube a GitHub
               └─ ⑨ INFORMAR al usuario
```

### Agentes del Pipeline Arduino:

| Agente | Archivo | Modelo | Rol |
|--------|---------|--------|-----|
| **orquestador-ar** | `agents/orquestador-ar.md` | deepseek-v4-flash | Orquestador Arduino. Gestiona pipeline con compilación y transferencia física. |
| **analista-ar** | `agents/analista-ar.md` | deepseek-v4-pro | Analiza requerimiento Arduino → crea PLAN.md con mapeo de pines, placa objetivo, FQBN, librerías. |
| **implementador-ar** | `agents/implementador-ar.md` | deepseek-v4-flash | Crea sketch .ino, compila con arduino-cli, transfiere a placa física vía USB. Usa alias `ardu`, `ardu-compile`, `ardu-upload`. |
| **auditor-ar** | `agents/auditor-ar.md` | deepseek-v4-pro | Auditoría Arduino: verifica pines, memoria (RAM/Flash), timing, rebotes, cross-compile. Estima uso de recursos. |
| **documentador-ar** | `agents/documentador-ar.md` | deepseek-v4-pro | Documentación Arduino con tabla de pines (pinout), diagrama de conexión, comandos arduino-cli. |
| **publicador-ar** | `agents/publicador-ar.md` | deepseek-v4-flash | Publica proyecto Arduino en GitHub. NO sube binarios compilados (build/, .hex, .elf). |

**Cores Arduino instalados globalmente**: arduino:avr, arduino:renesas_uno, esp8266:esp8266, esp32:esp32, Heltec-esp32:esp32

---

## 🏗️ PIPELINE 3: LOOP (Meta-optimización)

**Propósito**: Pipeline de optimización iterativa. Un meta-agente (explorador-loop) prueba múltiples estrategias invocando al pipeline LOOP completo hasta alcanzar una meta medible.

**Diagrama**:
```
EXPLORADOR-LOOP (meta-agente, opera POR ENCIMA)
       │
       │ Invoca orquestador-loop en cada iteración (máx 10)
       │ Evalúa resultados en SQLite
       │ Refina estrategia entre iteraciones
       ▼
┌─ ORQUESTADOR-LOOP (modo delegado cuando SKIP_CLARIFICATION=true)
│      ├─ ③ ANALISTA-LOOP
│      ├─ ④ IMPLEMENTADOR-LOOP
│      ├─ ⑤ AUDITOR-LOOP
│      ├─ ⑥ DECISIÓN (APROBADO/INCIDENCIAS)
│      ├─ ⑦ DOCUMENTADOR-LOOP
│      └─ ⑧ PUBLICADOR-LOOP
└─
```

### Agentes del Pipeline LOOP:

| Agente | Archivo | Modelo | Rol |
|--------|---------|--------|-----|
| **explorador-loop** | `agents/explorador-loop.md` | deepseek-v4-pro | **Meta-agente**. Recibe objetivo difuso + meta medible. Genera prompts, invoca orquestador-loop, evalúa en SQLite, refina estrategia. Máx 10 iteraciones. |
| **orquestador-loop** | `agents/orquestador-loop.md` | deepseek-v4-flash | Orquestador LOOP. Soporta **modo delegado** (con SKIP_CLARIFICATION=true) cuando es invocado por explorador-loop. |
| **analista-loop** | `agents/analista-loop.md` | deepseek-v4-pro | Igual que analista estándar pero para pipeline LOOP. |
| **implementador-loop** | `agents/implementador-loop.md` | deepseek-v4-flash | Igual que implementador estándar pero para pipeline LOOP. |
| **auditor-loop** | `agents/auditor-loop.md` | deepseek-v4-pro | Igual que auditor estándar pero para pipeline LOOP. |
| **documentador-loop** | `agents/documentador-loop.md` | deepseek-v4-pro | Igual que documentador estándar pero para pipeline LOOP. |
| **publicador-loop** | `agents/publicador-loop.md` | deepseek-v4-flash | Igual que publicador estándar pero para pipeline LOOP. |

---

## 🖥️ INFRAESTRUCTURA COMPARTIDA

### Dashboard (Command Center)

| Componente | Archivo | Descripción |
|------------|---------|-------------|
| Web App | `dashboard/app.py` | Flask web en puerto 5050. Endpoints: `/`, `/api/state`, `/stream` (SSE), `/health` |
| State Manager | `dashboard/pipeline_state.py` | Gestiona estado de pipelines con fcntl.flock (thread-safe). Steps: orquestador→analista→implementador→auditor→documentador→publicador |
| CLI Helper | `dashboard/pipeline_status.py` | Script CLI para agentes: `start`, `complete`, `advance`, `error`, `comment`, `inc_iterations`, `set_step`, `check_iterations`, `state` |
| Templates | `dashboard/templates/` | HTML + tarjetas de sesión con diseño glassmorphism oscuro |
| Static | `dashboard/static/` | CSS + JS con SSE en tiempo real, animaciones premium |
| Plantilla HTML | `dashboard/templates/index.html` | Interfaz visual que muestra pipelines en tiempo real |
| Plantilla sesión | `dashboard/templates/_session_card.html` | Tarjeta individual por sesión/pipeline |

**URL del dashboard**: `http://metagentes:5050` o `http://localhost:5050`

### Script de Instalación

| Archivo | Descripción |
|---------|-------------|
| `setup_pipeline.sh` | Instalador completo: instala dependencias (nodejs, npm, git, python3, gh), configura OpenCode, autentica GitHub wanderc137, crea estructura de carpetas, configura git global |

### Archivos de Configuración

| Archivo | Descripción |
|---------|-------------|
| `opencode.jsonc` | Configuración principal de OpenCode. default_agent: "orquestador", plugin: opencode-dynamic-context-pruning, skills paths |
| `.gitignore` | Exclusiones para el directorio de configuración de OpenCode |

---

## 🎯 SKILLS DISPONIBLES

| Skill | Archivo | Descripción |
|-------|---------|-------------|
| **cashdro-integracion** | `skills/cashdro-integracion/SKILL.md` | Integración con máquinas de pago CashDro vía REST/JSON sobre HTTPS. Contiene: API completa (startOperation, askOperation, finishOperation, etc.), manejo de errores, normalización de respuestas anidadas, clase Python canónica de referencia. |

---

## 📁 PROYECTOS REALIZADOS (21 proyectos)

Ubicados en `proyectos/`:
```
apiMercadoPublico/     CalculadoraRendimiento/  CashDroController/
ColmenaAR/             Curso_ESP32_Riego/       DHT22/
esp32cam-blink/        febos-facturador/        hospital-lorawan-dashboard/
LCD1602_I2C/           port_scanner/            PrototiposExpress/
QRProductScanner/      random-serial/           RelojMundial/
SpeechToTextAux/       SpeechToTextAuxPy/       SuBus/
TotemCarrito/          turnero/                 YL69_SoilMoisture/
```

---

## 📄 DOCUMENTACIÓN ADICIONAL

| Archivo | Descripción |
|---------|-------------|
| `VISION_CREADOR_CURSOS_TECNICOS.md` | Visión de sistema multi-agente para generar mini-cursos técnicos diarios con componentes de Aliexpress (ESP32, Arduino, Raspberry Pi, Teltonika) |
| `plan_social_agents.md` | Plan para agente publicador multi-red social (Discord, Telegram, Reddit, Dev.to, Mastodon, Hackster, Hackaday, Medium, LinkedIn) |
| `CHANGELOG.md` | Historial de cambios: suscripción Go, dashboard unificado, flujo multi-sesión interactivo |

---

## 🚀 CÓMO INSTALAR EN OTRA INSTANCIA

### Requisitos previos
- Linux/macOS con bash
- Node.js + npm (para OpenCode CLI)
- Python 3.8+
- Git
- GitHub CLI (gh)

### Pasos de instalación

```bash
# 1. Copiar todo el contenido de pipelines-export/ a la máquina destino

# 2. Ejecutar el instalador automático
bash setup_pipeline.sh

# 3. O Instalar manualmente:
#    a) Copiar agents/*.md a ~/.config/opencode/agents/
#    b) Copiar opencode.jsonc a ~/.config/opencode/opencode.jsonc
#    c) Copiar skills/ a ~/.config/opencode/skills/
#    d) Iniciar el dashboard:
#       cd AG.CreadorContenido && python3 dashboard/app.py 5050 &

# 4. Verificar instalación
ls ~/.config/opencode/agents/   # Deben aparecer 19 archivos .md
opencode --version               # OpenCode debe estar instalado
gh auth status                   # Debe mostrar wanderc137
```

### Estructura final esperada en ~/.config/opencode/:
```
~/.config/opencode/
├── opencode.jsonc
├── .gitignore
├── agents/
│   ├── orquestador.md           # Pipeline Standard
│   ├── analista.md
│   ├── implementador.md
│   ├── auditor.md
│   ├── documentador.md
│   ├── publicador.md
│   ├── orquestador-ar.md        # Pipeline Arduino
│   ├── analista-ar.md
│   ├── implementador-ar.md
│   ├── auditor-ar.md
│   ├── documentador-ar.md
│   ├── publicador-ar.md
│   ├── orquestador-loop.md      # Pipeline LOOP
│   ├── analista-loop.md
│   ├── implementador-loop.md
│   ├── auditor-loop.md
│   ├── documentador-loop.md
│   ├── publicador-loop.md
│   ├── explorador-loop.md       # Meta-agente LOOP
│   └── ... (agentes adicionales)
└── skills/
    └── cashdro-integracion/
        └── SKILL.md
```

---

## ⚙️ MODELOS Y CONFIGURACIÓN DE AGENTES

| Pipeline | Agentes | Modelo |
|----------|---------|--------|
| **Standard** | orquestador, implementador, publicador | `opencode-go/deepseek-v4-flash` |
| **Standard** | analista, auditor, documentador | `opencode-go/deepseek-v4-pro` |
| **Arduino** | orquestador-ar, implementador-ar, publicador-ar | `opencode-go/deepseek-v4-flash` |
| **Arduino** | analista-ar, auditor-ar, documentador-ar | `opencode-go/deepseek-v4-pro` |
| **LOOP** | orquestador-loop, implementador-loop, publicador-loop | `opencode-go/deepseek-v4-flash` |
| **LOOP** | analista-loop, auditor-loop, documentador-loop | `opencode-go/deepseek-v4-pro` |
| **LOOP** | explorador-loop | `opencode-go/deepseek-v4-pro` |

Todos los agentes tienen permiso `"*": allow` y modo `all`.

---

## 🔄 RESUMEN DE FLUJOS

### Pipeline Standard / LOOP
```
① Registrar → ② Clarificar → ③ Analizar (PLAN.md) → ④ Implementar (código)
→ ⑤ Auditar (AUDIT_REPORT.md) → [APROBADO? → Sí: ⑦, No: ④ (máx 7 ciclos)]
→ ⑦ Documentar (README + docs/) → ⑧ Publicar (GitHub) → ⑨ Informar
```

### Pipeline Arduino
```
① Registrar → ② Clarificar (placa+ pines) → ③ Analizar-ar (PLAN.md+pinout)
→ ④ Implementar-ar (.ino + compilar + transferir a placa USB)
→ ⑤ Auditar-ar (AUDIT_REPORT.md) → [APROBADO? → Sí: ⑦, No: ④ (máx 7 ciclos)]
→ ⑦ Documentar-ar (README + docs/ + pinout) → ⑧ Publicar-ar (GitHub) → ⑨ Informar
```

### Pipeline LOOP (meta)
```
explorador-loop recibe OBJETIVO + META
  → Para N=1..10:
      → Genera prompt mejorado según iteraciones previas
      → Invoca orquestador-loop (modo delegado, SKIP_CLARIFICATION=true)
      → Orquestador-loop ejecuta pipeline completo (analizar→implementar→...)
      → Ejecuta backtest
      → Evalúa resultado en SQLite
      → Si cumple META → ÉXITO, rompe loop
      → Si no → diagnostica, varía parámetros, siguiente iteración
  → Reporte final con tabla comparativa
```

---

*Fin del Manifiesto — Ecosistema completo de pipelines OpenCode*
