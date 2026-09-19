import {
    Bluetooth,
    LoaderCircle,
    Unplug,
} from "lucide-react"

import { Button } from "@shadcn/button"

import { ConnectionStatus } from "@components/header/ConnectionStatus"

import { useBluetooth } from "@features/bluetooth/BluetoothProvider"

export const Header = () => {
    const {
        connected,
        connecting,
        deviceName,
        connect,
        disconnect,
    } = useBluetooth()

    return (
        <header className="sticky top-0 z-50 bg-background/95">
            <div className="mx-auto flex h-16 w-full max-w-5xl items-center justify-between gap-4 px-4">

                <h1 className="truncate text-4xl font-peralta leading-tight">
                    Kubi
                </h1>


                {/* Connection */}
                <div className="flex items-center gap-4">
                    <ConnectionStatus
                        connected={connected}
                        connecting={connecting}
                        deviceName={deviceName}
                    />

                    {!connected ? (
                        <Button
                            type="button"
                            onClick={connect}
                            disabled={connecting}
                            className="gap-2 bg-blue-800 hover:bg-blue-700"
                        >
                            {connecting ? (
                                <LoaderCircle className="size-4 animate-spin" />
                            ) : (
                                <Bluetooth className="size-4" />
                            )}

                            <span className="hidden sm:inline">
                                {connecting
                                    ? "Conectando..."
                                    : "Conectar"}
                            </span>
                        </Button>
                    ) : (
                        <Button
                            type="button"
                            variant="outline"
                            onClick={disconnect}
                            className="gap-2 text-destructive hover:text-destructive"
                        >
                            <Unplug className="size-4" />

                            <span className="hidden sm:inline">
                                Desconectar
                            </span>
                        </Button>
                    )}
                </div>
            </div>
        </header>
    )
}