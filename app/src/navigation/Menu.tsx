import { Clock3, CloudSun, MessageCircle, Smile, Timer, type LucideIcon } from "lucide-react"

import { Button } from "@shadcn/button"

import type { AppModes } from "@models/navigation";

interface NavItem {
  id: AppModes
  label: string
  icon: LucideIcon
}

interface Props {
  activeView: AppModes
  onViewChange: (view: AppModes) => void
}

const NavItems: NavItem[] = [
  { id: "emociones", label: "Emociones", icon: Smile },
  { id: "mensajes", label: "Mensajes", icon: MessageCircle },
  { id: "clima", label: "Clima", icon: CloudSun },
  { id: "pomodoro", label: "Pomodoro", icon: Timer },
  { id: "rtc", label: "RTC", icon: Clock3 },
]

export function Menu({ activeView, onViewChange }: Props) {

  return (
    <nav className="flex gap-3 overflow-x-auto py-2">
      {NavItems.map(
        ({ id, label, icon: Icon }) => (
          <Button
            key={id}
            variant={activeView === id ? "default" : "outline"}
            className={`min-w-28 gap-2 cursor-pointer ${activeView === id
                ? "bg-blue-700 text-white hover:bg-blue-700"
                : "hover:bg-blue-50 hover:text-blue-700"
              }`}
            onClick={() => onViewChange(id)}
          >
            <Icon className="size-4" />

            {label}
          </Button>
        )
      )}
    </nav>
  )
}