import {
  CloudSun,
  Droplets,
  LoaderCircle,
  MapPin,
  RefreshCw,
  Send,
  Thermometer,
} from "lucide-react"
import { useEffect, useState } from "react"
import { Bar, BarChart, CartesianGrid, XAxis, YAxis } from "recharts"

import { Button } from "@shadcn/button"
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@shadcn/card"
import {
  ChartContainer,
  ChartTooltip,
  ChartTooltipContent,
  type ChartConfig,
} from "@shadcn/chart"

import { useBluetooth } from "@features/bluetooth/BluetoothProvider"
import {
  getCurrentWeather,
  type WeatherData,
} from "@services/weather/weather.service"

const temperatureConfig = {
  temperature: {
    label: "Temperatura",
    color: "#27B4F5",
  },
} satisfies ChartConfig

const humidityConfig = {
  humidity: {
    label: "Humedad",
    color: "#CC297D",
  },
} satisfies ChartConfig

export const Weather = () => {
  const { connected, sendCommand } = useBluetooth()

  const [weather, setWeather] = useState<WeatherData | null>(null)
  const [loading, setLoading] = useState(false)
  const [sending, setSending] = useState(false)
  const [error, setError] = useState<string | null>(null)

  const fetchWeather = async () => {
    if (loading) return

    setLoading(true)
    setError(null)

    try {
      const data = await getCurrentWeather()
      setWeather(data)
    } catch (error) {
      setError(
        error instanceof Error
          ? error.message
          : "Error al obtener el clima"
      )
    } finally {
      setLoading(false)
    }
  }

  const sendWeatherToKubi = async () => {
    if (!weather || !connected || sending) return

    setSending(true)

    try {
      const command =
        `weather:${weather.temperature}|${weather.description.toLowerCase()}|Ubicacion actual|${weather.humidity}`

      await sendCommand(command)
    } finally {
      setSending(false)
    }
  }

  useEffect(() => {
    fetchWeather()
  }, [])

  return (
    <div className="space-y-4">
      <Card>
        <CardHeader>
          <div className="flex items-start justify-between gap-4">
            <div>
              <CardTitle>Clima</CardTitle>
              <CardDescription>
                Condiciones meteorológicas de tu ubicación actual.
              </CardDescription>
            </div>

            <Button
              type="button"
              variant="outline"
              size="sm"
              disabled={loading}
              onClick={fetchWeather}
            >
              {loading
                ? <LoaderCircle className="size-4 animate-spin" />
                : <RefreshCw className="size-4" />}
              Actualizar
            </Button>
          </div>
        </CardHeader>

        <CardContent>
          {loading && !weather && (
            <div className="flex h-52 flex-col items-center justify-center gap-3 text-muted-foreground">
              <LoaderCircle className="size-7 animate-spin" />
              <p className="text-sm">Obteniendo clima...</p>
            </div>
          )}

          {error && !weather && (
            <div className="flex h-52 flex-col items-center justify-center gap-3 text-center">
              <CloudSun className="size-8 text-muted-foreground" />
              <p className="text-sm text-destructive">{error}</p>
              <Button variant="outline" onClick={fetchWeather}>
                Intentar nuevamente
              </Button>
            </div>
          )}

          {weather && (
            <div className="space-y-6">
              <div className="grid grid-cols-2 gap-3 lg:grid-cols-4">
                <div className="rounded-xl border bg-muted/20 p-4">
                  <div className="mb-3 flex items-center gap-2 text-sm text-muted-foreground">
                    <Thermometer className="size-4" />
                    Temperatura
                  </div>
                  <p className="text-3xl font-semibold">
                    {weather.temperature}°C
                  </p>
                </div>

                <div className="rounded-xl border bg-muted/20 p-4">
                  <div className="mb-3 flex items-center gap-2 text-sm text-muted-foreground">
                    <Droplets className="size-4" />
                    Humedad
                  </div>
                  <p className="text-3xl font-semibold">
                    {weather.humidity}%
                  </p>
                </div>

                <div className="rounded-xl border bg-muted/20 p-4">
                  <div className="mb-3 flex items-center gap-2 text-sm text-muted-foreground">
                    <CloudSun className="size-4" />
                    Condición
                  </div>
                  <p className="font-semibold">
                    {weather.description}
                  </p>
                </div>

                <div className="rounded-xl border bg-muted/20 p-4">
                  <div className="mb-3 flex items-center gap-2 text-sm text-muted-foreground">
                    <MapPin className="size-4" />
                    Ubicación
                  </div>
                  <p className="text-sm font-semibold">
                    {weather.latitude.toFixed(2)},{" "}
                    {weather.longitude.toFixed(2)}
                  </p>
                </div>
              </div>

              <div className="flex justify-end">
                <Button
                  type="button"
                  disabled={!connected || sending}
                  onClick={sendWeatherToKubi}
                >
                  {sending
                    ? <LoaderCircle className="size-4 animate-spin" />
                    : <Send className="size-4" />}

                  {sending ? "Enviando..." : "Enviar a Kubi"}
                </Button>
              </div>
            </div>
          )}
        </CardContent>
      </Card>

      {weather && (
        <div className="grid gap-4 lg:grid-cols-2">
          <Card>
            <CardHeader>
              <CardTitle className="text-base">
                Temperatura por hora
              </CardTitle>
              <CardDescription>
                Pronóstico para las próximas 8 horas.
              </CardDescription>
            </CardHeader>

            <CardContent>
              <ChartContainer
                config={temperatureConfig}
                className="h-65 w-full"
              >
                <BarChart
                  accessibilityLayer
                  data={weather.hourly}
                  margin={{ left: -15, right: 5 }}
                >
                  <CartesianGrid vertical={false} />

                  <XAxis
                    dataKey="time"
                    tickLine={false}
                    axisLine={false}
                    tickMargin={10}
                  />

                  <YAxis
                    tickLine={false}
                    axisLine={false}
                    tickFormatter={(value) => `${value}°`}
                  />

                  <ChartTooltip
                    cursor={false}
                    content={
                      <ChartTooltipContent
                        formatter={(value) => (
                          <span>{value} °C</span>
                        )}
                      />
                    }
                  />

                  <Bar
                    dataKey="temperature"
                    fill="var(--color-temperature)"
                    radius={[6, 6, 0, 0]}
                    className="bg-blue-400"
                  />
                </BarChart>
              </ChartContainer>
            </CardContent>
          </Card>

          <Card>
            <CardHeader>
              <CardTitle className="text-base">
                Humedad por hora
              </CardTitle>
              <CardDescription>
                Humedad relativa para las próximas 8 horas.
              </CardDescription>
            </CardHeader>

            <CardContent>
              <ChartContainer
                config={humidityConfig}
                className="h-65 w-full"
              >
                <BarChart
                  accessibilityLayer
                  data={weather.hourly}
                  margin={{ left: -15, right: 5 }}
                >
                  <CartesianGrid vertical={false} />

                  <XAxis
                    dataKey="time"
                    tickLine={false}
                    axisLine={false}
                    tickMargin={10}
                  />

                  <YAxis
                    domain={[0, 100]}
                    tickLine={false}
                    axisLine={false}
                    tickFormatter={(value) => `${value}%`}
                  />

                  <ChartTooltip
                    cursor={false}
                    content={
                      <ChartTooltipContent
                        formatter={(value) => (
                          <span>{value}%</span>
                        )}
                      />
                    }
                  />

                  <Bar
                    dataKey="humidity"
                    fill="var(--color-humidity)"
                    radius={[6, 6, 0, 0]}
                  />
                </BarChart>
              </ChartContainer>
            </CardContent>
          </Card>
        </div>
      )}
    </div>
  )
}