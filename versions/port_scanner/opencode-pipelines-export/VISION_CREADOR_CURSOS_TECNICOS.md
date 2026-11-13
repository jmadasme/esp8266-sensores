# VISIÓN: Creador de Cursos Técnicos Automatizados

## Idea General

Sistema multi-agente autónomo que **diariamente** genera un mini-proyecto técnico completo y funcional basado en un componente real de Aliexpress, asignado a una plataforma/placa específica (ESP32, Arduino Uno Q, Raspberry Pi 5, Raspberry Pi Pico W, Teltonika CALYX).

## Flujo de trabajo diario

```
Input del día: URL Aliexpress + Plataforma asignada

  ↓

[SCRAPER]   → Extrae specs, precio, imágenes del producto
  ↓

[ARQUITECTO] → Diseña el mini-proyecto del día (qué vamos a construir)
  ↓

[IMPLEMENTADOR] → Genera código listo para copiar/pegar (.ino, .py, etc.)
  ↓

[DIAGRAMADOR] → Crea diagrama eléctrico/esquemático (Mermaid / ASCII)
  ↓

[DOCUMENTADOR] → Redacta tutorial completo + PDF descargable
  ↓

[PUBLICADOR] → Distribuye a Telegram, GitHub, Reddit, LinkedIn
```

## Rotación semanal sugerida

| Día       | Plataforma         | Ejemplo de componente         |
|-----------|---------------------|-------------------------------|
| Lunes     | ESP32               | Sensor + pantalla OLED        |
| Martes    | Arduino Uno Q       | Módulo relé 2 canales         |
| Miércoles | Raspberry Pi 5      | Cámara HQ / sensor            |
| Jueves    | Raspberry Pi Pico W | Sensor distancia HC-SR04      |
| Viernes   | Teltonika CALYX     | Expansor I/O / RS485          |
| Sábado    | Libre / wildcard    | Cualquier placa               |
| Domingo   | ESP32               | Proyecto integrador semanal   |

## Componentes del ecosistema multi-agente

### 1. Agente Scraper
- Busca/completa specs del producto desde URL Aliexpress
- Extrae: nombre, precio, descripción, imágenes
- Guarda datos estructurados

### 2. Agente Arquitecto de Proyecto
- Diseña qué proyecto construir con ese componente + plataforma
- Define: objetivos, conexiones, funcionalidad
- Crea plan técnico breve

### 3. Agente Implementador (similar al actual)
- Escribe código funcional y completo
- Comprobable / compilable directamente
- Adaptado a la plataforma destino

### 4. Agente Diagramador
- Genera diagrama de conexiones eléctricas
- Formato compatible con Mermaid o similar
- Incluye leyenda de pines

### 5. Agente Documentador
- Redacta tutorial paso a paso
- Genera PDF descargable
- Incluye: introducción, materiales, conexiones, código, explicación

### 6. Agente Publicador
- Publica en Telegram (canal de cursos)
- Publica en GitHub (repositorio con código fuente)
- Publica en Reddit / LinkedIn (resumen + enlace)

### 7. Orquestrador
- Ejecuta el pipeline diario vía cron
- Controla rotación de plataformas
- Gestiona logs y estado

## Tecnologías / Herramientas

- **OpenCode** como motor de agentes (CLI headless + `opencode run`)
- **Cron** para programación diaria
- **Plan Free** (DeepSeek V4 Flash) para ejecución diaria
- **Telegram Bot API** para canal de salida principal
- **YouTube Data API v3** para investigación (solo lectura)
- **Twilio / WhatsApp Business API** como alternativa de salida
- **Mermaid** para diagramas eléctricos
- **Markdown + conversión ligera** para PDF descargable

## Notas importantes

- Instagram y TikTok no son viables para automatización (APIs cerradas)
- YouTube solo como fuente de investigación, no para publicación (requiere video)
- El plan Free alcanza perfectamente para una ejecución diaria
- El código generado debe ser directamente usable (compilable/cargable)

## Próximos pasos a definir

1. Canal principal de publicación (Telegram, GitHub Pages, blog)
2. Formato del PDF (pandoc, markdown simple, u otro)
3. Fuente de entrada: URL manual vs scraper automático de Aliexpress
4. Rotación exacta de plataformas y días
5. Primer MVP: probar con un solo día y una sola plataforma
