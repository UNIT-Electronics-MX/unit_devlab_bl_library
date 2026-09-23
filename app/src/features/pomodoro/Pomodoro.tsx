import { useState } from "react"

import {
  Label,
  PolarAngleAxis,
  PolarRadiusAxis,
  RadialBar,
  RadialBarChart,
} from "recharts"

import {
  Minus,
  Pause,
  Play,
  Plus,
  RotateCcw,
} from "lucide-react"

import { Button } from "@/components/ui/button"

import {
  Card,
  CardContent,
  CardFooter,
} from "@/components/ui/card"

import {
  type ChartConfig,
  ChartContainer,
} from "@/components/ui/chart"

import { useBluetooth } from "@features/bluetooth/BluetoothProvider"

const chartConfig = {
  progress: {
    label: "Pomodoro",
    color: "#39A8F7",
  },
} satisfies ChartConfig

export function Pomodoro() {

  const {
    connected,
    pomodoro,
    sendCommand,
  } = useBluetooth()

  const [workMinutes, setWorkMinutes] = useState(25)
  const [breakMinutes, setBreakMinutes] = useState(5)

  const totalSeconds =
    pomodoro.mode === "break"
      ? breakMinutes * 60
      : workMinutes * 60

  const progress =
    totalSeconds > 0
      ? (pomodoro.remainingSeconds / totalSeconds) * 100
      : 0

  const chartData = [
    {
      name: "pomodoro",
      progress,
    },
  ]

  const minutes = Math.floor(
    pomodoro.remainingSeconds / 60
  )

  const seconds =
    pomodoro.remainingSeconds % 60

  const formattedTime =
    `${minutes.toString().padStart(2, "0")}:` +
    `${seconds.toString().padStart(2, "0")}`

  const startPomodoro = async () => {
    await sendCommand(
      `pomodoro:start|${workMinutes}|${breakMinutes}`
    )
  }

  const pausePomodoro = async () => {
    await sendCommand(
      `pomodoro:pause|${workMinutes}|${breakMinutes}`
    )
  }

  const resetPomodoro = async () => {
    await sendCommand(
      `pomodoro:reset|${workMinutes}|${breakMinutes}`
    )
  }

  const increaseWork = () => {
    setWorkMinutes((previous) =>
      Math.min(previous + 1, 120)
    )
  }

  const decreaseWork = () => {
    setWorkMinutes((previous) =>
      Math.max(previous - 1, 1)
    )
  }

  const increaseBreak = () => {
    setBreakMinutes((previous) =>
      Math.min(previous + 1, 60)
    )
  }

  const decreaseBreak = () => {
    setBreakMinutes((previous) =>
      Math.max(previous - 1, 1)
    )
  }

  return (
    <div className="dark">

      <Card className="flex flex-col border-border bg-blue-950 text-card-foreground">

        <CardContent className="flex flex-col items-center gap-6 pb-0">

          <ChartContainer
            config={chartConfig}
            className="mx-auto size-70"
          >

            <RadialBarChart
              data={chartData}
              startAngle={90}
              endAngle={-270}
              innerRadius="75%"
              outerRadius="95%"
              cx="50%"
              cy="50%"
            >

              <PolarAngleAxis
                type="number"
                domain={[0, 100]}
                dataKey="progress"
                tick={false}
              />

              <PolarRadiusAxis
                tick={false}
                tickLine={false}
                axisLine={false}
              >

                <Label
                  content={({ viewBox }) => {

                    if (
                      viewBox &&
                      "cx" in viewBox &&
                      "cy" in viewBox
                    ) {

                      return (
                        <text
                          x={viewBox.cx}
                          y={viewBox.cy}
                          textAnchor="middle"
                          dominantBaseline="middle"
                        >

                          <tspan
                            x={viewBox.cx}
                            y={(viewBox.cy ?? 0) - 10}
                            className="fill-foreground text-4xl font-bold"
                          >
                            {formattedTime}
                          </tspan>

                          <tspan
                            x={viewBox.cx}
                            y={(viewBox.cy ?? 0) + 25}
                            className="fill-muted-foreground text-sm"
                          >
                            {
                              pomodoro.mode === "break"
                                ? "Descanso"
                                : "Enfoque"
                            }
                          </tspan>

                        </text>
                      )

                    }

                    return null
                  }}
                />

              </PolarRadiusAxis>

              <RadialBar
                dataKey="progress"
                background={{
                  fill: "var(--muted)",
                }}
                cornerRadius={10}
                fill="var(--color-progress)"
              />

            </RadialBarChart>

          </ChartContainer>

          <div className="flex flex-wrap justify-center gap-3">

            <Button
              onClick={startPomodoro}
              disabled={
                !connected ||
                pomodoro.running
              }
              className="cursor-pointer gap-2"
            >
              <Play className="size-4" />

              Iniciar
            </Button>

            <Button
              variant="outline"
              onClick={pausePomodoro}
              disabled={
                !connected ||
                !pomodoro.running
              }
              className="cursor-pointer gap-2"
            >
              <Pause className="size-4" />

              Pausar
            </Button>

            <Button
              variant="outline"
              onClick={resetPomodoro}
              disabled={!connected}
              className="cursor-pointer gap-2"
            >
              <RotateCcw className="size-4" />

              Reiniciar
            </Button>

          </div>

          <div className="grid w-full max-w-md grid-cols-2 gap-4">

            <div className="flex flex-col items-center gap-3 rounded-xl border border-border bg-muted/30 p-4">

              <span className="text-sm text-muted-foreground">
                Enfoque
              </span>

              <span className="text-2xl font-bold">
                {workMinutes} min
              </span>

              <div className="flex gap-2">

                <Button
                  variant="outline"
                  size="icon"
                  disabled={pomodoro.running}
                  onClick={decreaseWork}
                  className="cursor-pointer"
                >
                  <Minus className="size-4" />
                </Button>

                <Button
                  variant="outline"
                  size="icon"
                  disabled={pomodoro.running}
                  onClick={increaseWork}
                  className="cursor-pointer"
                >
                  <Plus className="size-4" />
                </Button>

              </div>

            </div>

            <div className="flex flex-col items-center gap-3 rounded-xl border border-border bg-muted/30 p-4">

              <span className="text-sm text-muted-foreground">
                Descanso
              </span>

              <span className="text-2xl font-bold">
                {breakMinutes} min
              </span>

              <div className="flex gap-2">

                <Button
                  variant="outline"
                  size="icon"
                  disabled={pomodoro.running}
                  onClick={decreaseBreak}
                  className="cursor-pointer"
                >
                  <Minus className="size-4" />
                </Button>

                <Button
                  variant="outline"
                  size="icon"
                  disabled={pomodoro.running}
                  onClick={increaseBreak}
                  className="cursor-pointer"
                >
                  <Plus className="size-4" />
                </Button>

              </div>

            </div>

          </div>

        </CardContent>

        <CardFooter className="mt-4 flex-col gap-1 text-sm">

          <div className="font-medium">
            Pomodoro Timer
          </div>

          <div className="text-muted-foreground">
            {workMinutes} min enfoque ·{" "}
            {breakMinutes} min descanso
          </div>

          <div className="text-muted-foreground">
            {connected
              ? "Sincronizado con KUBI"
              : "Bluetooth desconectado"}
          </div>

        </CardFooter>

      </Card>

    </div>
  )
}