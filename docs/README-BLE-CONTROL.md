# Pulsar C6 - Control por Web Bluetooth

Esta página permite controlar el **Pulsar C6 Pet Robot** directamente desde tu navegador usando **Web Bluetooth API**.

## 🎯 Requisitos

1. **Navegador compatible**: Chrome, Edge, Opera o cualquier navegador Chromium que soporte Web Bluetooth
2. **Pulsar C6 con firmware BLE**: Carga el sketch `mk3_ble_web_hybrid.ino` en tu ESP32-C6
3. **Conexión segura**: La página debe servirse por HTTPS o ser `localhost`

## 📱 Compatibilidad

| Navegador | Soporte | Notas |
|-----------|---------|-------|
| Chrome/Chromium | ✅ Completo | Versión 56+ |
| Edge | ✅ Completo | Basado en Chromium |
| Opera | ✅ Completo | Basado en Chromium |
| Firefox | ❌ No | No soporta Web Bluetooth |
| Safari | ⚠️ Limitado | iOS 13+, solo dispositivos Apple |

## 🔌 Conectar el Robot

1. Abre [pulsar-c6-control.html](./pulsar-c6-control.html)
2. Asegúrate de que tu **Pulsar C6 está encendido** y tiene el firmware `mk3_ble_web_hybrid` cargado
3. Haz clic en **"Conectar BLE"**
4. Selecciona `PulsarC6_BLE` de la lista de dispositivos disponibles
5. Autoriza la conexión cuando te lo pida el navegador

## 🎭 Comandos Disponibles

### Emociones
- **Happy** (😊): Cara feliz
- **Sleepy** (😴): Cara soñolienta
- **Surprised** (😲): Cara sorprendida
- **Wink** (😉): Guiño
- **Angry** (😠): Cara enojada

### Acciones
- **Stand Up** (📍): Pose neutral en pie
- **Sit Down** (🪑): Sentarse
- **Sleep** (🛏️): Acostarse a dormir
- **Wave** (👋): Saludar con la pata
- **Walk** (🚶): Caminar
- **Stop** (⏹️): Detener

## 🔧 Especificaciones Técnicas

### UUIDs BLE
- **Service UUID**: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **Characteristic UUID**: `beb5483e-36e1-4688-b7f5-ea07361b26a8`

### Protocolo
- Comandos enviados como **texto UTF-8** sin terminador
- Ejemplos válidos: `happy`, `walk`, `stop`
- No distingue mayúsculas/minúsculas (case-insensitive)

## ⚙️ Configuración en el Hardware

El firmware del robot está preconfigurado con estos UUIDs, pero si necesitas cambiarlos:

**En `mk3_ble_web_hybrid.ino`:**
```cpp
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
```

## 📊 Log de Actividad

La página mantiene un log en tiempo real de todas las acciones:
- Intentos de conexión
- Comandos enviados
- Errores y eventos

Usa el botón **"Limpiar"** para limpiar el log.

## 🐛 Solución de Problemas

### No aparece el dispositivo
1. Verifica que el Pulsar C6 está alimentado
2. Verifica que tiene cargado el firmware `mk3_ble_web_hybrid.ino`
3. Intenta apagar/encender el robot

### Conexión se cae
1. Aléjate menos de 10 metros del robot
2. Elimina interferencias WiFi
3. Intenta reconectar

### Los comandos no llegan
1. Asegúrate de que la barra de conexión muestra **"Conectado" en verde**
2. Revisa el log de actividad
3. Intenta desconectar y reconectar

## 📝 Desarrollo

Para integrar la página en otro lugar:
1. Copia `pulsar-c6-control.html` a tu servidor
2. Asegúrate de que el servidor use **HTTPS**
3. Los UUIDs deben coincidir con el firmware del robot

## 📄 Licencia

UNIT Electronics - Proyectos de Hardware Abierto
