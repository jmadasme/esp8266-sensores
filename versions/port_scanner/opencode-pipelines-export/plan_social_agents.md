# Plan: Agentes de Publicación Multi-Red Social

## Contexto Actual
- Pipeline OpenCode funcionando: analista → implementador → auditor → documentador → publicador (GitHub)
- Repos publicados en GitHub (wanderc137): YL69_SoilMoisture, LCD1602_I2C, DHT22
- Modelo actual: DeepSeek V4 Free (si se agotan créditos, saltar a Pro o MiMo-V2.5-Pro)

## Idea: agente `publicador_social`
Después de que el `publicador` suba a GitHub, un nuevo agente publica automáticamente en múltiples redes sociales:
- Resumen del proyecto README
- Link al repo de GitHub
- Tech highlights / features

## Plataformas viables (con API gratuita)

| Plataforma | API para publicar | Librería | Dificultad | Por qué incluirla |
|-----------|-------------------|----------|------------|-------------------|
| **Discord** | Webhooks | `requests` | Muy fácil | Comunidad directa, notificaciones |
| **Telegram** | Bot API (BotFather) | `python-telegram-bot` | Fácil | Alcance masivo, grupos nicho |
| **Reddit** | PRAW | `praw` | Media | r/arduino, r/electronics, r/esp32 |
| **Dev.to** | API Key | `requests` | Fácil | Comunidad dev, SEO decente |
| **Mastodon** | Token | `mastodon.py` | Fácil | Tech federada, sin algoritmo |
| **Hackster.io** | API propia | `requests` | Media | **Audiencia maker ideal**, proyectos Arduino |
| **Hackaday.io** | API propia | `requests` | Media | Logs técnicos, comunidad leal |
| **Medium** | Token | `medium-sdk` | Media | Artículos divulgación, descubrimiento |
| **LinkedIn** | OAuth2 | `linkedin-api` | Compleja | Red profesional, recruiters |

## Plataformas NO viables
- **Instructables** — No tiene API de escritura
- **Twitter/X** — API de escritura solo con plan pago ($100/mes)

## Stack completo (9 plataformas)
Una vez consolidado el pipeline, el `publicador_social` dispara a todas:

```
GitHub (publicador original)
  └─ publicador_social
       ├─ Discord (webhook)
       ├─ Telegram (bot)
       ├─ Reddit (PRAW)
       ├─ Dev.to (API key)
       ├─ Mastodon (token)
       ├─ Hackster.io
       ├─ Hackaday.io
       ├─ Medium
       └─ LinkedIn
```

## Stack recomendado inicial (3 plataformas)
**Reddit + Dev.to + Mastodon** — cubren comunidad maker, dev y tech federada. Las 3 son gratuitas, APIs sólidas y documentadas. Agregar Hackster, Hackaday, Medium y LinkedIn una vez validado el flujo básico.

## Estado del pipeline actual
- Modelo: DeepSeek V4 Free (switch a Pro o MiMo-V2.5-Pro si se acaban créditos)
- 3 proyectos publicados exitosamente
- Cada proyecto en `proyectos/` con su propio `.git`
- `setup_pipeline.sh` listo para replicar en desktop
- `plan_multimedia.md` listo para Coqui TTS + SadTalker

## Pendiente
- Diseñar y crear el agente `publicador_social` en `opencode.jsonc`
- Crear script Python unificado para publicar en múltiples redes
- Integrar con el pipeline existente
- También: pensar en nuevos proyectos Arduino para alimentar al pipeline
