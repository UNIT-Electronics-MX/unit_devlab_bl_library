import {
  createContext,
  useCallback,
  useContext,
  useMemo,
  useRef,
  useState,
  type PropsWithChildren,
} from "react"

import type { Message } from "@models/message"
import { PulsarC6BLE } from "@services/bluetooth/PulsarBLE"

type LogType = "info" | "ok" | "warn" | "error"

export interface BluetoothLog {
  id: number
  message: string
  type: LogType
  timestamp: Date
}

export interface PomodoroState {
  remainingSeconds: number
  mode: "work" | "break"
  running: boolean
}

interface BluetoothContextValue {
  connected: boolean
  connecting: boolean
  deviceName: string | null

  currentEmotion: string | null
  currentAction: string | null
  motionAutoEnabled: boolean

  pomodoro: PomodoroState
  messages: Message[]
  logs: BluetoothLog[]

  connect: () => Promise<void>
  disconnect: () => void

  sendCommand: (command: string) => Promise<boolean>
  sendMessage: (text: string) => Promise<boolean>
  toggleMotionAuto: () => Promise<void>

  clearMessages: () => void
  clearLogs: () => void
}

const BluetoothContext = createContext<BluetoothContextValue | undefined>(
  undefined
)

const DEFAULT_POMODORO: PomodoroState = {
  remainingSeconds: 25 * 60,
  mode: "work",
  running: false,
}

export function BluetoothProvider({ children }: PropsWithChildren) {
  const robotRef = useRef<PulsarC6BLE | null>(null)

  const [connected, setConnected] = useState(false)
  const [connecting, setConnecting] = useState(false)
  const [deviceName, setDeviceName] = useState<string | null>(null)

  const [currentEmotion, setCurrentEmotion] = useState<string | null>(null)
  const [currentAction, setCurrentAction] = useState<string | null>(null)
  const [motionAutoEnabled, setMotionAutoEnabled] = useState(false)

  const [pomodoro, setPomodoro] = useState<PomodoroState>(DEFAULT_POMODORO)
  const [messages, setMessages] = useState<Message[]>([])
  const [logs, setLogs] = useState<BluetoothLog[]>([])

  const addLog = useCallback((message: string, type: LogType = "info") => {
    setLogs((previous) => {
      const next = [
        ...previous,
        {
          id: Date.now(),
          message,
          type,
          timestamp: new Date(),
        },
      ]

      return next.slice(-60)
    })
  }, [])

  const clearLogs = useCallback(() => {
    setLogs([])
  }, [])

  const addMessage = useCallback((text: string, type: Message["type"]) => {
    const newMessage: Message = {
      id: crypto.randomUUID(),
      text,
      type,
      timestamp: new Date(),
    }

    setMessages((current) => [...current, newMessage].slice(-20))
  }, [])

  const clearMessages = useCallback(() => {
    setMessages([])
  }, [])

  const resetState = useCallback(() => {
    setConnected(false)
    setDeviceName(null)
    setCurrentEmotion(null)
    setCurrentAction(null)
    setMotionAutoEnabled(false)
    setPomodoro(DEFAULT_POMODORO)
  }, [])

  const handlePomodoroNotification = useCallback(
    (payload: string) => {
      const parts = payload.split("|")

      if (parts.length < 3) {
        addLog(`Payload Pomodoro inválido: ${payload}`, "warn")
        return
      }

      const remainingSeconds = Math.max(
        0,
        Number.parseInt(parts[0], 10) || 0
      )

      const mode: "work" | "break" =
        parts[1] === "break" ? "break" : "work"

      const running = parts[2] === "running"

      setPomodoro({
        remainingSeconds,
        mode,
        running,
      })
    },
    [addLog]
  )

  const handleNotification = useCallback(
    (data: DataView) => {
      const message = new TextDecoder().decode(data)

      console.log("BLE RX:", message)
      addLog(`RX: ${message}`, "info")

      if (message.startsWith("pomodoro:")) {
        handlePomodoroNotification(message.substring(9))
        return
      }

      if (message === "motion:auto:on") {
        setMotionAutoEnabled(true)
        setCurrentAction("auto imu")
        addLog("Auto IMU activado", "ok")
        return
      }

      if (message === "motion:auto:off") {
        setMotionAutoEnabled(false)
        setCurrentAction(null)
        addLog("Auto IMU desactivado", "info")
        return
      }

      if (message.startsWith("motion:emotion:")) {
        const emotion = message.substring(15)
        setCurrentEmotion(emotion)
        return
      }

      if (message === "rtc:sync_needed") {
        addLog("RTC requiere sincronización", "warn")
        return
      }

      if (message === "rtc:ok") {
        addLog("RTC sincronizado", "ok")
        return
      }

      if (message.startsWith("msg:")) {
        const text = message.substring(4)

        addMessage(text, "received")
        addLog(`Mensaje recibido: ${text}`, "info")
        return
      }

      addLog(`Notificación no reconocida: ${message}`, "warn")
    },
    [addLog, addMessage, handlePomodoroNotification]
  )

  const initializeRobot = useCallback(() => {
    if (robotRef.current) return robotRef.current

    const robot = new PulsarC6BLE({
      onLog: (message) => {
        addLog(message, "info")
      },

      onConnected: () => {
        const status = robot.getStatus()

        setConnected(true)
        setDeviceName(status.deviceName ?? "Pulsar C6")

        addLog(
          `Conectado a ${status.deviceName ?? "Pulsar C6"}`,
          "ok"
        )
      },

      onDisconnected: () => {
        resetState()
        addLog("Dispositivo desconectado", "warn")
      },

      onError: (message) => {
        addLog(message, "error")
      },
    })

    robotRef.current = robot

    return robot
  }, [addLog, resetState])

  const connect = useCallback(async () => {
    if (connecting || connected) return

    setConnecting(true)
    addLog("Buscando dispositivo BLE...", "info")

    try {
      const robot = initializeRobot()
      const success = await robot.connect()

      if (!success) {
        addLog("No fue posible establecer la conexión", "error")
        return
      }

      const status = robot.getStatus()

      setConnected(true)
      setDeviceName(status.deviceName ?? "Pulsar C6")

      const subscribed = await robot.subscribeToNotifications(
        handleNotification
      )

      if (!subscribed) {
        addLog("No fue posible suscribirse a notificaciones BLE", "warn")
      }

      await robot.sendCommand("rtc:status")
      await robot.sendCommand("pomodoro:status")
      await robot.sendCommand("motion:auto:status")

      addLog("Conexión BLE inicializada correctamente", "ok")
    } catch (error) {
      const message =
        error instanceof Error ? error.message : "Error desconocido"

      addLog(`Error BLE: ${message}`, "error")
      resetState()
    } finally {
      setConnecting(false)
    }
  }, [
    addLog,
    connected,
    connecting,
    handleNotification,
    initializeRobot,
    resetState,
  ])

  const disconnect = useCallback(() => {
    const robot = robotRef.current

    if (!robot) return

    robot.disconnect()
    resetState()

    addLog("Desconectando dispositivo...", "info")
  }, [addLog, resetState])

  const sendCommand = useCallback(
    async (command: string): Promise<boolean> => {
      const robot = robotRef.current

      if (!robot || !connected) {
        addLog("No hay conexión BLE activa", "error")
        return false
      }

      try {
        const success = await robot.sendCommand(command)

        if (success) {
          addLog(`TX: ${command}`, "info")
        } else {
          addLog(`Error al enviar: ${command}`, "error")
        }

        return success
      } catch (error) {
        const message =
          error instanceof Error ? error.message : "Error desconocido"

        addLog(`Error enviando comando: ${message}`, "error")
        return false
      }
    },
    [addLog, connected]
  )

  const sendMessage = useCallback(
    async (text: string): Promise<boolean> => {
      const cleanMessage = text.trim()

      if (!cleanMessage) return false

      const success = await sendCommand(`msg:${cleanMessage}`)

      if (success) {
        addMessage(cleanMessage, "sent")
      }

      return success
    },
    [addMessage, sendCommand]
  )

  const toggleMotionAuto = useCallback(async () => {
    const command = motionAutoEnabled
      ? "motion:auto:off"
      : "motion:auto:on"

    await sendCommand(command)
  }, [motionAutoEnabled, sendCommand])

  const value = useMemo<BluetoothContextValue>(
    () => ({
      connected,
      connecting,
      deviceName,

      currentEmotion,
      currentAction,
      motionAutoEnabled,

      pomodoro,
      messages,
      logs,

      connect,
      disconnect,

      sendCommand,
      sendMessage,
      toggleMotionAuto,

      clearMessages,
      clearLogs,
    }),
    [
      connected,
      connecting,
      deviceName,

      currentEmotion,
      currentAction,
      motionAutoEnabled,

      pomodoro,
      messages,
      logs,

      connect,
      disconnect,

      sendCommand,
      sendMessage,
      toggleMotionAuto,

      clearMessages,
      clearLogs,
    ]
  )

  return (
    <BluetoothContext.Provider value={value}>
      {children}
    </BluetoothContext.Provider>
  )
}

export function useBluetooth() {
  const context = useContext(BluetoothContext)

  if (!context) {
    throw new Error(
      "useBluetooth debe utilizarse dentro de BluetoothProvider"
    )
  }

  return context
}