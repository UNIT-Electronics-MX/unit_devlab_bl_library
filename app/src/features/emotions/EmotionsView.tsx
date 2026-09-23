import { Angry, Check, Cloud, Compass, Eye, LoaderCircle, Moon, Smile, Sparkles } from "lucide-react"
import { useEffect, useState } from "react"

import { Button } from "@shadcn/button"
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@shadcn/card"
import { useBluetooth } from "@features/bluetooth/BluetoothProvider"

const EMOTIONS = [
    { command: "happy", label: "Feliz", icon: Smile },
    { command: "sleepy", label: "Dormir", icon: Moon },
    { command: "surprised", label: "Sorpresa", icon: Sparkles },
    { command: "wink", label: "Guiño", icon: Eye },
    { command: "vomit", label: "Mareado", icon: Cloud },
    { command: "angry", label: "Enojo", icon: Angry },
] as const

type EmotionCommand = typeof EMOTIONS[number]["command"]

const isEmotionCommand = (value: string): value is EmotionCommand =>
    EMOTIONS.some(({ command }) => command === value)

export const EmotionsView = () => {
    const { connected, currentEmotion, motionAutoEnabled, sendCommand, toggleMotionAuto } = useBluetooth()
    const [selectedEmotion, setSelectedEmotion] = useState<EmotionCommand | null>(null)
    const [pendingCommand, setPendingCommand] = useState<string | null>(null)

    useEffect(() => {
        if (currentEmotion && isEmotionCommand(currentEmotion)) setSelectedEmotion(currentEmotion)
    }, [currentEmotion])

    const handleEmotion = async (command: EmotionCommand) => {
        if (!connected || pendingCommand) return

        setPendingCommand(command)

        try {
            const success = await sendCommand(command)
            if (success) setSelectedEmotion(command)
        } finally {
            setPendingCommand(null)
        }
    }

    const handleMotionAuto = async () => {
        if (!connected || pendingCommand) return

        setPendingCommand("motion-auto")

        try {
            await toggleMotionAuto()
        } finally {
            setPendingCommand(null)
        }
    }

    return (
        <Card>
            <CardHeader>
                <CardTitle>Emociones</CardTitle>
                <CardDescription>Selecciona una expresión para mostrar en Kubi.</CardDescription>
            </CardHeader>

            <CardContent>
                <div className="grid grid-cols-2 gap-3 sm:grid-cols-3 lg:grid-cols-4">
                    {EMOTIONS.map(({ command, label, icon: Icon }) => {
                        const isActive = selectedEmotion === command
                        const isLoading = pendingCommand === command

                        return (
                            <Button
                                key={command}
                                type="button"
                                variant="outline"
                                disabled={!connected || pendingCommand !== null}
                                aria-pressed={isActive}
                                onClick={() => handleEmotion(command)}
                                className={`h-28 flex-col gap-3 rounded-xl transition-all font-arcade ${isActive
                                    ? "border-blue-600 border-b-4 bg-blue-400 text-white hover:bg-blue-600 hover:text-white"
                                    : "hover:border-blue-300 hover:bg-white dark:hover:bg-blue-950/30"
                                    }`}
                            >
                                {isLoading ? <LoaderCircle className="size-6 animate-spin" /> : <Icon className="size-6" />}
                                <span>{label}</span>
                            </Button>
                        )
                    })}

                    <Button
                        type="button"
                        variant="outline"
                        disabled={!connected || pendingCommand !== null}
                        aria-pressed={motionAutoEnabled}
                        onClick={handleMotionAuto}
                        className={`h-28 flex-col gap-3 rounded-xl transition-all font-arcade ${motionAutoEnabled
                            ? "border-blue-600 border-b-4 bg-blue-400 text-white hover:bg-blue-600 hover:text-white"
                            : "hover:border-blue-300 hover:bg-white dark:hover:bg-blue-950/30"
                            }`}
                    >
                        {pendingCommand === "motion-auto"
                            ? <LoaderCircle className="size-6 animate-spin" />
                            : motionAutoEnabled
                                ? <Check className="size-6" />
                                : <Compass className="size-6" />}
                        <span>Auto IMU</span>
                    </Button>
                </div>

                {!connected && (
                    <p className="mt-4 text-center text-sm text-red-500">
                        Conecta a Kubi para habilitar los controles :P
                    </p>
                )}
            </CardContent>
        </Card>
    )
}