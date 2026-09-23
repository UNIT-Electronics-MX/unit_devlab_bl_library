import type { Message } from "@models/message"

import "./messages.css"

interface MessageBubbleProps {
    message: Message
}

export const MessageBubble = ({ message }: MessageBubbleProps) => {
    const isSent = message.type === "sent"

    return (
        <div className={`flex items-end gap-2 ${isSent ? "justify-end" : "justify-start"}`}>

            <div className={`mensaje ${isSent ? "enviado" : "recibido"}`}>
                <p className="text-sm tracking-wide font-mono">{message.text}</p>
            </div>
        </div>
    )
}