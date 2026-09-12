export interface PulsarC6BLEOptions {
    onLog?: (message: string) => void
    onConnected?: () => void
    onDisconnected?: () => void
    onError?: (message: string) => void
}

export interface PulsarC6BLEStatus {
    isConnected: boolean
    deviceName: string | null
}

type NotificationCallback = (data: DataView) => void

const SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
const CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

const EMOTIONS = [
    "happy",
    "sleepy",
    "surprised",
    "wink",
    "vomit",
    "angry",
] as const

const ACTIONS = ["motion:auto:on", "motion:auto:off"] as const

export class PulsarC6BLE {
    private device: BluetoothDevice | null = null
    private server: BluetoothRemoteGATTServer | null = null
    private characteristic: BluetoothRemoteGATTCharacteristic | null = null

    private notificationCallback: NotificationCallback | null = null

    private readonly onLog: (message: string) => void
    private readonly onConnected: () => void
    private readonly onDisconnected: () => void
    private readonly onError: (message: string) => void

    constructor(options: PulsarC6BLEOptions = {}) {
        this.onLog = options.onLog ?? (() => { })
        this.onConnected = options.onConnected ?? (() => { })
        this.onDisconnected = options.onDisconnected ?? (() => { })
        this.onError = options.onError ?? (() => { })
    }

    get isConnected(): boolean {
        return Boolean(
            this.device?.gatt?.connected &&
            this.server?.connected &&
            this.characteristic
        )
    }

    public getStatus(): PulsarC6BLEStatus {
        return {
            isConnected: this.isConnected,
            deviceName: this.device?.name ?? null,
        }
    }

    public async connect(): Promise<boolean> {
        if (!this.isBluetoothAvailable()) {
            this.emitError("Web Bluetooth no está disponible en este navegador.")

            return false
        }

        if (!window.isSecureContext) {
            this.emitError( "Web Bluetooth requiere HTTPS o localhost." )

            return false
        }

        if (this.isConnected) {
            this.onLog("El dispositivo ya está conectado.")
            return true
        }

        try {
            this.onLog("Buscando dispositivo BLE...")

            this.device = await navigator.bluetooth.requestDevice({
                filters: [
                    {
                        services: [SERVICE_UUID],
                    },
                ],
                optionalServices: [SERVICE_UUID],
            })

            if (!this.device) {
                this.emitError("No se seleccionó ningún dispositivo.")
                return false
            }

            this.onLog(
                `Dispositivo seleccionado: ${this.device.name ?? "Sin nombre"}`
            )

            this.device.addEventListener(
                "gattserverdisconnected",
                this.handleDisconnected
            )

            if (!this.device.gatt) {
                this.emitError(
                    "El dispositivo no dispone de un servidor GATT."
                )

                return false
            }

            this.onLog("Conectando al servidor GATT...")

            this.server = await this.device.gatt.connect()

            this.onLog("Servidor GATT conectado.")

            const service = await this.server.getPrimaryService(
                SERVICE_UUID
            )

            this.onLog("Servicio BLE encontrado.")

            this.characteristic =
                await service.getCharacteristic(
                    CHARACTERISTIC_UUID
                )

            this.onLog("Característica BLE encontrada.")

            this.onConnected()

            return true
        } catch (error) {
            this.cleanupConnection()

            const message = this.getErrorMessage(error)

            if (message.includes("User cancelled")) {
                this.onLog(
                    "Selección de dispositivo cancelada."
                )
            } else {
                this.emitError(
                    `Error durante la conexión BLE: ${message}`
                )
            }

            return false
        }
    }

    public disconnect(): void {
        if (!this.device) {
            return
        }

        try {
            if (
                this.device.gatt &&
                this.device.gatt.connected
            ) {
                this.onLog("Desconectando dispositivo BLE...")
                this.device.gatt.disconnect()
            } else {
                this.cleanupConnection()
            }
        } catch (error) {
            this.emitError(
                `Error al desconectar: ${this.getErrorMessage(error)}`
            )

            this.cleanupConnection()
        }
    }

    public async sendCommand(
        command: string
    ): Promise<boolean> {
        if (!command.trim()) {
            this.emitError(
                "No se puede enviar un comando vacío."
            )

            return false
        }

        if (
            !this.characteristic ||
            !this.isConnected
        ) {
            this.emitError(
                "No existe una conexión BLE activa."
            )

            return false
        }

        try {
            const encoder = new TextEncoder()
            const data = encoder.encode(command)

            await this.writeCharacteristic(data)

            this.onLog(`TX: ${command}`)

            return true
        } catch (error) {
            this.emitError(
                `Error enviando "${command}": ${this.getErrorMessage(error)}`
            )

            return false
        }
    }

    public async subscribeToNotifications(
        callback: NotificationCallback
    ): Promise<boolean> {
        if (
            !this.characteristic ||
            !this.isConnected
        ) {
            this.emitError(
                "No existe una conexión BLE activa."
            )

            return false
        }

        try {
            this.notificationCallback = callback

            this.characteristic.removeEventListener(
                "characteristicvaluechanged",
                this.handleCharacteristicValueChanged
            )

            this.characteristic.addEventListener(
                "characteristicvaluechanged",
                this.handleCharacteristicValueChanged
            )

            await this.characteristic.startNotifications()

            this.onLog("Notificaciones BLE habilitadas.")

            return true
        } catch (error) {
            this.emitError( `Error activando notificaciones: ${this.getErrorMessage(error)}` )

            return false
        }
    }

    public async unsubscribeFromNotifications(): Promise<void> {
        if (!this.characteristic) {
            return
        }

        try {
            this.characteristic.removeEventListener(
                "characteristicvaluechanged",
                this.handleCharacteristicValueChanged
            )

            if (this.characteristic.properties.notify) {
                await this.characteristic.stopNotifications()
            }

            this.notificationCallback = null

            this.onLog( "Notificaciones BLE deshabilitadas." )

        } catch (error) {
            this.emitError( `Error desactivando notificaciones: ${this.getErrorMessage(error)}` )
        }
    }

    public static isEmotion(
        command: string
    ): boolean {
        return EMOTIONS.includes(
            command as (typeof EMOTIONS)[number]
        )
    }

    public static isAction(
        command: string
    ): boolean {
        return ACTIONS.includes(
            command as (typeof ACTIONS)[number]
        )
    }

    private readonly handleCharacteristicValueChanged = (
        event: Event
    ): void => {
        const characteristic =
            event.target as BluetoothRemoteGATTCharacteristic

        const value = characteristic.value

        if (!value) {
            return
        }

        this.notificationCallback?.(value)
    }

    private readonly handleDisconnected = (): void => {
        const deviceName = this.device?.name ?? "Dispositivo"

        this.onLog( `${deviceName} se ha desconectado.` )

        this.cleanupConnection()

        this.onDisconnected()
    }

    private async writeCharacteristic(
        data: Uint8Array<ArrayBuffer>): Promise<void> {

        const characteristic = this.characteristic

        if (!characteristic) {
            throw new Error( "Característica BLE no disponible." )
        }

        if ( characteristic.properties.writeWithoutResponse ) {
            
            await characteristic.writeValueWithoutResponse( data )

            return
        }

        if ( characteristic.properties.write ) {
            await characteristic.writeValueWithResponse( data )

            return
        }

        throw new Error( "La característica BLE no permite escritura." )
    }

    private cleanupConnection(): void {
        if (this.characteristic) {
            this.characteristic.removeEventListener(
                "characteristicvaluechanged",
                this.handleCharacteristicValueChanged
            )
        }

        if (this.device) {
            this.device.removeEventListener(
                "gattserverdisconnected",
                this.handleDisconnected
            )
        }

        this.notificationCallback = null
        this.characteristic = null
        this.server = null
        this.device = null
    }

    private isBluetoothAvailable(): boolean {
        return (
            typeof navigator !== "undefined" && "bluetooth" in navigator )
    }

    private emitError( message: string ): void {
        this.onError(message)
    }

    private getErrorMessage( error: unknown ): string {
        if (error instanceof Error) {
            return error.message
        }

        return String(error)
    }
}