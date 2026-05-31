const { WebSocketServer } = require('ws');

let wss = null;

function attachWebSocket(httpServer) {
  wss = new WebSocketServer({ server: httpServer, path: '/ws' });

  wss.on('connection', (ws) => {
    ws.on('error', (err) => console.error('WebSocket client error:', err.message));
  });

  console.log('WebSocket server attached at /ws');
}

/**
 * Broadcast a notification to all connected browser dashboard clients.
 * @param {import('./notificationStore').Notification} notification
 */
function broadcast(notification) {
  if (!wss) return;
  const message = JSON.stringify({ type: 'notification', data: notification });
  wss.clients.forEach((client) => {
    if (client.readyState === 1) client.send(message);
  });
}

module.exports = { attachWebSocket, broadcast };
