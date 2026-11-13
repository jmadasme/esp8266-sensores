// ═══════════════════════════════════════════════════════════════════
//  secrets.example.h — PLANTILLA DE CONFIGURACIÓN
//
//  Copiá este archivo como secrets.h y completá con tus valores reales:
//
//      cp secrets.example.h secrets.h
//
//  secrets.h está en .gitignore y NO se versiona, para no publicar
//  credenciales. Los scripts compilar.sh y subir.sh avisan si falta.
// ═══════════════════════════════════════════════════════════════════

#ifndef SECRETS_H
#define SECRETS_H

// ── Broker MQTT ───────────────────────────────────────────────────
#define MQTT_BROKER   "www.smartage.cl"
#define MQTT_PORT     1886

// Usuario y contraseña del broker.
#define MQTT_USERNAME "TU_USUARIO"
#define MQTT_PASSWORD "TU_PASSWORD"

// ClientId. IMPORTANTE: este broker exige un ClientId REGISTRADO además
// del usuario y la contraseña. Con un ClientId cualquiera la respuesta
// es CONNACK 5 "not authorised" aunque las credenciales sean correctas.
#define MQTT_CLIENT   "TU_CLIENT_ID"

// ── Red WiFi ──────────────────────────────────────────────────────
#define WIFI_SSID     "TU_RED_WIFI"
#define WIFI_PASSWORD "TU_PASSWORD_WIFI"

#endif  // SECRETS_H
