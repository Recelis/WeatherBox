const { publishBatch } = require('./mqtt');

const IP_API_URL = 'http://ip-api.com/json/?fields=lat,lon,city';
const OPEN_METEO_BASE = 'https://api.open-meteo.com/v1/forecast';

const DAY_NAMES = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];

// WMO weather interpretation codes → human-readable summary
// https://open-meteo.com/en/docs#weathervariables
const WMO_CODES = {
  0: 'Clear sky',
  1: 'Mainly clear', 2: 'Partly cloudy', 3: 'Overcast',
  45: 'Foggy', 48: 'Icy fog',
  51: 'Light drizzle', 53: 'Drizzle', 55: 'Heavy drizzle',
  61: 'Light rain', 63: 'Rain', 65: 'Heavy rain',
  71: 'Light snow', 73: 'Snow', 75: 'Heavy snow',
  77: 'Snow grains',
  80: 'Light showers', 81: 'Showers', 82: 'Heavy showers',
  85: 'Snow showers', 86: 'Heavy snow showers',
  95: 'Thunderstorm',
  96: 'Thunderstorm with hail', 99: 'Heavy thunderstorm with hail',
};

let intervalId = null;
let cachedLocation = null;

// ── Location ──────────────────────────────────────────────────────────────────

async function getLocation() {
  // Fixed coords in .env take priority — recommended for a Pi at home
  const lat = parseFloat(process.env.WEATHER_LAT);
  const lon = parseFloat(process.env.WEATHER_LON);
  if (!isNaN(lat) && !isNaN(lon)) {
    return { lat, lon, city: process.env.WEATHER_CITY || 'Home' };
  }

  // ip-api.com: free, no API key, 45 req/min — cached so it's only called once per run
  if (cachedLocation) return cachedLocation;

  const res = await fetch(IP_API_URL);
  if (!res.ok) throw new Error(`ip-api.com returned ${res.status} — set WEATHER_LAT and WEATHER_LON in .env to skip geolocation`);
  const data = await res.json();
  if (data.status === 'fail') throw new Error(`ip-api.com: ${data.message} — set WEATHER_LAT and WEATHER_LON in .env`);

  cachedLocation = { lat: data.lat, lon: data.lon, city: data.city || 'Unknown' };
  return cachedLocation;
}

// ── Weather fetch ─────────────────────────────────────────────────────────────

async function fetchWeather() {
  const { lat, lon, city } = await getLocation();

  const url = new URL(OPEN_METEO_BASE);
  url.searchParams.set('latitude', lat);
  url.searchParams.set('longitude', lon);
  url.searchParams.set('current', 'temperature_2m,relative_humidity_2m,weather_code');
  url.searchParams.set('timezone', 'auto');

  const res = await fetch(url.toString());
  if (!res.ok) throw new Error(`Open-Meteo returned ${res.status}`);
  const data = await res.json();

  const current = data.current;
  const temp = Math.round(current.temperature_2m ?? 0);
  const humidity = Math.round(current.relative_humidity_2m ?? 0);
  const summary = WMO_CODES[current.weather_code] ?? 'Unknown';

  const dayName = DAY_NAMES[new Date().getDay()];
  const preview = `${temp}°C · ${summary} · Humidity ${humidity}%`;

  const notification = {
    id: `iot-weather-${Math.floor(Date.now() / 1000)}`,
    source: 'iot',
    sender: 'Weather',
    channel: `${city} · ${dayName}`.slice(0, 32),
    preview: preview.slice(0, 120),
    timestamp: new Date().toISOString(),
    priority: 1,
    metadata: { platformMessageId: String(Date.now()) },
  };

  publishBatch([notification]);
  console.log(`Weather published: ${preview}`);
  return notification;
}

// ── Scheduler ─────────────────────────────────────────────────────────────────

function start() {
  const intervalMs = parseInt(process.env.WEATHER_POLL_INTERVAL_MS, 10) || 60 * 60 * 1000;

  fetchWeather().catch((err) => console.error('Weather fetch failed:', err.message));
  intervalId = setInterval(() => {
    fetchWeather().catch((err) => console.error('Weather fetch failed:', err.message));
  }, intervalMs);

  console.log(`Weather poller started — interval ${intervalMs / 1000}s`);
}

function stop() {
  if (intervalId) { clearInterval(intervalId); intervalId = null; }
}

module.exports = { start, stop, fetchWeather };
