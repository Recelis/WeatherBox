require('dotenv').config();
const { createServer } = require('http');
const app = require('./app');
const { initMqtt } = require('./services/mqtt');
const { attachWebSocket } = require('./services/websocket');
const tokenStore = require('./services/tokenStore');
const teamsSubscription = require('./services/teamsSubscription');
const weatherPoller = require('./services/weatherPoller');

const PORT = process.env.PORT || 3000;

const httpServer = createServer(app);
attachWebSocket(httpServer);

httpServer.listen(PORT, async () => {
  console.log(`WeatherBox server running on http://localhost:${PORT}`);

  // Bootstrap Teams Graph subscription if already authenticated
  if (tokenStore.isConnected('teams') && process.env.TEAMS_WEBHOOK_URL) {
    try {
      await teamsSubscription.createSubscriptions(process.env.TEAMS_WEBHOOK_URL);
    } catch (err) {
      console.warn('Could not create Teams subscription on startup:', err.message);
    }
  }
});

initMqtt();

if (process.env.WEATHER_API_KEY) {
  weatherPoller.start();
} else {
  console.log('WEATHER_API_KEY not set — weather poller disabled. Set it in .env to enable.');
}
