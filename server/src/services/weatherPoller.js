const { publishBatch } = require('./mqtt');

const PIRATE_WEATHER_BASE = 'https://api.pirateweather.net/forecast';
const IPAPI_URL = 'https://ipapi.co/json/';

const DAY_NAMES = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];

let intervalId = null;

// ── Location ──────────────────────────────────────────────────────────────────

async function getLocation() {
  // Use fixed coords from env if provided — avoids a network call every hour
  const lat = parseFloat(process.env.WEATHER_LAT);
  const lon = parseFloat(process.env.WEATHER_LON);
  if (!isNaN(lat) && !isNaN(lon)) {
    return { lat, lon, city: process.env.WEATHER_CITY || 'Home' };
  }

  const res = await fetch(IPAPI_URL);
  if (!res.ok) throw new Error(`ipapi.co returned ${res.status}`);
  const data = await res.json();
  return { lat: data.latitude, lon: data.longitude, city: data.city || 'Unknown' };
}

// ── Weather fetch ─────────────────────────────────────────────────────────────

async function fetchWeather() {
  const apiKey = process.env.WEATHER_API_KEY;
  if (!apiKey) throw new Error('WEATHER_API_KEY not set in .env');

  const { lat, lon, city } = await getLocation();

  const url = `${PIRATE_WEATHER_BASE}/${apiKey}/${lat},${lon}?units=ca&exclude=minutely,hourly,alerts`;
  const res = await fetch(url);
  if (!res.ok) throw new Error(`PirateWeather returned ${res.status}`);
  const data = await res.json();

  const current = data.currently;
  const temp = Math.round(current.temperature ?? 0);
  const humidity = Math.round((current.humidity ?? 0) * 100);
  const summary = current.summary || 'Unknown';

  const dayName = DAY_NAMES[new Date().getDay()];
  const channel = `${city} · ${dayName}`;
  const preview = `${temp}°C · ${summary} · Humidity ${humidity}%`;

  const notification = {
    id: `iot-weather-${Math.floor(Date.now() / 1000)}`,
    source: 'iot',
    sender: 'Weather',
    channel: channel.slice(0, 32),
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

  // Fetch immediately on start, then on the interval
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
