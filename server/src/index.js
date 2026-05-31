require('dotenv').config();
const { createServer } = require('http');
const app = require('./app');
const { initMqtt } = require('./services/mqtt');
const { attachWebSocket } = require('./services/websocket');

const PORT = process.env.PORT || 3000;

const httpServer = createServer(app);
attachWebSocket(httpServer);

httpServer.listen(PORT, () => {
  console.log(`WeatherBox server running on http://localhost:${PORT}`);
});

initMqtt();
