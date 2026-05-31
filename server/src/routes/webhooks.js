const { Router } = require('express');
const store = require('../services/notificationStore');
const rules = require('../services/rules');
const { publishBatch } = require('../services/mqtt');
const { broadcast } = require('../services/websocket');

const router = Router();

/**
 * Central handler called by each platform's webhook route after it has
 * parsed and normalised the payload into the unified Notification schema.
 *
 * @param {import('../services/notificationStore').Notification} notification
 */
function handleIncoming(notification) {
  const match = rules.evaluate(notification);
  if (!match) return; // dropped by rules engine

  const enriched = { ...notification, priority: match.priority };
  const accepted = store.add(enriched);
  if (!accepted) return; // duplicate within dedupe window

  broadcast(enriched);
  publishBatch(store.getRecent(rules.getRules().maxMessages));
}

// Stubs — each platform's feature branch fills in the real implementation
router.post('/slack', (req, res) => {
  res.status(501).json({ error: 'Slack webhook not yet implemented' });
});

router.post('/teams', (req, res) => {
  res.status(501).json({ error: 'Teams webhook not yet implemented' });
});

router.post('/messenger', (req, res) => {
  res.status(501).json({ error: 'Messenger webhook not yet implemented' });
});

module.exports = router;
module.exports.handleIncoming = handleIncoming;
