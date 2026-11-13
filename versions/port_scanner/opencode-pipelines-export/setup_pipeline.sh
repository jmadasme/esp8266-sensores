#!/bin/bash
set -e

# =============================================================================
# setup_pipeline.sh
# Instalador automático del ecosistema completo de pipelines OpenCode
# Incluye: Pipeline Standard (Python), Pipeline Arduino, Pipeline LOOP
# 
# Uso: bash setup_pipeline.sh
# =============================================================================

TOKEN="[REDACTED]"
GITHUB_USER="wanderc137"
OPENCODE_CONFIG_DIR="$HOME/.config/opencode"
OPENCODE_CONFIG_FILE="$OPENCODE_CONFIG_DIR/opencode.jsonc"
WORKSPACE_BASE="$HOME/Documentos/Proyects/OpenCode"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m'

log()   { echo -e "${GREEN}[✓]${NC} $1"; }
warn()  { echo -e "${YELLOW}[!]${NC} $1"; }
error() { echo -e "${RED}[✗]${NC} $1"; }
info()  { echo -e "${CYAN}[i]${NC} $1"; }
header(){ echo ""; echo "════════════════════════════════════════════"; echo -e "  $1"; echo "════════════════════════════════════════════"; }

# =============================================================================
# PASO 0: Detectar SO e instalar dependencias base
# =============================================================================
header "PASO 0/8 — Dependencias del sistema"

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    info "Ubuntu/Debian detectado"
    sudo apt update
    sudo apt install -y nodejs npm git python3 python3-pip gh curl
elif [[ "$OSTYPE" == "darwin"* ]]; then
    info "macOS detectado"
    if ! command -v brew &>/dev/null; then
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    brew install node git gh python3
else
    warn "SO no probado. Instala manualmente: nodejs, npm, git, python3, gh"
fi

log "Dependencias base instaladas"

# =============================================================================
# PASO 1: Verificar/Instalar OpenCode
# =============================================================================
header "PASO 1/8 — OpenCode CLI"

if command -v opencode &>/dev/null; then
    log "OpenCode ya está instalado: $(opencode --version 2>/dev/null || echo 'ok')"
else
    info "Instalando OpenCode..."
    sudo npm install -g @opencode/cli
    log "OpenCode instalado"
fi

# =============================================================================
# PASO 2: Inicializar OpenCode
# =============================================================================
header "PASO 2/8 — Inicializar OpenCode"

if [ -f "$OPENCODE_CONFIG_FILE" ]; then
    info "OpenCode ya inicializado (respaldando configuración anterior)"
    cp "$OPENCODE_CONFIG_FILE" "$OPENCODE_CONFIG_FILE.bak.$(date +%s)"
else
    mkdir -p "$OPENCODE_CONFIG_DIR"
    opencode init 2>/dev/null || warn "init no disponible, creando configuración manual"
fi
log "OpenCode inicializado"

# =============================================================================
# PASO 3: Escribir configuración de agentes
# =============================================================================
header "PASO 3/8 — Configuración de agentes"

cat > "$OPENCODE_CONFIG_FILE" << 'CONFIGEOF'
{
  "$schema": "https://opencode.ai/config.json",
  "permission": "allow",
  "default_agent": "analista",
  "agent": {
    "analista": {
      "description": "Analista de requerimientos. Clarifica, genera plan, orquesta el pipeline. CRITICO: Todo el proyecto debe crearse dentro de la carpeta proyectos/NombreProyecto/. El PLAN.md y AUDIT_REPORT.md van dentro de proyectos/NombreProyecto/.",
      "mode": "all",
      "permission": { "*": "allow" }
    },
    "implementador": {
      "description": "Implementador de código Arduino/Python. Toma el plan y crea los archivos fuente. CRITICO: Crear todo DENTRO de proyectos/NombreProyecto/. Ejemplo: proyectos/LCD1602_I2C/ o proyectos/YL69_SoilMoisture/.",
      "mode": "all",
      "permission": { "*": "allow" }
    },
    "auditor": {
      "description": "Auditor de código. Revisa integridad, seguridad, funcionamiento, estabilidad, rendimiento. CRITICO: El AUDIT_REPORT.md se guarda dentro de proyectos/NombreProyecto/.",
      "mode": "all",
      "permission": { "*": "allow" }
    },
    "documentador": {
      "description": "Genera documentación de usuario (datasheet) y desarrollador (explicación de código fuente). Además crea un README.md de alta calidad con: explicación del dispositivo, principio físico de funcionamiento, forma de operar, pinout, ejemplos de uso realistas, diagrama de conexión, calibración, y advertencias. El README debe ser estético, bien estructurado y masivo en contenido. CRITICO: Crear docs/ DENTRO de proyectos/NombreProyecto/docs/. Un solo disparo al final del pipeline.",
      "mode": "all",
      "permission": { "*": "allow" }
    },
    "publicador": {
      "description": "Sube a GitHub el proyecto completo (código fuente, documentación, assets y README.md). USA SIEMPRE la cuenta wanderc137. NO usar jmadasme. CRITICO: NO subir PLAN.md ni AUDIT_REPORT.md. CRITICO: Inicializar git dentro de proyectos/NombreProyecto/ como repositorio independiente y subir SOLO ese proyecto. NO mezclar proyectos en el mismo repo.",
      "mode": "all",
      "permission": { "*": "allow" }
    }
  }
}
CONFIGEOF

log "Archivo de configuración creado: $OPENCODE_CONFIG_FILE"

# =============================================================================
# PASO 4: Autenticar GitHub wanderc137
# =============================================================================
header "PASO 4/8 — Autenticación GitHub (wanderc137)"

# Verificar si gh está autenticado como wanderc137
CURRENT_USER=$(env -u GITHUB_TOKEN gh api user --jq '.login' 2>/dev/null || echo "")

if [ "$CURRENT_USER" = "$GITHUB_USER" ]; then
    log "Ya autenticado como $GITHUB_USER"
else
    info "Autenticando como $GITHUB_USER..."
    
    # Hacer logout de cualquier cuenta actual
    env -u GITHUB_TOKEN gh auth logout --hostname github.com 2>/dev/null || true
    
    # Login con token (usando archivo temporal para evitar exposicion en ps aux)
    TMP_TOKEN_FILE=$(mktemp) && chmod 600 "$TMP_TOKEN_FILE" && echo -n "$TOKEN" > "$TMP_TOKEN_FILE"
    env -u GITHUB_TOKEN gh auth login --with-token < "$TMP_TOKEN_FILE" 2>&1
    rm -f "$TMP_TOKEN_FILE"
    
    # Asegurar que sea la cuenta activa
    env -u GITHUB_TOKEN gh auth switch --user "$GITHUB_USER" 2>/dev/null || true
    
    log "Autenticado como $GITHUB_USER"
fi

# Verificar
AUTH_CHECK=$(env -u GITHUB_TOKEN gh auth status 2>&1 | grep -c "wanderc137" || true)
if [ "$AUTH_CHECK" -gt 0 ]; then
    log "Verificación de autenticación: OK"
else
    warn "Verificación de autenticación falló. Revisa manualmente con: env -u GITHUB_TOKEN gh auth status"
fi

# =============================================================================
# PASO 5: Crear estructura de carpetas
# =============================================================================
header "PASO 5/8 — Estructura de carpetas"

mkdir -p "$WORKSPACE_BASE"
mkdir -p "$WORKSPACE_BASE/proyectos"

# Crear .gitignore global para proyectos
cat > "$WORKSPACE_BASE/.gitignore" << 'GIEOF'
PLAN.md
AUDIT_REPORT.md
plantilla_especificacion.md
*.bak
GIEOF

log "Workspace creado: $WORKSPACE_BASE"
log "Carpeta proyectos: $WORKSPACE_BASE/proyectos"

# =============================================================================
# PASO 6: Configurar git global
# =============================================================================
header "PASO 6/8 — Configuración git"

git config --global user.name "wanderc137"
git config --global user.email "wanderc137@gmail.com"
git config --global init.defaultBranch main

log "Git configurado: wanderc137"

# =============================================================================
# PASO 7: Verificación final
# =============================================================================
header "PASO 7/8 — Verificación final"

ERRORS=0

# Verificar OpenCode
if command -v opencode &>/dev/null; then
    log "OpenCode CLI: INSTALADO"
else
    error "OpenCode CLI: NO INSTALADO"
    ERRORS=$((ERRORS + 1))
fi

# Verificar configuración
if [ -f "$OPENCODE_CONFIG_FILE" ]; then
    log "Configuración de agentes: OK"
else
    error "Configuración de agentes: FALTANTE"
    ERRORS=$((ERRORS + 1))
fi

# Verificar GitHub
GH_OK=$(env -u GITHUB_TOKEN gh auth status 2>&1 | grep -c "wanderc137" || true)
if [ "$GH_OK" -gt 0 ]; then
    log "GitHub wanderc137: AUTENTICADO"
else
    error "GitHub wanderc137: NO AUTENTICADO"
    ERRORS=$((ERRORS + 1))
fi

# Verificar estructura
if [ -d "$WORKSPACE_BASE/proyectos" ]; then
    log "Estructura proyectos/: OK"
else
    error "Estructura proyectos/: FALTANTE"
    ERRORS=$((ERRORS + 1))
fi

# =============================================================================
# PASO 8: Resumen final
# =============================================================================
header "PASO 8/8 — RESUMEN FINAL"

if [ "$ERRORS" -eq 0 ]; then
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║           ✅ PIPELINE INSTALADO CON ÉXITO          ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "  📁  Workspace:    ${CYAN}$WORKSPACE_BASE${NC}"
    echo -e "  📋  Config:      ${CYAN}$OPENCODE_CONFIG_FILE${NC}"
    echo -e "  🔐  GitHub:      ${CYAN}$GITHUB_USER${NC}"
    echo -e "  📦  Proyectos:   ${CYAN}$WORKSPACE_BASE/proyectos/${NC}"
    echo -e "  🤖  Agentes:     ${CYAN}analista → implementador → auditor → documentador → publicador${NC}"
    echo ""
    echo -e "  ${YELLOW}🚀 Para empezar:${NC}"
    echo -e "     cd $WORKSPACE_BASE"
    echo -e "     opencode"
    echo -e "     @analista [especificación técnica del dispositivo]"
    echo ""
else
    echo ""
    echo -e "${RED}╔════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║   ⚠️  HUBO $ERRORS ERROR(ES) EN LA INSTALACIÓN   ║${NC}"
    echo -e "${RED}╚════════════════════════════════════════════╝${NC}"
    echo ""
    echo "Revisa los mensajes de error arriba y corrige manualmente."
    echo "Luego vuelve a ejecutar: bash setup_pipeline.sh"
    echo ""
fi
