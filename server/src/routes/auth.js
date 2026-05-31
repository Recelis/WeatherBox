const { Router } = require('express');
const crypto = require('crypto');
const tokenStore = require('../services/tokenStore');

const router = Router();

// ── Helpers ───────────────────────────────────────────────────────────────────

function makeState() {
  return crypto.randomBytes(16).toString('hex');
}

// In-memory state store for CSRF protection (keyed by state token, expires in 10 min)
const pendingStates = new Map();
function saveState(state) {
  pendingStates.set(state, Date.now());
  setTimeout(() => pendingStates.delete(state), 10 * 60 * 1000);
}
function consumeState(state) {
  if (!pendingStates.has(state)) return false;
  pendingStates.delete(state);
  return true;
}

// ── Auth status ───────────────────────────────────────────────────────────────

router.get('/status', (_req, res) => {
  res.json({
    slack: tokenStore.isConnected('slack'),
    teams: tokenStore.isConnected('teams'),
    messenger: tokenStore.isConnected('messenger'),
  });
});

// ── Slack OAuth 2.0 ───────────────────────────────────────────────────────────

router.get('/slack', (req, res) => {
  const { SLACK_BOT_TOKEN, SLACK_CLIENT_ID } = process.env;

  // If a bot token is provided directly in .env, skip the OAuth flow
  if (SLACK_BOT_TOKEN) {
    tokenStore.save('slack', { botToken: SLACK_BOT_TOKEN });
    return res.redirect('/auth/status');
  }

  if (!SLACK_CLIENT_ID) return res.status(500).send('SLACK_CLIENT_ID not set in .env');

  const state = makeState();
  saveState(state);

  const redirectUri = `${req.protocol}://${req.get('host')}/auth/slack/callback`;
  const scopes = 'channels:history,channels:read,groups:history,im:history,users:read';

  const url = new URL('https://slack.com/oauth/v2/authorize');
  url.searchParams.set('client_id', SLACK_CLIENT_ID);
  url.searchParams.set('scope', scopes);
  url.searchParams.set('redirect_uri', redirectUri);
  url.searchParams.set('state', state);

  res.redirect(url.toString());
});

router.get('/slack/callback', async (req, res) => {
  const { code, state, error } = req.query;

  if (error) return res.status(400).send(`Slack OAuth error: ${error}`);
  if (!consumeState(state)) return res.status(400).send('Invalid or expired state');

  const { SLACK_CLIENT_ID, SLACK_CLIENT_SECRET } = process.env;
  const redirectUri = `${req.protocol}://${req.get('host')}/auth/slack/callback`;

  try {
    const response = await fetch('https://slack.com/api/oauth.v2.access', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: new URLSearchParams({ client_id: SLACK_CLIENT_ID, client_secret: SLACK_CLIENT_SECRET, code, redirect_uri: redirectUri }),
    });
    const data = await response.json();
    if (!data.ok) return res.status(400).send(`Slack token exchange failed: ${data.error}`);

    tokenStore.save('slack', {
      botToken: data.access_token,
      teamId: data.team?.id,
      teamName: data.team?.name,
    });

    res.send('Slack connected! You can close this tab.');
  } catch (err) {
    res.status(500).send(`Token exchange error: ${err.message}`);
  }
});

// ── MS Teams OAuth 2.0 (Azure AD) ─────────────────────────────────────────────

router.get('/teams', (req, res) => {
  const { TEAMS_TENANT_ID, TEAMS_CLIENT_ID } = process.env;
  if (!TEAMS_TENANT_ID || !TEAMS_CLIENT_ID) return res.status(500).send('TEAMS_TENANT_ID and TEAMS_CLIENT_ID not set in .env');

  const state = makeState();
  saveState(state);

  const redirectUri = process.env.TEAMS_REDIRECT_URI || `${req.protocol}://${req.get('host')}/auth/teams/callback`;
  const scopes = 'ChannelMessage.Read.All Chat.Read offline_access';

  const url = new URL(`https://login.microsoftonline.com/${TEAMS_TENANT_ID}/oauth2/v2.0/authorize`);
  url.searchParams.set('client_id', TEAMS_CLIENT_ID);
  url.searchParams.set('response_type', 'code');
  url.searchParams.set('redirect_uri', redirectUri);
  url.searchParams.set('scope', scopes);
  url.searchParams.set('state', state);

  res.redirect(url.toString());
});

router.get('/teams/callback', async (req, res) => {
  const { code, state, error } = req.query;

  if (error) return res.status(400).send(`Teams OAuth error: ${error}`);
  if (!consumeState(state)) return res.status(400).send('Invalid or expired state');

  const { TEAMS_TENANT_ID, TEAMS_CLIENT_ID, TEAMS_CLIENT_SECRET } = process.env;
  const redirectUri = process.env.TEAMS_REDIRECT_URI || `${req.protocol}://${req.get('host')}/auth/teams/callback`;

  try {
    const response = await fetch(`https://login.microsoftonline.com/${TEAMS_TENANT_ID}/oauth2/v2.0/token`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: new URLSearchParams({ client_id: TEAMS_CLIENT_ID, client_secret: TEAMS_CLIENT_SECRET, code, redirect_uri: redirectUri, grant_type: 'authorization_code' }),
    });
    const data = await response.json();
    if (data.error) return res.status(400).send(`Teams token exchange failed: ${data.error_description}`);

    tokenStore.save('teams', {
      accessToken: data.access_token,
      refreshToken: data.refresh_token,
      expiresAt: new Date(Date.now() + data.expires_in * 1000).toISOString(),
    });

    res.send('MS Teams connected! You can close this tab.');
  } catch (err) {
    res.status(500).send(`Token exchange error: ${err.message}`);
  }
});

// ── Facebook Messenger OAuth ───────────────────────────────────────────────────

router.get('/messenger', (req, res) => {
  const { MESSENGER_APP_ID } = process.env;
  if (!MESSENGER_APP_ID) return res.status(500).send('MESSENGER_APP_ID not set in .env');

  const state = makeState();
  saveState(state);

  const redirectUri = process.env.MESSENGER_REDIRECT_URI || `${req.protocol}://${req.get('host')}/auth/messenger/callback`;

  const url = new URL('https://www.facebook.com/v19.0/dialog/oauth');
  url.searchParams.set('client_id', MESSENGER_APP_ID);
  url.searchParams.set('redirect_uri', redirectUri);
  url.searchParams.set('scope', 'pages_messaging,pages_show_list,pages_read_engagement');
  url.searchParams.set('state', state);

  res.redirect(url.toString());
});

router.get('/messenger/callback', async (req, res) => {
  const { code, state, error } = req.query;

  if (error) return res.status(400).send(`Messenger OAuth error: ${error}`);
  if (!consumeState(state)) return res.status(400).send('Invalid or expired state');

  const { MESSENGER_APP_ID, MESSENGER_APP_SECRET } = process.env;
  const redirectUri = process.env.MESSENGER_REDIRECT_URI || `${req.protocol}://${req.get('host')}/auth/messenger/callback`;

  try {
    const url = new URL('https://graph.facebook.com/v19.0/oauth/access_token');
    url.searchParams.set('client_id', MESSENGER_APP_ID);
    url.searchParams.set('client_secret', MESSENGER_APP_SECRET);
    url.searchParams.set('redirect_uri', redirectUri);
    url.searchParams.set('code', code);

    const response = await fetch(url.toString());
    const data = await response.json();
    if (data.error) return res.status(400).send(`Messenger token exchange failed: ${data.error.message}`);

    tokenStore.save('messenger', {
      accessToken: data.access_token,
      expiresAt: data.expires_in ? new Date(Date.now() + data.expires_in * 1000).toISOString() : null,
    });

    res.send('Facebook Messenger connected! You can close this tab.');
  } catch (err) {
    res.status(500).send(`Token exchange error: ${err.message}`);
  }
});

module.exports = router;
