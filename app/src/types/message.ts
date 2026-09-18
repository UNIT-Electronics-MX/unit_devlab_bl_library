export interface Message {
  id: string
  text: string
  type: "sent" | "received"
  timestamp: Date
}