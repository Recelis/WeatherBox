require('dotenv').config();
const { createServer } = require('http');
const app = require('./app');
const { initMqtt } = require('./services/mqtt');
const { attachWebSocket } = require('./services/websocket');
const tokenStore = require('./services/tokenStore');
const teamsSubscription = require('./services/teamsSubscription');

const PORT = process.env.PORT || 3000;

const httpServer = createServer(app);
attachWebSocket(httpServer);

httpServer.listen(PORT, async () => {
  console.log(`WeatherBox server running on http://localhost:${PORT}`);

  // Bootstrap Teams Graph subscription if already authenticated
  if (tokenStore.isConnected('teams') && process.env.TEAMS_WEBHOOK_URL) {
    try {
      await teamsSubscription.createSubscription(process.env.TEAMS_WEBHOOK_URL);
    } catch (err) {
      console.warn('Could not create Teams subscription on startup:', err.message);
    }
  }
});

initMqtt();
