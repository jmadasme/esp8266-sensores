# Datasheet — Port Scanner Local

## Documentación de Usuario

---

## 1. Identificación del Producto

| Campo                | Valor                          |
|----------------------|--------------------------------|
| **Nombre**           | Port Scanner Local             |
| **Versión**          | 1.0.0                          |
| **Tipo**             | Herramienta de diagnóstico de red |
| **Plataforma**       | Python 3.10+ (Linux, macOS, Windows) |
| **Repositorio**      | `port_scanner/`                |
| **Última actualización** | Junio 2026                  |

---

## 2. Especificaciones Técnicas

### 2.1 Generales

| Especificación              | Valor                          |
|-----------------------------|--------------------------------|
| Lenguaje de programación    | Python 3.10+                   |
| Interfaz de usuario         | Consola (stdout)               |
| Protocolo de escaneo        | TCP (conexión completa)        |
| Rango de puertos por defecto| 1 – 1024                       |
| Host por defecto            | 127.0.0.1 (loopback)           |
| Método de paralelización    | `ThreadPoolExecutor`           |
| Librerías externas          | `rich` ≥ 13.0.0 (opcional)     |
| Dependencias del sistema    | Ninguna (solo Python estándar) |

### 2.2 Rendimiento

| Métrica                        | Valor Típico                    |
|--------------------------------|---------------------------------|
| Tiempo de escaneo (1024 puertos, 50 threads) | 10 – 30 segundos   |
| Tiempo de escaneo (1024 puertos, 200 threads)| 5 – 10 segundos    |
| Uso de CPU (50 threads)        | 10 – 30% (1 core)               |
| Uso de RAM                     | ~15 – 30 MB                     |
| Conexiones simultáneas máximas | 50 (configurable)               |

### 2.3 Protocolos Soportados

| Protocolo | Detección | Banner Grabbing |
|-----------|-----------|-----------------|
| TCP       | Sí (conexión completa) | Sí (probes genéricas) |

### 2.4 Puertos y Servicios Identificables

El diccionario `known_services` en `services.py` incluye **todos los puertos del 1 al 1024** según la asignación IANA. Los puertos no incluidos se reportan como `desconocido`.

**Ejemplos de servicios identificables:**
- SSH (22), HTTP (80), HTTPS (443), FTP (21)
- SMTP (25), POP3 (110), IMAP (143)
- DNS (53), DHCP (67/68), SNMP (161)
- Y más de 1000 servicios adicionales

---

## 3. Guía de Instalación Detallada

### 3.1 Requisitos previos

```bash
# Verificar versión de Python
python3 --version
# Debe mostrar: Python 3.10.x o superior

# Verificar que pip está disponible
python3 -m pip --version
```

### 3.2 Instalación automática (recomendada)

```bash
# Descargar o clonar el proyecto
cd port_scanner

# Ejecutar — todo se configura automáticamente
python main.py
```

### 3.3 Instalación manual

```bash
# Linux / macOS
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python main.py
```

```powershell
# Windows (PowerShell)
python -m venv .venv
.venv\Scripts\Activate.ps1
pip install -r requirements.txt
python main.py
```

### 3.4 Verificación post-instalación

```bash
# Activar virtualenv
source .venv/bin/activate

# Verificar que rich está instalado (opcional)
pip list | grep rich
# rich==13.x.x

# Ejecutar escaneo de prueba
python main.py
```

---

## 4. Guía de Uso

### 4.1 Escaneo estándar

```bash
python main.py
```

Este comando inicia el escaneo completo. No requiere argumentos adicionales.

### 4.2 Interpretación de la salida

**Encabezado informativo:**
```
============================================================
  PORT SCANNER LOCAL
============================================================
  Host:    127.0.0.1
  Puertos: 1-1024
  Timeout: 2.0s
  Threads: 50
============================================================
```

**Tabla de resultados (con rich):**

| Columna   | Significado                                      |
|-----------|--------------------------------------------------|
| Puerto    | Número de puerto TCP detectado como abierto       |
| Estado    | Siempre "Abierto" (los cerrados no se listan)    |
| Servicio  | Nombre del servicio según IANA                   |
| Versión   | Banner capturado (versión del software)          |

**Leyenda de colores (con rich):**
- **Cian** — Número de puerto
- **Negrita** — Estado
- **Verde** — Nombre del servicio
- **Amarillo** — Versión / Banner

**Resumen final:**
```
Resumen: 3 puerto(s) abierto(s) encontrado(s).
Tiempo total: 12.34 segundos
```

### 4.3 Interrupción del escaneo

Presiona `Ctrl+C` en cualquier momento para detener el escaneo de forma segura:

```
[!] Escaneo interrumpido por el usuario.
```

### 4.4 Personalización de configuración

Edita las constantes al inicio de `main.py`:

```python
HOST: str = "192.168.1.1"       # IP a escanear
START_PORT: int = 1              # Puerto inicial
END_PORT: int = 500              # Puerto final (solo 1-500)
TIMEOUT: float = 3.0             # Timeout más largo
MAX_THREADS: int = 100           # Más concurrencia
```

---

## 5. Solución de Problemas

### 5.1 Error: "Python no encontrado"

**Problema**: El comando `python` no se reconoce.
**Solución**: Usa `python3` en Linux/macOS o verifica que Python esté en el PATH.

### 5.2 Error: "No se encontraron puertos abiertos"

**Posibles causas y soluciones:**

| Causa                           | Solución                                           |
|---------------------------------|----------------------------------------------------|
| No hay servicios ejecutándose   | Inicia un servicio (ej: `sudo systemctl start ssh`) |
| Firewall bloqueando loopback    | Verifica reglas de firewall local                  |
| Timeout muy bajo                | Aumenta `TIMEOUT` a 3.0 o 5.0                     |
| Host incorrecto                 | Verifica que `HOST = "127.0.0.1"`                 |

### 5.3 Error: "ImportError: No module named rich"

**Problema**: La librería `rich` no está instalada.
**Solución**: El programa funciona sin `rich` usando formato de texto plano. Para instalar `rich`:

```bash
source .venv/bin/activate
pip install rich>=13.0.0
```

### 5.4 Error: "Permission denied" al crear el virtualenv

**Problema**: Permisos insuficientes en el directorio del proyecto.
**Solución**: Asegúrate de tener permisos de escritura:

```bash
chmod u+w /ruta/al/proyecto/port_scanner
```

### 5.5 Error: "OSError: [Errno 24] Too many open files"

**Problema**: El sistema ha alcanzado el límite de archivos abiertos (sockets).
**Solución**: Reduce `MAX_THREADS` en `main.py`:

```python
MAX_THREADS: int = 30
```

O aumenta el límite del sistema:

```bash
# Linux temporal
ulimit -n 4096

# Linux permanente (en /etc/security/limits.conf)
# usuario soft nofile 4096
# usuario hard nofile 8192
```

### 5.6 Banner grabbing no devuelve versión

| Causa                           | Solución                                           |
|---------------------------------|----------------------------------------------------|
| El servicio no envía banner     | Es normal en algunos servicios (ej: servicios binarios) |
| Timeout muy corto               | Aumenta `timeout` en `grab_banner()`               |
| El servicio requiere autenticación | No es posible obtener banner sin credenciales     |
| Probes inadecuadas              | Modifica `_PROBES` en `banner.py`                  |

### 5.7 El escaneo es muy lento

| Causa                           | Solución                                           |
|---------------------------------|----------------------------------------------------|
| Pocos threads                   | Aumenta `MAX_THREADS` (máx. recomendado: 200)     |
| Timeout muy alto                | Reduce `TIMEOUT` a 1.0 o 0.5                      |
| Rango de puertos muy amplio     | Reduce `END_PORT` o cambia `START_PORT`           |
| Sistema con recursos limitados  | Reduce `MAX_THREADS` a 20-30                      |

### 5.8 Error: "address already in use"

**Problema**: El puerto de loopback está siendo usado por otra aplicación.
**Solución**: No afecta al escaneo; el scanner es cliente, no servidor.

### 5.9 El script no se re-ejecuta en el virtualenv

**Problema**: En algunos sistemas, `os.execv` puede fallar.
**Solución**: Activa manualmente el virtualenv y ejecuta:

```bash
source .venv/bin/activate
python main.py
```

---

## 6. Mantenimiento

### 6.1 Actualización de dependencias

```bash
source .venv/bin/activate
pip install --upgrade rich
```

### 6.2 Recreación del entorno virtual

```bash
rm -rf .venv
python main.py   # Se recrea automáticamente
```

### 6.3 Ampliación del mapa de servicios

Para agregar servicios no estándar, edita `services.py`:

```python
known_services = {
    # ... puertos existentes ...
    8080: "http-alt",
    8443: "https-alt",
    27017: "mongodb",   # Puerto no incluido en el rango 1-1024
}
```

---

## 7. Registro de Versiones

| Versión | Fecha      | Cambios                                      |
|---------|------------|----------------------------------------------|
| 1.0.0   | Jun 2026   | Versión inicial completa                     |
| 0.9.0   | —          | Beta: escaneo funcional, banner grabbing     |
| 0.1.0   | —          | Prototipo inicial con escaneo básico         |

---

## 8. Contacto y Soporte

Para reportar problemas o sugerir mejoras, consulta el repositorio del proyecto o abre un issue en el sistema de seguimiento correspondiente.
