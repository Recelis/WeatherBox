const { Router } = require('express');
const crypto = require('crypto');
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
  if (!match) return;

  const enriched = { ...notification, priority: match.priority };
  const accepted = store.add(enriched);
  if (!accepted) return;

  broadcast(enriched);
  publishBatch(store.getRecent(rules.getRules().maxMessages));
}

// ── Slack ─────────────────────────────────────────────────────────────────────

router.post('/slack', (req, res) => {
  if (!verifySlackSignature(req)) return res.status(401).send('Invalid signature');

  const body = req.body;

  // Slack sends a one-time URL verification challenge when you first register
  if (body.type === 'url_verification') return res.json({ challenge: body.challenge });

  if (body.type === 'event_callback') {
    const event = body.event;

    // Only handle actual user messages (ignore bot messages, edits, deletions)
    if (event.type !== 'message' || event.subtype || event.bot_id) return res.sendStatus(200);

    const notification = {
      id: `slack-${event.channel}-${event.ts}`,
      source: 'slack',
      sender: event.user || 'Unknown',
      channel: event.channel_type === 'im' ? 'DM' : `#${event.channel}`,
      preview: (event.text || '').slice(0, 120),
      timestamp: new Date(parseFloat(event.ts) * 1000).toISOString(),
      priority: 1,
      metadata: {
        platformMessageId: event.ts,
        threadId: event.thread_ts || null,
        workspaceOrTenant: body.team_id,
      },
    };

    handleIncoming(notification);
  }

  res.sendStatus(200);
});

function verifySlackSignature(req) {
  const signingSecret = process.env.SLACK_SIGNING_SECRET;
  if (!signingSecret) {
    console.warn('SLACK_SIGNING_SECRET not set — skipping signature verification');
    return true;
  }

  const timestamp = req.headers['x-slack-request-timestamp'];
  const slackSig = req.headers['x-slack-signature'];
  if (!timestamp || !slackSig) return false;

  // Reject requests older than 5 minutes to prevent replay attacks
  if (Math.abs(Date.now() / 1000 - Number(timestamp)) > 300) return false;

  const rawBody = JSON.stringify(req.body);
  const baseString = `v0:${timestamp}:${rawBody}`;
  const hmac = crypto.createHmac('sha256', signingSecret).update(baseString).digest('hex');
  const expected = `v0=${hmac}`;

  return crypto.timingSafeEqual(Buffer.from(expected), Buffer.from(slackSig));
}

// ── Teams (stub — filled in by AC6) ──────────────────────────────────────────

router.post('/teams', (req, res) => {
  res.status(501).json({ error: 'Teams webhook not yet implemented' });
});

// ── Messenger (stub — filled in by AC7) ──────────────────────────────────────

router.post('/messenger', (req, res) => {
  res.status(501).json({ error: 'Messenger webhook not yet implemented' });
});

module.exports = router;
module.exports.handleIncoming = handleIncoming;
