export interface HourlyWeather {
  time: string
  temperature: number
  humidity: number
}

export interface WeatherData {
  temperature: number
  humidity: number
  weatherCode: number
  description: string
  latitude: number
  longitude: number
  hourly: HourlyWeather[]
}

interface OpenMeteoResponse {
  current: {
    temperature_2m: number
    relative_humidity_2m: number
    weather_code: number
  }
  hourly: {
    time: string[]
    temperature_2m: number[]
    relative_humidity_2m: number[]
  }
}

const WEATHER_DESCRIPTIONS: Record<number, string> = {
  0: "Despejado",
  1: "Mayormente despejado",
  2: "Parcialmente nublado",
  3: "Nublado",
  45: "Niebla",
  48: "Niebla con escarcha",
  51: "Llovizna ligera",
  53: "Llovizna",
  55: "Llovizna intensa",
  61: "Lluvia ligera",
  63: "Lluvia",
  65: "Lluvia intensa",
  71: "Nevada ligera",
  73: "Nevada",
  75: "Nevada intensa",
  80: "Chubascos ligeros",
  81: "Chubascos",
  82: "Chubascos intensos",
  95: "Tormenta",
  96: "Tormenta con granizo",
  99: "Tormenta intensa",
}

const getPosition = (): Promise<GeolocationPosition> => {
  return new Promise((resolve, reject) => {
    navigator.geolocation.getCurrentPosition(resolve, reject, {
      enableHighAccuracy: false,
      timeout: 10000,
      maximumAge: 300000,
    })
  })
}

export const getCurrentWeather = async (): Promise<WeatherData> => {
  if (!navigator.geolocation) {
    throw new Error("El navegador no soporta geolocalización")
  }

  const position = await getPosition()
  const latitude = position.coords.latitude
  const longitude = position.coords.longitude

  const params = new URLSearchParams({
    latitude: latitude.toString(),
    longitude: longitude.toString(),
    current: "temperature_2m,relative_humidity_2m,weather_code",
    hourly: "temperature_2m,relative_humidity_2m",
    timezone: "auto",
    forecast_days: "1",
  })

  const response = await fetch(
    `https://api.open-meteo.com/v1/forecast?${params}`
  )

  if (!response.ok) {
    throw new Error("No fue posible consultar el clima")
  }

  const data: OpenMeteoResponse = await response.json()
  const now = new Date()

  const hourly = data.hourly.time
    .map((time, index) => ({
      rawTime: time,
      time: new Date(time).toLocaleTimeString("es-MX", {
        hour: "2-digit",
        minute: "2-digit",
      }),
      temperature: Math.round(data.hourly.temperature_2m[index]),
      humidity: data.hourly.relative_humidity_2m[index],
    }))
    .filter(({ rawTime }) => new Date(rawTime) >= now)
    .slice(0, 8)
    .map(({ rawTime: _, ...item }) => item)

  return {
    temperature: Math.round(data.current.temperature_2m),
    humidity: data.current.relative_humidity_2m,
    weatherCode: data.current.weather_code,
    description:
      WEATHER_DESCRIPTIONS[data.current.weather_code] ??
      "Condición desconocida",
    latitude,
    longitude,
    hourly,
  }
}