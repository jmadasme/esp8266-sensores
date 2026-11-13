---
name: cashdro-integracion
description: Use ONLY when integrating with CashDro cash payment machines via Web Service (REST/JSON over HTTPS). Covers startOperation, acknowledgeOperationId, askOperation polling, finishOperation (finalizar/cancelar), setOperationImported, doTest, askLevels, askAlerts, askDiagnosis, askPendingOperations, askOperationInExecution, receipt printing, web visualization, SSL certificate handling, error codes, retry logic (-1900), response normalization (nested unwrap), cancel_event pattern, and homologation requirements. Use for POS/TPV software integrating cash payments.
license: MIT
compatibility: opencode
metadata:
  audience: integrators
  protocol: HTTPS REST JSON (GET params)
  device: CashDro (todos los modelos)
  version: >=22.01.21.11
---

# CashDro — Integración por Web Service

## ¿Qué es CashDro?

Sistema de gestión de efectivo (CDM — Cash Deposit Module / Cash Recycler) para puntos de venta automatizados y autoservicio. Integra:

- **Aceptador de billetes**: validación multiespectral (visible, IR, UV), magnética, dimensional
- **Aceptador de monedas**: clasificación por diámetro, peso, composición metálica
- **Reciclador**: almacena billetes/monedas en cassettes por denominación; reutiliza el efectivo ingresado para dar cambio
- **Comunicación**: API REST sobre HTTPS (Ethernet RJ45), responde JSON

**No usa USB ni serie — solo red Ethernet.**

- **Web demo:** https://integration.cashdro.com
- **Puerto:** 443/TCP (HTTPS) o 80/TCP (HTTP si se habilita)
- **Base URL Web Service:** `https://<ip>/Cashdro3WS/index3.php`
- **Base URL Interfaz Web:** `https://<ip>/Cashdro3Web/index.html`
- **Versión mínima firmware:** 22.01.21.11

## Arquitectura

```
┌──────────────────┐   HTTPS GET (JSON)    ┌──────────────────┐
│   HOST (TPV)     │ ◄──────────────────► │    CASHDRO        │
│  Windows/Linux   │   Cashdro3WS/         │  (IP fija)        │
│  Python/Node/etc │   index3.php          │  192.168.1.252    │
└──────────────────┘                       └──────────────────┘
       │                                          │
       │   HTTPS GET (HTML)                       │
       └──────────────────────────────────────────┘
             Cashdro3Web/index.html
```

Dos canales:
1. **Web Service** → transacciones, consultas → responde JSON
2. **Interfaz Web** → visualización de pantallas → responde HTML

## Niveles de integración

| Nivel       | Alcance                                                                 |
|-------------|-------------------------------------------------------------------------|
| **Básico**  | Venta completa, cancelar venta, redondeo por falta de monedas, pago     |
| **Avanzado**| Todo lo anterior + finalizar operaciones                                |
| **Completo** | Todo lo anterior + ingreso, carga, retirada, consultas, impresión de comprobantes |

No es necesario implementar todas las operaciones para integrar.

---

## SEGURIDAD — Certificados SSL

CashDro usa certificados autofirmados. Tres opciones:

### Opción 1: No validar (más común en desarrollo)

Deshabilitar verificación SSL en el cliente HTTP y suprimir warnings:

```python
import urllib3
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

# requests
response = requests.get(url, verify=False)
```

```php
curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, false);
```

### Opción 2: Activar HTTP (puerto 80)

Un técnico debe: **Configuración → Parámetros generales → Integración Web** → activar HTTP. Requiere reinicio.

### Opción 3: Instalar certificados propios

1. Colocar en USB (FAT32):
   - `cashdro/ssl/private.key` (PEM)
   - `cashdro/ssl/certificate.crt` (X509)
2. Insertar USB en CashDro.
3. Ir a **Configuración → Parámetros generales → Integración Web** → "Habilitar certificados SSL".
4. Reiniciar CashDro.

---

## PROTOCOLO — Parámetros comunes

**Todas las llamadas son HTTP GET con query string.**

| Parámetro    | Obligatorio     | Descripción                                          |
|-------------|-----------------|------------------------------------------------------|
| `operation` | Siempre         | Nombre de la operación (doTest, startOperation, etc.) |
| `name`      | Casi siempre    | Usuario de CashDro (ej: `manager`, `admin`)          |
| `password`  | Casi siempre    | Contraseña del usuario en CashDro                    |
| `posid`     | Opcional        | ID del terminal TPV (máx 50 chars)                   |
| `posuser`   | Opcional        | Nombre del operador/vendedor (máx 50 chars)          |
| `aliasid`   | Opcional        | ID propio del Host para la operación (máx 50 chars)  |

**IMPORTANTE DE SEGURIDAD:** Las credenciales (`name`/`password`) se envían como parámetros GET en la URL, visibles en texto plano. Esto es requisito del diseño de la API CashDro. NO habilitar logging de depuración HTTP en producción para no exponer credenciales.

El usuario `name`/`password` debe existir en CashDro y tener permisos. Es independiente del `posuser` (operador del TPV).

**Parámetros con valores JSON:** Cuando un parámetro requiere un objeto JSON (ej: `parameters`), se envía como string JSON en la query:
```
&parameters={"amount":"1825"}
```
El código cliente debe serializar dict/list a JSON string.

---

## TRANSACCIÓN DE VENTA (type=4) — Flujo completo

Flujo obligatorio:
```
startOperation → acknowledgeOperationId → [poll askOperation hasta state=F] → setOperationImported
```

### Paso 1: startOperation

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=startOperation
  &name=<name>
  &password=[REDACTED]
  &type=4
  &posid=<posid>
  &posuser=<posuser>
  &aliasid=<aliasid>
  &parameters={"amount":"<amount>"}
```

- `type=4` = venta
- `amount` = importe en **céntimos** (×100). Ej: 18.25€ → `"1825"`
- Monto máximo: 9999.99€
- La respuesta puede tener la estructura anidada **doble** (ver sección "Normalización de respuestas")

**Respuesta exitosa (code=1):**
```json
{
  "code": 1,
  "response": {
    "errorMessage": "none",
    "operation": {
      "operationId": "223"
    }
  }
}
```

### Paso 2: acknowledgeOperationId

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=acknowledgeOperationId
  &name=<name>
  &password=[REDACTED]
  &operationId=<operationId>
```

### Paso 3: askOperation — Polling

Repetir cada 2s hasta `state="F"` (timeout recomendado: 120s).

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=askOperation
  &operationId=<operationId>
  &name=<name>
  &password=[REDACTED]
```

**Campos clave de la respuesta (ya normalizada — sin doble anidamiento):**

| Campo                         | Descripción                                                                   |
|-------------------------------|-------------------------------------------------------------------------------|
| `state`                       | **I** = pendiente acknowledge, **Q** = en cola, **E** = ejecutando, **F** = finalizada |
| `total`                       | Importe total de la transacción (×100)                                        |
| `totalin`                     | Importe introducido por el cliente (×100)                                     |
| `totalout`                    | Importe dispensado como cambio (×100)                                         |
| `change`                      | Cambio devuelto (×100) — campo adicional presente en algunas respuestas       |
| `payInProgress`               | 0 = falta dinero, 1 = importe completo y almacenado                           |
| `payOutProgress`              | 0 = faltan billetes por retirar, 1 = todos retirados                          |
| `withError`                   | "true" si hay error físico (atasco, etc.), "false" si ok                      |
| `amountchangenotavailable`    | Importe no devuelto por falta de cambio (×100)                                |
| `devices[].type`              | 1 = validador monedas, 2 = reciclador monedas, 3 = billetero                  |
| `devices[].pieces[].value`    | Denominación (ej: "500" = 5€, "1000" = 10€)                                  |
| `devices[].pieces[].finishlevelrecycler` | Nivel final en reciclador tras la operación                       |
| `devices[].pieces[].finishlevelcassette` | Nivel final en cassette tras la operación                         |
| `messages[]`                  | Eventos: 0=Pieza rechazada falta cambio, 1=No reciclable, 2=No aceptada temp, 3=No aceptada, 4=No reconocida, 5=Reciclador lleno |

**Validación de estado durante el polling:** Los estados válidos en progreso son `I`, `Q`, `E`, `""` (vacío). Cualquier otro estado es un error. Cuando `state="F"`, verificar `withError`: si es `"true"`, la operación finalizó con error físico.

### Paso 4: setOperationImported

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=setOperationImported
  &name=<name>
  &password=[REDACTED]
  &operationId=<operationId>
```

---

## CANCELAR / FINALIZAR (finishOperation)

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=finishOperation
  &name=<name>
  &password=[REDACTED]
  &operationId=<operationId>
  &type=<endType>
```

- `type=1` → Finalizar normalmente
- `type=2` → Cancelar / anular

---

## TABLA COMPLETA DE ERRORES (code < 0)

| code  | errorMessage                                  | Significado                                      | Acción recomendada                        |
|-------|-----------------------------------------------|--------------------------------------------------|-------------------------------------------|
| -1    | Authentication Failed                         | Usuario/password incorrectos                     | Verificar credenciales                    |
| -2    | Operation not queued / not found              | CashDro ocupado o operationId inválido           | Reintentar tras unos segundos             |
| -3    | Wrong amount / Invalid parameters             | Importe incorrecto o parámetros inválidos        | Verificar amount en céntimos              |
| -4    | User does not have permission                 | Usuario sin permiso para esa operación           | Usar otro usuario o asignar permisos      |
| -99   | Invalid Parameters                            | Parámetros incorrectos en la llamada             | Revisar query string                      |
| -998  | Can't start operation. Check system status    | CashDro fuera de servicio                        | Revisar diagnóstico                       |
| -999  | Cashdro service is not running                | El servicio CashDro está parado                  | Reiniciar servicio CashDro                |
| -1900 | System busy                                   | CashDro cargando/actualizando                    | **Reintentar automáticamente** (3 reintentos, 5s entre cada uno) |

---

## NORMALIZACIÓN DE RESPUESTAS (ANIDAMIENTO DOBLE)

**CRÍTICO:** La API CashDro a veces anida la respuesta en un nivel extra. El cliente DEBE normalizar esto:

```python
# Ejemplo: startOperation puede devolver:
# {"response": {"operation": {"operationId": "452"}}}
# O:
# {"response": {"operation": {"operation": {"operationId": "452"}}}}

response = data.get("response", {})
operation = response.get("operation", {})

# Normalizar: si operation contiene otra clave "operation", desanidar
if "operation" in operation:
    operation = operation["operation"]

operation_id = operation.get("operationId", "")
```

**Este patrón se repite en varias operaciones:**

| Operación       | Clave a desanidar si existe doble nivel |
|-----------------|------------------------------------------|
| startOperation  | `response.operation.operation`           |
| askOperation    | `response.operation.operation`           |
| askLevels       | `response.levels.levels`                 |
| askAlerts       | `response.alerts.alerts`                 |
| askDiagnosis    | `response.diagnosis.diagnosis`           |

Siempre verificar `isinstance(x, dict)` antes de desanidar.

---

## OPERACIONES SIN MOVIMIENTO DE EFECTIVO

### doTest — Verificar conexión

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=doTest
  &name=<name>
  &password=[REDACTED]
```

Responde `{"code":1,"response":{"errorMessage":"none"}}` si la conexión y autenticación son correctas.

### askLevels — Niveles de efectivo

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=askLevels
  &name=<name>
  &password=[REDACTED]
```

Retorna diccionario con denominaciones y cantidades. Requiere normalización de doble nivel.

### askAlerts — Alertas activas

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=askAlerts
  &name=<name>
  &password=[REDACTED]
```

Retorna una **lista** de alertas. Requiere normalización: puede venir como `response.alerts` (lista) o `response.alerts.alerts` (dict con lista dentro). Cada alerta tiene `code` y `description`.

### askDiagnosis — Diagnóstico del equipo

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=askDiagnosis
  &name=<name>
  &password=[REDACTED]
```

**NOTA:** El nombre de la operación es `askDiagnosis` (no `doDiagnosis`). Confirmado por implementación funcionando en producción.

Retorna estado de todos los subsistemas. Valores típicos: `"OK"`, `"true"`, `"1"` para estado correcto.

### askPendingOperations — Operaciones pendientes de importar

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=askPendingOperations
  &name=<name>
  &password=[REDACTED]
```

### askOperations — Operaciones entre fechas

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=askOperations
  &name=<name>
  &password=[REDACTED]
  &dateFrom=<YYYY-MM-DD>
  &dateTo=<YYYY-MM-DD>
```

### askOperationInExecution — Operación en ejecución

```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=askOperationInExecution
  &name=<name>
  &password=[REDACTED]
```

### Impresión de comprobantes

**Consultar pendientes:**
```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=askPendingReceipts
  &name=<name>
  &password=[REDACTED]
```

**Marcar como impreso:**
```
GET https://<ip>/Cashdro3WS/index3.php?
  operation=setReceiptPrinted
  &name=<name>
  &password=[REDACTED]
  &receiptId=<receiptId>
```

---

## VISUALIZACIÓN WEB (Interfaz HTML)

### Menú directo (sin login)

```
https://<ip>/Cashdro3Web/index.html#/menu?username=<name>&password=[REDACTED]
```

### Pantalla splash (lo que muestra CashDro ahora)

```
https://<ip>/Cashdro3Web/index.html#/splash/true
```

---

## OTROS TIPOS DE TRANSACCIÓN

Mismo flujo de 4 pasos que venta. Cambiar `type` y `parameters`:

| type | Transacción | parameters (ejemplo)                 |
|------|-------------|--------------------------------------|
| 4    | Venta/Cobro | `{"amount":"<amount>"}`              |
| *    | Pago        | `{"amount":"<amount>"}`              |
| *    | Ingreso     | `{"amount":"<amount>"}`              |
| *    | Carga       | `{"amount":"<amount>"}`              |
| *    | Retirada    | `{"amount":"<amount>"}`              |

*Los type numéricos para pago/ingreso/carga/retirada no están documentados en el manual. Consultar con soporte o probar en https://integration.cashdro.com.

---

## CLASE PYTHON COMPLETA (REFERENCIA CANÓNICA)

Basada en implementación funcionando en producción (CashDroController v1.0):

```python
"""
cashdro_api.py — Cliente API REST para CashDro.
Implementación de referencia basada en producción.
"""

from __future__ import annotations

import json
import time
from typing import Any, Callable, Optional

import requests
import urllib3

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)


# ─── Configuración (ajustable) ──────────────────────────────────

CASHDRO_HOST = "192.168.1.212"
CASHDRO_USER = "manager"
CASHDRO_PASSWORD = "2"
API_BASE_PATH = "/Cashdro3WS/index3.php"

REQUEST_TIMEOUT = 10          # timeout por llamada HTTP (segundos)
MAX_RETRIES = 3               # reintentos ante fallo de conexión
MAX_RETRIES_SYSTEM_BUSY = 3   # reintentos extra para código -1900
RETRY_DELAY_SYSTEM_BUSY = 5   # espera entre reintentos -1900 (segundos)
POLL_INTERVAL = 2             # segundos entre consultas de estado
POLL_TIMEOUT = 120            # timeout total de polling

POS_ID = "1"
POS_USER = "manager"
ALIAS_ID = "CKL120T2"
AMOUNT_MAX = 9999.99

ERROR_CODES = {
    -1: "Authentication Failed — usuario o contraseña incorrectos",
    -2: "Operation not queued / not found",
    -3: "Wrong amount / Invalid parameters — monto inválido",
    -4: "User does not have permission — usuario sin permisos",
    -99: "Invalid Parameters",
    -1900: "System busy — sistema ocupado, reintente más tarde",
    -998: "Can't start operation. Check system status",
    -999: "CashDro service is not running",
}


class CashDroError(Exception):
    """Excepción con código de error CashDro."""
    def __init__(self, code: int, message: str = "") -> None:
        self.code = code
        self.message = message or ERROR_CODES.get(code, f"Error desconocido ({code})")
        super().__init__(f"[{code}] {self.message}")


class CashDroAPI:
    """
    Cliente API REST CashDro.

    NOTA DE SEGURIDAD: Las credenciales se envían como parámetros GET en la URL.
    Esto es requisito del diseño de la API CashDro. NO habilitar logging de
    depuración HTTP en producción para no exponer credenciales.
    """

    def __init__(
        self,
        host: str = CASHDRO_HOST,
        user: str = CASHDRO_USER,
        password: str = CASHDRO_PASSWORD,
        timeout: int = REQUEST_TIMEOUT,
        max_retries: int = MAX_RETRIES,
    ) -> None:
        self.base_url = f"https://{host}{API_BASE_PATH}"
        self.user = user
        self.password = [REDACTED]
        self.timeout = timeout
        self.max_retries = max_retries

    # ─── Método interno de llamada HTTP ─────────────────────────

    def _call(self, operation: str, params: Optional[dict[str, Any]] = None) -> dict[str, Any]:
        """
        Ejecuta llamada GET al CashDro con reintentos.

        Lógica de reintento:
        - Fallo de conexión (ConnectionError, Timeout): hasta MAX_RETRIES intentos, 1s entre ellos
        - Código -1900 (System busy): hasta MAX_RETRIES_SYSTEM_BUSY intentos extras, 5s entre ellos
        - Cualquier otro código de error: lanza CashDroError inmediatamente
        """
        query_params: dict[str, Any] = {
            "operation": operation,
            "name": self.user,
            "password": self.password,
        }
        if params:
            for key, value in params.items():
                if isinstance(value, (dict, list)):
                    query_params[key] = json.dumps(value)
                else:
                    query_params[key] = value

        last_exception: Optional[Exception] = None
        system_busy_retries_left = MAX_RETRIES_SYSTEM_BUSY

        for attempt in range(1, self.max_retries + 1):
            try:
                response = requests.get(
                    self.base_url,
                    params=query_params,
                    timeout=self.timeout,
                    verify=False,
                )
                response.raise_for_status()
                data: dict[str, Any] = response.json()

                code = data.get("code", -99)
                if code != 1:
                    error_msg = ""
                    if "response" in data and isinstance(data["response"], dict):
                        error_msg = data["response"].get("errorMessage", "")
                    # -1900: reintentar automáticamente
                    if code == -1900 and system_busy_retries_left > 0:
                        system_busy_retries_left -= 1
                        time.sleep(RETRY_DELAY_SYSTEM_BUSY)
                        continue
                    raise CashDroError(code, error_msg)

                return data

            except (requests.exceptions.ConnectionError,
                    requests.exceptions.Timeout,
                    requests.exceptions.RequestException) as e:
                last_exception = e
                if attempt < self.max_retries:
                    time.sleep(1)
                continue

        if system_busy_retries_left <= 0 and last_exception is None:
            raise CashDroError(-1900, "System busy persistente tras múltiples intentos")
        raise requests.exceptions.RequestException(
            f"Error de conexión tras {self.max_retries} intentos: {last_exception}"
        ) from last_exception

    # ─── Operaciones de la API ──────────────────────────────────

    def test_connection(self) -> dict[str, Any]:
        """doTest — Verifica conexión y autenticación."""
        return self._call("doTest")

    def start_operation(
        self,
        op_type: int = 4,
        amount: Optional[float] = None,
        posid: str = POS_ID,
        posuser: str = POS_USER,
        aliasid: str = ALIAS_ID,
    ) -> str:
        """
        startOperation — Inicia transacción.

        Returns: operationId (string)
        """
        params: dict[str, Any] = {
            "type": str(op_type),
            "posid": posid,
            "posuser": posuser,
            "aliasid": aliasid,
        }
        if amount is not None:
            amount_cents = int(round(amount * 100))
            params["parameters"] = {"amount": str(amount_cents)}

        data = self._call("startOperation", params)
        response = data.get("response", {})
        operation = response.get("operation", {})

        # Normalizar anidamiento doble
        if "operation" in operation:
            operation = operation["operation"]

        operation_id: str = operation.get("operationId", "")
        if not operation_id:
            raise CashDroError(-99, "No se recibió operationId")
        return operation_id

    def acknowledge_operation(self, operation_id: str) -> dict[str, Any]:
        """acknowledgeOperationId — Confirma ejecución."""
        return self._call("acknowledgeOperationId", {"operationId": operation_id})

    def ask_operation(self, operation_id: str) -> dict[str, Any]:
        """
        askOperation — Consulta estado.
        Retorna dict ya normalizado (sin doble anidamiento).
        """
        data = self._call("askOperation", {"operationId": operation_id})
        response = data.get("response", {})
        operation = response.get("operation", {})
        if "operation" in operation:
            operation = operation["operation"]
        return operation

    def finish_operation(self, operation_id: str, end_type: int = 1) -> dict[str, Any]:
        """
        finishOperation — Finaliza (end_type=1) o cancela (end_type=2).
        """
        return self._call("finishOperation", {
            "operationId": operation_id,
            "type": str(end_type)
        })

    def set_operation_imported(self, operation_id: str) -> dict[str, Any]:
        """setOperationImported — Marca como importada."""
        return self._call("setOperationImported", {"operationId": operation_id})

    def ask_levels(self) -> dict[str, Any]:
        """askLevels — Niveles de efectivo."""
        data = self._call("askLevels")
        response = data.get("response", {})
        levels = response.get("levels", {})
        if "levels" in levels:
            levels = levels["levels"]
        return levels

    def ask_alerts(self) -> list[dict[str, Any]]:
        """askAlerts — Alertas activas. Retorna lista."""
        data = self._call("askAlerts")
        response = data.get("response", {})
        alerts = response.get("alerts", [])
        if isinstance(alerts, dict) and "alerts" in alerts:
            alerts = alerts["alerts"]
        return alerts if isinstance(alerts, list) else []

    def ask_diagnosis(self) -> dict[str, Any]:
        """askDiagnosis — Diagnóstico del equipo."""
        data = self._call("askDiagnosis")
        response = data.get("response", {})
        diagnosis = response.get("diagnosis", {})
        if "diagnosis" in diagnosis:
            diagnosis = diagnosis["diagnosis"]
        return diagnosis

    # ─── Polling con cancelación ────────────────────────────────

    def wait_for_completion(
        self,
        operation_id: str,
        poll_interval: int = POLL_INTERVAL,
        poll_timeout: int = POLL_TIMEOUT,
        cancel_event: Optional[Callable[[], bool]] = None,
    ) -> dict[str, Any]:
        """
        Polling hasta state='F' o timeout.

        Args:
            operation_id: ID de la operación.
            poll_interval: Segundos entre consultas.
            poll_timeout: Timeout total en segundos.
            cancel_event: Función opcional que retorna True para cancelar.

        Returns:
            Estado final normalizado de la operación.

        Raises:
            TimeoutError: Si se agota el tiempo.
            CashDroError: Si hay error durante la operación o estado inesperado.
        """
        start_time = time.monotonic()

        while True:
            if cancel_event is not None and cancel_event():
                raise KeyboardInterrupt("Cancelada por el usuario")

            elapsed = time.monotonic() - start_time
            if elapsed >= poll_timeout:
                raise TimeoutError(
                    f"Operación no finalizó tras {poll_timeout}s de espera"
                )

            state_data = self.ask_operation(operation_id)
            state = state_data.get("state", "")

            if state == "F":
                with_error = str(state_data.get("withError", "false")).lower()
                if with_error == "true":
                    raise CashDroError(-99, "Operación finalizó con error físico (atasco, etc.)")
                return state_data

            # Estados válidos en progreso
            if state not in ("I", "Q", "E", ""):
                raise CashDroError(-99, f"Estado inesperado '{state}'")

            time.sleep(poll_interval)

    # ─── Flujo completo de venta ────────────────────────────────

    def execute_sale(
        self,
        amount_euros: float,
        posid: str = POS_ID,
        posuser: str = POS_USER,
        aliasid: str = ALIAS_ID,
        cancel_event: Optional[Callable[[], bool]] = None,
    ) -> dict[str, Any]:
        """
        Ejecuta una venta completa.

        Returns:
            {
                "operationId": str,
                "total": float (€),
                "totalIn": float (€),
                "totalOut": float (€),
                "withError": str ("true"/"false"),
                "devices": list,
                "raw": dict  # estado completo sin procesar
            }
        """
        op_id = self.start_operation(op_type=4, amount=amount_euros,
                                     posid=posid, posuser=posuser, aliasid=aliasid)
        self.acknowledge_operation(op_id)
        state_data = self.wait_for_completion(op_id, cancel_event=cancel_event)
        self.set_operation_imported(op_id)

        def _cents(val: str) -> float:
            try:
                return int(val) / 100.0
            except (ValueError, TypeError):
                return 0.0

        return {
            "operationId": op_id,
            "total": _cents(state_data.get("total", "0")),
            "totalIn": _cents(state_data.get("totalin", "0")),
            "totalOut": _cents(state_data.get("totalout", "0")),
            "withError": state_data.get("withError", "false"),
            "devices": state_data.get("devices", []),
            "raw": state_data,
        }


# ─── Ejemplo de uso ─────────────────────────────────────────────

if __name__ == "__main__":
    api = CashDroAPI(host="192.168.1.252", user="manager", password="2")

    # Verificar conexión
    print("Conectando...", api.test_connection())

    # Venta de 18.25 €
    result = api.execute_sale(18.25)
    print(f"Venta completada. ID: {result['operationId']}")
    print(f"  Ingresado: {result['totalIn']:.2f} €")
    print(f"  Cambio:    {result['totalOut']:.2f} €")

    # Estado del equipo
    print("Niveles:", api.ask_levels())
    print("Alertas:", api.ask_alerts())
    print("Diagnóstico:", api.ask_diagnosis())
```

---

## PATRONES DE IMPLEMENTACIÓN (BUENAS PRÁCTICAS)

### 1. Manejo de Ctrl+C con cancel_event

```python
import signal

_cancel_requested = False

def _signal_handler(sig, frame):
    global _cancel_requested
    if not _cancel_requested:
        _cancel_requested = True  # primer Ctrl+C → cancelar operación
    else:
        sys.exit(0)               # segundo Ctrl+C → salir forzoso

signal.signal(signal.SIGINT, _signal_handler)

# Pasar como cancel_event:
result = api.wait_for_completion(op_id, cancel_event=lambda: _cancel_requested)
```

### 2. Estrategia de reintentos

| Escenario                        | Reintentos | Espera entre reintentos |
|----------------------------------|------------|-------------------------|
| Fallo de conexión (ConnectionError, Timeout) | 3          | 1 segundo               |
| Código -1900 (System busy)       | 3 extras   | 5 segundos              |
| Otros errores de aplicación      | 0 (falla inmediato) | —              |

### 3. Normalización de anidamiento

Siempre aplicar este patrón al extraer de `response`:
```python
def _unwrap(data, key):
    val = data.get(key, {})
    if isinstance(val, dict) and key in val:
        val = val[key]
    return val
```

### 4. Conversión de céntimos a euros

```python
def cents_to_euro(valor: str) -> float:
    try:
        return int(valor) / 100.0
    except (ValueError, TypeError):
        return 0.0
```

---

## HOMOLOGACIÓN

Para certificar la integración (nivel básico):

1. **Venta completa** — flujo de 4 pasos funcionando correctamente.
2. **Cancelar venta** — durante el pago, cancelar con `finishOperation(type=2)`.
3. **Redondeo** — gestionar `amountchangenotavailable` a favor del cliente. El redondeo lo configura el técnico en CashDro; no se consulta desde el Host.
4. **Control de cambio y cancelación activos siempre.**

Verificación: arqueo Z en el TPV vs resumen de operaciones en CashDro. Deben coincidir.

---

## DESPLIEGUE TÍPICO

```
┌─────────────┐  SSH + sshpass   ┌──────────────────────┐  Ethernet  ┌──────────┐
│ Linux/Mac   │ ───────────────► │ Windows 10 (CKL120)  │ ─────────► │ CashDro  │
│ desarrollo  │                  │ python-3.12.5/       │   HTTPS    │  .252    │
│             │                  │ CashDroController/   │            │          │
└─────────────┘                  └──────────────────────┘            └──────────┘
```

Script de lanzamiento remoto (`cashdro.sh`):
```bash
#!/bin/bash
sshpass -p '<password>' ssh -t -o StrictHostKeyChecking=no \
  sshuser@192.168.1.34 \
  "C:\Users\sshuser\python-3.12.5\python.exe C:\Users\sshuser\CashDroController\main.py"
```

**Dependencias:** solo `requests>=2.25.0`. Python 3.8+.

**Configuración de red Windows:**
- IP: `192.168.1.34` (misma subred que CashDro)
- Máscara: `255.255.255.0`
- Gateway/DNS: vacío (red aislada)

---

## CUIDADOS Y RECOMENDACIONES (CHECKLIST)

1. [ ] **Actualizar CashDro** a la última versión oficial antes de integrar.
2. [ ] **amount en céntimos** — multiplicar ×100, redondear con `int(round())`, enviar como string.
3. [ ] **Siempre `setOperationImported`** al finalizar. Si no, la operación queda pendiente.
4. [ ] **Normalizar anidamiento doble** en TODAS las respuestas.
5. [ ] **Manejar -1900 con reintentos** — CashDro puede estar cargando al iniciar.
6. [ ] **Polling cada 2s, timeout 120s** — el cliente puede tardar insertando efectivo.
7. [ ] **Verificar `withError`** cuando state="F" — puede haber finalizado con atasco.
8. [ ] **Validar estados en polling** — solo I, Q, E, "" son válidos en progreso.
9. [ ] **El usuario CashDro (`name`) ≠ `posuser`** — el primero necesita permisos en CashDro.
10. [ ] **SSL autofirmado** — `verify=False` + `urllib3.disable_warnings()`.
11. [ ] **Credenciales en GET** — no loguear URLs completas en producción.
12. [ ] **Solo una transacción a la vez** — si `code=-2`, reintentar.
13. [ ] **Cable de red directo o switch** — CashDro no usa USB/serie.
14. [ ] **`change` vs `totalout`** — la respuesta puede incluir ambos. `totalout` es lo dispensado; `change` puede ser un resumen.
15. [ ] **Monto máximo 9999.99€** — validar antes de enviar.
