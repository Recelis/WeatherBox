const mqtt = require('mqtt');
const fs = require('fs');

let client = null;

function initMqtt() {
  const { AWS_IOT_ENDPOINT, AWS_IOT_CERT_PATH, AWS_IOT_KEY_PATH, AWS_IOT_CA_PATH } = process.env;

  if (!AWS_IOT_ENDPOINT || !AWS_IOT_CERT_PATH) {
    console.warn('AWS IoT credentials not configured — MQTT publisher disabled.');
    return;
  }

  client = mqtt.connect(`mqtts://${AWS_IOT_ENDPOINT}`, {
    cert: fs.readFileSync(AWS_IOT_CERT_PATH),
    key: fs.readFileSync(AWS_IOT_KEY_PATH),
    ca: fs.readFileSync(AWS_IOT_CA_PATH),
    clientId: `weatherbox-server-${Date.now()}`,
    reconnectPeriod: 5000,
  });

  client.on('connect', () => console.log('MQTT connected to AWS IoT'));
  client.on('error', (err) => console.error('MQTT error:', err.message));
  client.on('reconnect', () => console.log('MQTT reconnecting...'));
}

/**
 * Publish a batch of notifications to the ESP32 via AWS IoT MQTT.
 * Payload is serialised once here and must fit the Mega's 6000-byte Serial buffer.
 * @param {import('./notificationStore').Notification[]} notifications
 */
function publishBatch(notifications) {
  if (!client?.connected) {
    console.warn('MQTT not connected — skipping publish');
    return;
  }

  const topic = process.env.AWS_IOT_TOPIC_PUBLISH || 'weatherbox/notifications';
  const payload = JSON.stringify({ notifications });

  if (Buffer.byteLength(payload) > 6000) {
    console.warn(`Batch payload ${Buffer.byteLength(payload)} bytes exceeds 6000-byte Mega buffer — truncating`);
    // Trim from the end (lowest priority messages were appended last)
    while (notifications.length > 1 && Buffer.byteLength(JSON.stringify({ notifications })) > 6000) {
      notifications.pop();
    }
  }

  client.publish(topic, JSON.stringify({ notifications }), { qos: 1 });
}

module.exports = { initMqtt, publishBatch };
