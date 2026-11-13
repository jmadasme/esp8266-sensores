# OpenCode Pipelines Export

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![OpenCode Compatible](https://img.shields.io/badge/OpenCode-Compatible-blue)
![Python 3.8+](https://img.shields.io/badge/Python-3.8+-blue.svg)
![Flask 3.0+](https://img.shields.io/badge/Flask-3.0+-green.svg)

> **Paquete de exportación auto-contenido del ecosistema completo de pipelines OpenCode.** Respalda, transfiere y replica la configuración de **19 agentes** distribuidos en **3 pipelines**, un dashboard web Flask con monitoreo en tiempo real, un skill de integración con máquinas de pago CashDro, y documentación completa del ecosistema.

---

## Tabla de contenidos

- [Resumen del ecosistema](#resumen-del-ecosistema)
- [Los 3 pipelines](#los-3-pipelines)
  - [1. Pipeline Standard (Python)](#1-pipeline-standard-python)
  - [2. Pipeline Arduino (Embebidos)](#2-pipeline-arduino-embebidos)
  - [3. Pipeline LOOP (Meta-optimización)](#3-pipeline-loop-meta-optimización)
- [Requisitos previos](#requisitos-previos)
- [Instalación en otra instancia OpenCode](#instalación-en-otra-instancia-opencode)
  - [Instalación rápida con setup_pipeline.sh](#instalación-rápida-con-setup_pipelinesh)
  - [Instalación manual paso a paso](#instalación-manual-paso-a-paso)
  - [Instalación del dashboard Python](#instalación-del-dashboard-python)
- [Dashboard — Command Center](#dashboard--command-center)
- [Configuración del token de GitHub](#configuración-del-token-de-github)
- [Estructura del repositorio](#estructura-del-repositorio)
- [Arquitectura de pipelines](#arquitectura-de-pipelines)
- [Modelos de IA asignados](#modelos-de-ia-asignados)
- [Cómo añadir o eliminar agentes](#cómo-añadir-o-eliminar-agentes)
- [Advertencias y precauciones](#advertencias-y-precauciones)
- [Licencia](#licencia)

---

## Resumen del ecosistema

Este repositorio contiene la configuración completa del ecosistema **OpenCode** del equipo `wanderc137`, compuesto por:

| Componente | Descripción |
|------------|-------------|
| **3 pipelines** | Standard (Python), Arduino (Embebidos), LOOP (Meta-optimización iterativa) |
| **19 agentes** | Agentes especializados con modelos `deepseek-v4-flash` y `deepseek-v4-pro` |
| **Dashboard web** | Command Center en Flask puerto 5050 con monitoreo en tiempo real vía SSE |
| **Skill CashDro** | Integración REST/JSON con máquinas de pago CashDro (889 líneas) |
| **Script de instalación** | `setup_pipeline.sh` — 8 pasos automáticos para replicar el entorno |
| **Documentación** | Manifiesto, Changelog, Visión de cursos técnicos, Plan de agentes sociales |

El paquete cumple un doble propósito:

1. **Respaldo**: Conservar toda la configuración de agentes, pipelines, dashboard y skills en un repositorio versionado.
2. **Transferencia**: Permitir replicar el ecosistema completo en otra máquina Linux/macOS ejecutando `bash setup_pipeline.sh`.

---

## Los 3 pipelines

### 1. Pipeline Standard (Python)

Pipeline base para proyectos Python. Recibe un requerimiento del usuario y ejecuta el flujo completo hasta la publicación en GitHub.

```
Usuario → Orquestador → Analista → Implementador → Auditor → Documentador → Publicador → GitHub
```

| Orden | Agente | Modelo | Responsabilidad |
|-------|--------|--------|----------------|
| 1 | **orquestador** | flash | Interactúa con el usuario, clarifica requerimientos, gestiona el pipeline completo, invoca a todos los demás agentes |
| 2 | **analista** | pro | Analiza el requerimiento y crea `PLAN.md` con especificación funcional, diseño técnico, estructura de archivos y casos de prueba |
| 3 | **implementador** | flash | Lee `PLAN.md`, crea el código Python, configura entorno virtual (`venv`) e instala dependencias |
| 4 | **auditor** | pro | Análisis estático completo: integridad, seguridad, funcionamiento, estabilidad, rendimiento → `AUDIT_REPORT.md` |
| 5 | **documentador** | pro | Crea `README.md` + `docs/datasheet.md` + `docs/developer.md` |
| 6 | **publicador** | flash | Inicializa git en `PROYECTO_DIR`, configura remote y sube a GitHub (cuenta `wanderc137`). **No sube** `PLAN.md` ni `AUDIT_REPORT.md` |

**Flujo de decisión post-auditoría**: Si el auditor reporta incidencias, el orquestador actualiza `PLAN.md` con correcciones y re-ejecuta implementador→auditor. Máximo **7 ciclos** de iteración. Si tras 7 ciclos persisten incidencias, se consulta al usuario.

### 2. Pipeline Arduino (Embebidos)

Pipeline especializado para proyectos Arduino/ESP32 con compilación y transferencia física a la placa vía `arduino-cli`.

```
Usuario → Orquestador-AR → Analista-AR → Implementador-AR → Auditor-AR → Documentador-AR → Publicador-AR → GitHub
                                                                           └── arduino-cli (compilar + transferir)
```

| Orden | Agente | Modelo | Responsabilidad |
|-------|--------|--------|----------------|
| 1 | **orquestador-ar** | flash | Orquestador Arduino. Clarifica placa, pines, librerías. Gestiona pipeline con compilación física |
| 2 | **analista-ar** | pro | Crea `PLAN.md` con mapeo de pines (pinout), placa objetivo, FQBN, librerías requeridas |
| 3 | **implementador-ar** | flash | Crea sketch `.ino`, compila con `arduino-cli`, transfiere firmware a placa física vía USB |
| 4 | **auditor-ar** | pro | Auditoría específica Arduino: verificación de pines, memoria RAM/Flash, timing, rebotes, cross-compile |
| 5 | **documentador-ar** | pro | Documentación con tabla de pines, diagrama de conexión, comandos `arduino-cli` |
| 6 | **publicador-ar** | flash | Publica en GitHub. **No sube binarios** compilados (`build/`, `.hex`, `.elf`) |

**Cores Arduino instalados**: `arduino:avr`, `arduino:renesas_uno`, `esp8266:esp8266`, `esp32:esp32`, `Heltec-esp32:esp32`

### 3. Pipeline LOOP (Meta-optimización)

Pipeline de optimización iterativa. Un meta-agente (**explorador-loop**) prueba múltiples estrategias invocando al pipeline LOOP completo hasta alcanzar una meta medible.

```
EXPLORADOR-LOOP (meta-agente, opera POR ENCIMA)
       │
       │ Invoca orquestador-loop en cada iteración (máx 10)
       │ Evalúa resultados en SQLite
       │ Refina estrategia entre iteraciones
       ▼
┌─ ORQUESTADOR-LOOP (modo delegado: SKIP_CLARIFICATION=true)
│      ├─ ③ ANALISTA-LOOP
│      ├─ ④ IMPLEMENTADOR-LOOP
│      ├─ ⑤ AUDITOR-LOOP
│      ├─ ⑥ DECISIÓN (APROBADO / INCIDENCIAS)
│      ├─ ⑦ DOCUMENTADOR-LOOP
│      └─ ⑧ PUBLICADOR-LOOP
└─
```

| Orden | Agente | Modelo | Responsabilidad |
|-------|--------|--------|----------------|
| **meta** | **explorador-loop** | pro | Recibe objetivo difuso + meta medible. Genera prompts iterativos, evalúa en SQLite, refina estrategia. Máx **10 iteraciones** |
| 1 | **orquestador-loop** | flash | Soporta **modo delegado** (`SKIP_CLARIFICATION=true`) cuando es invocado por el explorador |
| 2 | **analista-loop** | pro | Igual que analista estándar pero para pipeline LOOP |
| 3 | **implementador-loop** | flash | Igual que implementador estándar pero para pipeline LOOP |
| 4 | **auditor-loop** | pro | Igual que auditor estándar pero para pipeline LOOP |
| 5 | **documentador-loop** | pro | Igual que documentador estándar pero para pipeline LOOP |
| 6 | **publicador-loop** | flash | Igual que publicador estándar pero para pipeline LOOP |

El explorador-loop es ideal para problemas de optimización paramétrica como trading algorítmico, ajuste de hiperparámetros, o tuning de algoritmos. Cada iteración genera un sub-proyecto independiente (`v1/`, `v2/`, ...) con su propio repositorio GitHub y tabla en SQLite.

---

## Requisitos previos

En la **máquina destino** donde se instalará el ecosistema se necesita:

- **Sistema operativo**: Linux (Ubuntu/Debian) o macOS
- **Node.js + npm** — para OpenCode CLI
- **Python 3.8+** — para el dashboard Flask
- **Git** — para control de versiones
- **GitHub CLI (`gh`)** — para publicación de proyectos
- **Conexión a internet** — para instalar dependencias y publicar en GitHub

> **Nota**: El script `setup_pipeline.sh` instala automáticamente todas las dependencias del sistema en Ubuntu/Debian y macOS. Para otras distribuciones, consulta la [instalación manual](#instalación-manual-paso-a-paso).

---

## Instalación en otra instancia OpenCode

### Instalación rápida con setup_pipeline.sh

```bash
# 1. Clona el repositorio
git clone https://github.com/wanderc137/opencode-pipelines-export.git
cd opencode-pipelines-export

# 2. Haz ejecutable el script de instalación
chmod +x setup_pipeline.sh

# 3. [IMPORTANTE] Configura tu token de GitHub
#    Edita setup_pipeline.sh y reemplaza <TU_GITHUB_PAT_TOKEN> por tu token real.
#    Instrucciones detalladas en: #configuración-del-token-de-github

# 4. Ejecuta el instalador
bash setup_pipeline.sh
```

El script realiza **8 pasos** automáticamente:

| Paso | Acción |
|------|--------|
| **PASO 0** | Detecta SO e instala dependencias base (`nodejs`, `npm`, `git`, `python3`, `pip`, `gh`, `curl`) |
| **PASO 1** | Verifica/Instala OpenCode CLI vía `npm global` |
| **PASO 2** | Inicializa OpenCode (crea `~/.config/opencode/`) |
| **PASO 3** | Escribe la configuración de agentes en `~/.config/opencode/opencode.jsonc` |
| **PASO 4** | Autentica GitHub CLI como `wanderc137` con token PAT |
| **PASO 5** | Crea estructura de carpetas (`~/Documentos/Proyects/OpenCode/proyectos/`) |
| **PASO 6** | Configura git global (`user.name`, `user.email`, `defaultBranch main`) |
| **PASO 7** | Verifica la instalación completa (OpenCode, configuración, GitHub, estructura) |
| **PASO 8** | Muestra resumen final con instrucciones de uso |

### Instalación manual paso a paso

Si prefieres control total sobre cada paso:

```bash
# 1. Instalar OpenCode CLI
sudo npm install -g @opencode/cli

# 2. Inicializar OpenCode
mkdir -p ~/.config/opencode
opencode init

# 3. Copiar configuración de agentes
cp opencode.jsonc ~/.config/opencode/

# 4. Autenticar GitHub
#    Genera un token en https://github.com/settings/tokens (scopes: repo, workflow)
gh auth login --with-token < ~/mi-token.txt

# 5. Crear estructura de workspace
mkdir -p ~/Documentos/Proyects/OpenCode/proyectos

# 6. Configurar git
git config --global user.name "wanderc137"
git config --global user.email "wanderc137@gmail.com"
git config --global init.defaultBranch main

# 7. Instalar dependencias del dashboard
pip install flask
```

### Instalación del dashboard Python

El dashboard tiene sus propias dependencias Python. Instálalas usando el entorno virtual incluido:

```bash
cd opencode-pipelines-export
python3 -m venv venv
source venv/bin/activate        # Linux/Mac
# venv\Scripts\activate         # Windows
pip install -r requirements.txt
```

Las dependencias están definidas en [`requirements.txt`](requirements.txt):

| Librería | Versión | Propósito |
|----------|---------|-----------|
| `flask` | `>=3.0.0` | Framework web para el dashboard (endpoints REST, renderizado HTML, SSE) |

---

## Dashboard — Command Center

El dashboard es una aplicación web Flask que permite **monitorear el estado de todos los pipelines en tiempo real**.

### Iniciar el dashboard

```bash
cd opencode-pipelines-export
source venv/bin/activate         # Activar entorno virtual
python3 dashboard/app.py 5050    # Puerto por defecto: 5050
```

### Endpoints

| Endpoint | Método | Descripción |
|----------|--------|-------------|
| `/` | GET | Interfaz HTML principal con header, contadores y grid de tarjetas de sesión |
| `/api/state` | GET | JSON con el estado completo de todas las sesiones y pipelines |
| `/stream` | GET | **Server-Sent Events** — actualizaciones en tiempo real cada 2 segundos |
| `/health` | GET | Health check (`{"status":"ok","sessions":N}`) |

### Acceso

| URL | Uso |
|-----|-----|
| `http://localhost:5050` | Acceso local |
| `http://<IP-DEL-EQUIPO>:5050` | Acceso desde red local |
| `http://metagentes:5050` | Acceso por hostname (si está configurado) |

### Fuentes de datos

El dashboard obtiene el estado desde dos fuentes que se sincronizan mutuamente:

1. **`pipeline_state.json`** — Estado global de pipelines (JSON + `fcntl.flock` para thread-safety)
2. **`proyectos/sesiones/<alias>/status.json`** — Estado individual de cada sesión (creado dinámicamente por los orquestadores)

### Interfaz de usuario

La interfaz presenta un diseño **glassmorphism oscuro** con:

- **Header**: Título "OpenCode Command Center" con icono CPU y 3 contadores (total / activas / completadas)
- **Tarjetas de sesión**: Cada pipeline activo se muestra como una tarjeta independiente con:
  - Dot de estado animado (amarillo pulsante = corriendo, verde = completado, rojo = error)
  - Barra de progreso con gradiente y efecto gleam
  - Lista de steps con iconos, estados, iteraciones y comentarios
  - Vista previa de log (últimas 10 líneas) en consola tipo terminal
- **Actualización en vivo**: SSE envía datos cada 2 segundos con reconexión automática
- **Responsive**: Adaptado a escritorio, tablet y móvil

---

## Configuración del token de GitHub

El archivo `setup_pipeline.sh` contiene un placeholder que **debes reemplazar** antes de ejecutar:

```bash
TOKEN="[REDACTED]"
```

### Cómo generar tu token

1. Accede a [GitHub Settings → Developer settings → Personal access tokens → Fine-grained tokens](https://github.com/settings/tokens?type=beta)
2. Haz clic en **"Generate new token"**
3. Configura:
   - **Token name**: `opencode-pipelines`
   - **Resource owner**: Tu cuenta (`wanderc137`)
   - **Repository access**: `All repositories`
   - **Permissions**: `Contents: Read and write`, `Metadata: Read-only`
4. Haz clic en **"Generate token"** y **copia el token inmediatamente** (no se volverá a mostrar)

### Reemplazar en setup_pipeline.sh

```bash
# Edita el archivo
nano setup_pipeline.sh

# Cambia la línea 12:
TOKEN="[REDACTED]"
# Por:
TOKEN="[REDACTED]"
```

> **IMPORTANTE**: Nunca subas el token real al repositorio. El `.gitignore` excluye archivos `.env`, `*.bak` y logs, pero **NO** protege `setup_pipeline.sh` si editas el token dentro. Considera usar variables de entorno como alternativa más segura.

---

## Estructura del repositorio

```
opencode-pipelines-export/               ← Repositorio GitHub (wanderc137)
│
├── agents/                              ← 19 archivos .md de definición de agentes
│   ├── [standard]  orquestador.md, analista.md, implementador.md,
│   │               auditor.md, documentador.md, publicador.md
│   ├── [arduino]   orquestador-ar.md, analista-ar.md, implementador-ar.md,
│   │               auditor-ar.md, documentador-ar.md, publicador-ar.md
│   └── [loop]      orquestador-loop.md, analista-loop.md, implementador-loop.md,
│                   auditor-loop.md, documentador-loop.md, publicador-loop.md,
│                   explorador-loop.md
│
├── dashboard/                           ← Aplicación Flask del Command Center
│   ├── app.py                           ← Punto de entrada Flask (puerto 5050)
│   ├── pipeline_state.py                ← Gestor de estado thread-safe (JSON + fcntl.flock)
│   ├── pipeline_status.py               ← CLI para que agentes reporten estado
│   ├── __init__.py                      ← Módulo Python
│   ├── templates/
│   │   ├── index.html                   ← Página principal (header, contadores, grid)
│   │   └── _session_card.html           ← Tarjeta individual por sesión/pipeline
│   └── static/
│       ├── css/style.css                ← Glassmorphism oscuro, animaciones premium
│       └── js/dashboard.js              ← Conexión SSE + renderizado dinámico
│
├── skills/                              ← Skills de OpenCode
│   └── cashdro-integracion/
│       └── SKILL.md                     ← API completa CashDro (889 líneas, clase Python canónica)
│
├── docs/                                ← Documentación adicional
│   ├── datasheet.md                     ← Guía de instalación y uso del paquete
│   └── developer.md                     ← Explicación del código fuente y arquitectura
│
├── opencode.jsonc                       ← Configuración principal de OpenCode
├── pipeline_state.json                  ← Snapshot del estado actual de pipelines
├── pipeline_state.json.lock             ← Archivo de lock para acceso concurrente
├── setup_pipeline.sh                    ← Instalador automático (256 líneas, 8 pasos)
├── requirements.txt                     ← Dependencias Python (flask>=3.0.0)
│
├── MANIFIESTO_COMPLETO.md               ← Documentación técnica del ecosistema
├── CHANGELOG.md                         ← Historial de cambios
├── VISION_CREADOR_CURSOS_TECNICOS.md    ← Visión futura de sistema multi-agente
├── plan_social_agents.md                ← Plan para publicador multi-red social
│
├── .gitignore                           ← Exclusiones para el repositorio
└── README.md                            ← Este archivo
```

---

## Arquitectura de pipelines

### Flujo completo del Pipeline Standard

```
┌─────────────────────────────────────────────────────────────────┐
│                       ORQUESTADOR                                │
│  (único que habla con el usuario y decide)                       │
│                                                                  │
│  ① REGISTRO       → $PSTATUS <proyecto> start                   │
│  ② CLARIFICAR     → Preguntas al usuario sobre requerimientos    │
│  ③ ANALISTA       → Crea PLAN.md (especificación + diseño)      │
│  ④ IMPLEMENTADOR  → Crea código Python + venv + dependencias     │
│  ⑤ AUDITOR        → AUDIT_REPORT.md (integridad, seguridad...)  │
│  ⑥ DECISIÓN       → ¿APROBADO?                                   │
│     ├─ SÍ → continuar a ⑦                                       │
│     └─ NO → corregir PLAN.md, volver a ④ (máx 7 ciclos)         │
│  ⑦ DOCUMENTADOR   → README.md + docs/datasheet.md + dev.md      │
│  ⑧ PUBLICADOR     → git init + push a GitHub (wanderc137)       │
│  ⑨ INFORMAR       → Resumen final al usuario                    │
└─────────────────────────────────────────────────────────────────┘
```

### Mecanismo de estado

El estado de cada pipeline se persiste en `pipeline_state.json` con esta estructura:

```json
{
  "pipelines": {
    "<nombre_proyecto>": {
      "current_step": "orquestador|analista|implementador|auditor|documentador|publicador",
      "steps": {
        "<step>": {
          "status": "pending|in_progress|completed|error",
          "iterations": 0,
          "comment": "",
          "last_update": "YYYY-MM-DDTHH:MM:SS"
        }
      },
      "overall_progress": 0,
      "completed": false,
      "created_at": "YYYY-MM-DDTHH:MM:SS",
      "updated_at": "YYYY-MM-DDTHH:MM:SS"
    }
  }
}
```

El progreso se calcula por **pesos ponderados**:
- `orquestador`: 10%, `analista`: 15%, `implementador`: 30%, `auditor`: 15%, `documentador`: 15%, `publicador`: 15%
- Cada paso en progreso suma el 50% de su peso; cada paso completado suma el 100%

### Comunicación entre agentes

La comunicación se realiza mediante:

1. **Archivos en disco**: `PLAN.md`, `AUDIT_REPORT.md`, código fuente
2. **Herramienta `task`**: El orquestador invoca a los demás agentes usando `task` de OpenCode
3. **CLI `pipeline_status.py`**: Los agentes reportan su estado al dashboard mediante comandos `$PSTATUS`

---

## Modelos de IA asignados

| Pipeline | Modelo Flash (`deepseek-v4-flash`) | Modelo Pro (`deepseek-v4-pro`) |
|----------|-------------------------------------|--------------------------------|
| **Standard** | orquestador, implementador, publicador | analista, auditor, documentador |
| **Arduino** | orquestador-ar, implementador-ar, publicador-ar | analista-ar, auditor-ar, documentador-ar |
| **LOOP** | orquestador-loop, implementador-loop, publicador-loop | analista-loop, auditor-loop, documentador-loop, explorador-loop |

- **Flash**: Rápido y económico. Para tareas de coordinación (orquestadores, implementadores, publicadores).
- **Pro**: Razonamiento profundo. Para análisis, auditoría, documentación y exploración (analistas, auditores, documentadores, explorador).

---

## Cómo añadir o eliminar agentes

### Añadir un nuevo agente

1. Crea un archivo `.md` en `agents/` con **frontmatter YAML** obligatorio:

   ```yaml
   ---
   description: "Descripción del agente"
   mode: all
   model: opencode-go/deepseek-v4-flash
   permission:
     "*": allow
   ---
   ```

2. Define la personalidad, flujo de trabajo y reglas en el cuerpo del Markdown.
3. Registra el agente en `opencode.jsonc` bajo la sección `agent`.
4. Si pertenece a un pipeline, actualiza `MANIFIESTO_COMPLETO.md`.

### Eliminar un agente

1. Elimina el archivo `.md` correspondiente de `agents/`.
2. Elimina su entrada en `opencode.jsonc`.
3. Actualiza `MANIFIESTO_COMPLETO.md` para reflejar el cambio.

---

## Advertencias y precauciones

### Seguridad

- **Nunca subas tokens reales** a GitHub. El placeholder `<TU_GITHUB_PAT_TOKEN>` en `setup_pipeline.sh` debe reemplazarse con tu token **solo en tu máquina local**. No hagas commit con el token real.
- **Credenciales en `opencode.jsonc`**: El archivo de configuración no debe contener tokens, API keys ni contraseñas. Usa variables de entorno para secretos.
- **Logs excluidos**: `dashboard/dashboard.log` y cualquier `*.log` están en `.gitignore`. No incluyas logs con información sensible en el repositorio.

### Instalación

- **Solo Linux/macOS**: `setup_pipeline.sh` está probado en Ubuntu/Debian y macOS. Para otras distribuciones (Fedora, Arch) se requiere instalación manual de dependencias.
- **Node.js + npm requeridos**: OpenCode CLI es un paquete npm. Sin Node.js, la instalación no funcionará.
- **GitHub CLI (`gh`) autenticado**: Sin autenticación, el publicador no podrá subir proyectos. Verifica con `gh auth status` tras la instalación.

### Uso

- **No ejecutes múltiples pipelines sobre el mismo `PROYECTO_DIR`**: Cada pipeline debe operar en su propio directorio. El acceso concurrente al mismo `pipeline_state.json` está protegido con `fcntl.flock`, pero mezclar proyectos causa inconsistencias.
- **Límite de iteraciones**: El ciclo implementador→auditor tiene un máximo de **7 iteraciones**. Si tras 7 ciclos persisten incidencias, el pipeline se detiene y consulta al usuario.
- **No elimines `pipeline_state.json` ni `pipeline_state.json.lock`** mientras haya pipelines en ejecución. Esto corrompería el estado.

### Dashboard

- **Puerto 5050**: Asegúrate de que el puerto 5050 esté libre antes de iniciar el dashboard.
- **Sin autenticación**: El dashboard no tiene login. No lo expongas a internet sin un proxy inverso con autenticación.
- **Thread-safety**: El acceso a `pipeline_state.json` usa bloqueo de archivos (`fcntl.flock`). El dashboard usa `threaded=True` en Flask pero las escrituras son atómicas.

---

## Licencia

MIT License

Copyright (c) 2026 wanderc137

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
