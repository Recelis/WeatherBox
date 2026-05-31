const { Router } = require('express');
const store = require('../services/notificationStore');
const rulesService = require('../services/rules');
const tokenStore = require('../services/tokenStore');

const router = Router();

// Rules API — used by the rules dashboard UI (AC9)
router.get('/rules', (_req, res) => {
  res.json(rulesService.getRules());
});

router.post('/rules', (req, res) => {
  const { rules, maxMessages, deduplicateWindowSeconds } = req.body;
  if (!Array.isArray(rules)) return res.status(400).json({ error: 'rules must be an array' });
  rulesService.save({ rules, maxMessages, deduplicateWindowSeconds });
  res.json({ ok: true });
});

// Notification feed API — used by the live feed UI (AC10)
router.get('/feed', (_req, res) => {
  res.json(store.getAll());
});

// Auth status shortcut
router.get('/status', (_req, res) => {
  res.json({
    slack: tokenStore.isConnected('slack'),
    teams: tokenStore.isConnected('teams'),
    messenger: tokenStore.isConnected('messenger'),
  });
});

module.exports = router;
