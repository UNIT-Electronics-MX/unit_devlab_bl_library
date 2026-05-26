/**
 * Pulsar C6 BLE Control Module
 * 
 * Módulo reutilizable para controlar Pulsar C6 via Web Bluetooth
 * desde cualquier página HTML.
 * 
 * Uso:
 *   const robot = new PulsarC6BLE();
 *   await robot.connect();
 *   await robot.sendCommand('happy');
 */

class PulsarC6BLE {
  constructor(config = {}) {
    // Configuración de UUIDs (por defecto para Pulsar C6)
    this.SERVICE_UUID = config.serviceUUID || '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
    this.CHARACTERISTIC_UUID = config.characteristicUUID || 'beb5483e-36e1-4688-b7f5-ea07361b26a8';
    this.DEVICE_NAME = config.deviceName || 'PulsarC6_BLE';

    // Estado interno
    this.device = null;
    this.server = null;
    this.service = null;
    this.characteristic = null;
    this.isConnected = false;

    // BLE operation lock to prevent simultaneous operations
    this.operationInProgress = false;
    this.commandQueue = [];
    this.processingQueue = false;

    // Callbacks opcionales
    this.onConnected = config.onConnected || (() => {});
    this.onDisconnected = config.onDisconnected || (() => {});
    this.onError = config.onError || (() => {});
    this.onLog = config.onLog || (() => {});

    // Validar soporte
    if (!navigator.bluetooth) {
      this.onError('Web Bluetooth no soportado en este navegador');
    }
  }

  // Logging interno
  log(message) {
    const timestamp = new Date().toLocaleTimeString();
    this.onLog(`[${timestamp}] ${message}`);
  }

  // Conectar al dispositivo BLE
  async connect() {
    try {
      this.log('Buscando dispositivo...');
      
      this.device = await navigator.bluetooth.requestDevice({
        filters: [{ services: [this.SERVICE_UUID] }],
        optionalServices: [this.SERVICE_UUID]
      });

      this.log(`Dispositivo encontrado: ${this.device.name || this.DEVICE_NAME}`);

      this.device.addEventListener('gattserverdisconnected', () => {
        this.onDisconnected();
        this.isConnected = false;
      });

      this.server = await this.device.gatt.connect();
      this.log('GATT conectado');

      this.service = await this.server.getPrimaryService(this.SERVICE_UUID);
      this.log('Servicio primario encontrado');

      this.characteristic = await this.service.getCharacteristic(this.CHARACTERISTIC_UUID);
      this.log('Característica found');

      this.isConnected = true;
      this.onConnected();
      this.log('✅ Conectado');

      return true;
    } catch (error) {
      this.isConnected = false;
      const msg = error.name === 'NotFoundError' 
        ? 'Dispositivo no encontrado'
        : error.message;
      this.onError(msg);
      this.log(`❌ Error: ${msg}`);
      return false;
    }
  }

  // Subscribe to notifications
  async subscribeToNotifications(callback) {
    if (!this.characteristic || !this.isConnected) {
      this.onError('No conectado');
      return false;
    }

    try {
      await this.characteristic.startNotifications();
      this.characteristic.addEventListener('characteristicvaluechanged', (event) => {
        const value = event.target.value;
        callback(value);
      });
      this.log('✅ Notificaciones activadas');
      return true;
    } catch (error) {
      this.onError(`Error al suscribir: ${error.message}`);
      this.log(`❌ Error: ${error.message}`);
      return false;
    }
  }

  // Desconectar
  disconnect() {
    if (this.device && this.device.gatt.connected) {
      this.device.gatt.disconnect();
      this.isConnected = false;
      this.log('Desconectado');
    }
  }

  // Enviar comando
  async sendCommand(command) {
    if (!this.characteristic || !this.isConnected) {
      this.onError('No conectado');
      return false;
    }

    // Add to queue instead of sending directly
    return new Promise((resolve) => {
      this.commandQueue.push({ command, resolve });
      this.processQueue();
    });
  }

  // Process command queue sequentially
  async processQueue() {
    if (this.processingQueue || this.commandQueue.length === 0) {
      return;
    }

    this.processingQueue = true;

    while (this.commandQueue.length > 0) {
      const { command, resolve } = this.commandQueue.shift();
      
      // Wait for any ongoing operation to complete
      while (this.operationInProgress) {
        await new Promise(r => setTimeout(r, 30));
      }

      this.operationInProgress = true;

      try {
        const encoder = new TextEncoder();
        await this.characteristic.writeValue(encoder.encode(command));
        this.log(`📤 Enviado: ${command}`);
        resolve(true);
      } catch (error) {
        this.onError(`Error al enviar: ${error.message}`);
        this.log(`❌ Error: ${error.message}`);
        resolve(false);
      } finally {
        this.operationInProgress = false;
        // Small delay between operations to prevent GATT conflicts
        await new Promise(r => setTimeout(r, 50));
      }
    }

    this.processingQueue = false;
  }

  // Enviar múltiples comandos en secuencia
  async sendCommands(commands, delayMs = 500) {
    for (const cmd of commands) {
      const ok = await this.sendCommand(cmd);
      if (!ok) return false;
      if (delayMs > 0) {
        await new Promise(r => setTimeout(r, delayMs));
      }
    }
    return true;
  }

  // Estado conexión
  getStatus() {
    return {
      isConnected: this.isConnected,
      deviceName: this.device?.name || null,
      serviceUUID: this.SERVICE_UUID,
      characteristicUUID: this.CHARACTERISTIC_UUID
    };
  }

  // Validadores de comando
  static VALID_EMOTIONS = ['happy', 'sleepy', 'surprised', 'wink','vomit', 'angry'];
  static VALID_ACTIONS = ['standup', 'sit', 'sleep', 'wave', 'walk', 'stop'];
  static VALID_COMMANDS = [...this.VALID_EMOTIONS, ...this.VALID_ACTIONS];

  static isValidCommand(cmd) {
    return this.VALID_COMMANDS.includes(cmd.toLowerCase());
  }

  static isEmotion(cmd) {
    return this.VALID_EMOTIONS.includes(cmd.toLowerCase());
  }

  static isAction(cmd) {
    return this.VALID_ACTIONS.includes(cmd.toLowerCase());
  }
}

// Exportar para módulos
if (typeof module !== 'undefined' && module.exports) {
  module.exports = PulsarC6BLE;
}
