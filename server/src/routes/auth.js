const { Router } = require('express');
const tokenStore = require('../services/tokenStore');

const router = Router();

// Connection status — used by the dashboard and by each platform's feature branch
router.get('/status', (_req, res) => {
  res.json({
    slack: tokenStore.isConnected('slack'),
    teams: tokenStore.isConnected('teams'),
    messenger: tokenStore.isConnected('messenger'),
  });
});

// Stubs — each platform's feature branch fills in the real OAuth flow
router.get('/slack', (_req, res) => res.status(501).send('Slack OAuth not yet implemented'));
router.get('/slack/callback', (_req, res) => res.status(501).send('Slack OAuth not yet implemented'));

router.get('/teams', (_req, res) => res.status(501).send('Teams OAuth not yet implemented'));
router.get('/teams/callback', (_req, res) => res.status(501).send('Teams OAuth not yet implemented'));

router.get('/messenger', (_req, res) => res.status(501).send('Messenger OAuth not yet implemented'));
router.get('/messenger/callback', (_req, res) => res.status(501).send('Messenger OAuth not yet implemented'));

module.exports = router;
