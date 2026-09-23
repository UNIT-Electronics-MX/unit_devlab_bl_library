import { useState } from "react";

import type { AppModes } from "@models/navigation";

import { Header } from "@components/header/Header";

import { Menu } from "@navigation/Menu";

import { EmotionsView } from "@features/emotions/EmotionsView";
import { Messages } from "@features/messages/Messages";
import { Weather } from "@features/weather/Weather";
import { Pomodoro } from "@features/pomodoro/Pomodoro";

export const App = () => {

  const [mode, setMode] = useState<AppModes>("emociones");

  return (
    <section className="flex min-h-screen flex-col bg-linear-to-b from-black to-blue-800">
      <Header />
      <main className="mx-auto w-full max-w-5xl p-5">
        <Menu activeView={mode} onViewChange={setMode} />

        <div className="pt-5" hidden={mode !== "emociones"} >
          <EmotionsView />
        </div>
        <div className="pt-5" hidden={mode !== "mensajes"} >
          <Messages />
        </div>
        <div className="pt-5" hidden={mode !== "clima"} >
          <Weather />
        </div>
        <div className="pt-5" hidden={mode !== "pomodoro"} >
          <Pomodoro />
        </div>
      </main>
    </section>
  )

}