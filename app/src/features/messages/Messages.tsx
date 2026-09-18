import { Broom, Send } from "lucide-react"
import { type FormEvent, useState } from "react"

import { Button } from "@shadcn/button"
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@shadcn/card"
import { Input } from "@shadcn/input"
import { ScrollArea } from "@shadcn/scroll-area"

import { useBluetooth } from "@features/bluetooth/BluetoothProvider"
import { MessageBubble } from "./MessageBubble"

import "./messages.css"

export const Messages = () => {
  const { connected, messages, sendMessage, clearMessages } = useBluetooth()

  const [message, setMessage] = useState("")
  const [sending, setSending] = useState(false)

  const handleSubmit = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault()

    const cleanMessage = message.trim()

    if (!cleanMessage || !connected || sending) return

    setSending(true)

    try {
      const success = await sendMessage(cleanMessage)
      if (success) setMessage("")
    } finally {
      setSending(false)
    }
  }

  return (
    <Card>
      <CardHeader>
        <div className="main-messages">
          <div>
            <CardTitle>Mensajes</CardTitle>
            <CardDescription>
              Envía mensajes para mostrarlos en la pantalla OLED de Kubi.
            </CardDescription>
          </div>

          <Button
            type="button"
            variant="ghost"
            size="sm"
            disabled={messages.length === 0}
            onClick={clearMessages}
          >
            <Broom className="size-4" />
            Limpiar
          </Button>
        </div>
      </CardHeader>

      <CardContent className="space-y-4">
        <ScrollArea className="scroll-area-messages">
          {messages.length === 0 ? (
            <p className="text-empty-messages">
              No hay mensajes todavía :(
            </p>
          ) : (
            <div className="space-y-4">
              {messages.map((message) => (
                <MessageBubble
                  key={message.id}
                  message={message}
                />
              ))}
            </div>
          )}
        </ScrollArea>

        <form onSubmit={handleSubmit} className="flex gap-2">
          <Input
            value={message}
            maxLength={100}
            disabled={!connected || sending}
            placeholder={
              connected
                ? "Escribe un mensaje para Kubi..."
                : "Conecta a Kubi para enviar mensajes"
            }
            onChange={(event) => setMessage(event.target.value)}
          />

          <Button
            type="submit"
            disabled={!connected || sending || !message.trim()}
          >
            <Send className="size-4" />

            <span className="hidden sm:inline">
              {sending ? "Enviando..." : "Enviar"}
            </span>
          </Button>
        </form>

        <div className="footer-messages text-muted-foreground">
          <span>
            {connected ? "Kubi conectado" : "Kubi desconectado"}
          </span>

          <span>
            {message.length}/100
          </span>
        </div>
      </CardContent>
    </Card>
  )
}