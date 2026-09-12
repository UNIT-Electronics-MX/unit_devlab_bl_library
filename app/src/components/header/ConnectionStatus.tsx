import { LoaderCircle } from "lucide-react"

interface ConnectionStatusProps {
  connected: boolean
  connecting: boolean
  deviceName: string | null
}

export const ConnectionStatus = ({
  connected,
  connecting,
  deviceName,
}: ConnectionStatusProps) => {
  const statusLabel = connecting
    ? "Conectando..."
    : connected
      ? "Online"
      : "Offline"

  return (
    <div className="flex items-center gap-2">
      {connecting ? (
        <LoaderCircle className="size-3.5 animate-spin text-muted-foreground" />
      ) : (
        <span
          className={[
            "size-2.5 rounded-full",
            connected
              ? "bg-emerald-500"
              : "bg-muted-foreground/50",
          ].join(" ")}
        />
      )}

      <div className="flex flex-col leading-tight">
        <span className="text-sm font-medium">
          {statusLabel}
        </span>

        {connected && deviceName && (
          <span className="max-w-32 truncate text-xs text-muted-foreground">
            {deviceName}
          </span>
        )}
      </div>
    </div>
  )
}