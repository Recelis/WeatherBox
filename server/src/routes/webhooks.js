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

// ── MS Teams (Graph change notifications) ─────────────────────────────────────

router.post('/teams', (req, res) => {
  // Graph sends a validationToken query param when first registering the subscription
  if (req.query.validationToken) {
    return res.status(200).type('text/plain').send(req.query.validationToken);
  }

  if (!verifyTeamsClientState(req.body)) return res.status(401).send('Invalid client state');

  const items = req.body?.value ?? [];
  for (const item of items) {
    const msg = item.resourceData;
    if (!msg) continue;

    const notification = {
      id: `teams-${msg.id || item.subscriptionId + '-' + Date.now()}`,
      source: 'teams',
      sender: msg.from?.user?.displayName || msg.from?.application?.displayName || 'Unknown',
      channel: msg.channelIdentity?.channelId || msg.chatId || 'Teams',
      preview: (msg.body?.content || '').replace(/<[^>]+>/g, '').slice(0, 120),
      timestamp: msg.createdDateTime || new Date().toISOString(),
      priority: 1,
      metadata: {
        platformMessageId: msg.id,
        threadId: msg.replyToId || null,
        workspaceOrTenant: msg.channelIdentity?.teamId || null,
      },
    };

    handleIncoming(notification);
  }

  res.sendStatus(202);
});

function verifyTeamsClientState(body) {
  const expectedState = process.env.TEAMS_CLIENT_SECRET?.slice(0, 16) || 'weatherbox';
  const items = body?.value ?? [];
  return items.every((item) => !item.clientState || item.clientState === expectedState);
}

// ── Facebook Messenger ────────────────────────────────────────────────────────

// Meta sends a GET request to verify the webhook URL before activating it
router.get('/messenger', (req, res) => {
  const mode = req.query['hub.mode'];
  const token = req.query['hub.verify_token'];
  const challenge = req.query['hub.challenge'];

  if (mode === 'subscribe' && token === process.env.MESSENGER_VERIFY_TOKEN) {
    return res.status(200).send(challenge);
  }
  res.sendStatus(403);
});

router.post('/messenger', (req, res) => {
  const body = req.body;
  if (body.object !== 'page') return res.sendStatus(404);

  for (const entry of body.entry ?? []) {
    for (const event of entry.messaging ?? []) {
      // Only handle inbound text messages (ignore echoes, reads, deliveries)
      if (!event.message || event.message.is_echo) continue;

      const senderId = event.sender?.id;
      const text = event.message.text || '';
      if (!text) continue;

      const notification = {
        id: `messenger-${event.message.mid || senderId + '-' + event.timestamp}`,
        source: 'messenger',
        sender: senderId || 'Unknown',
        channel: senderId || 'Messenger',
        preview: text.slice(0, 120),
        timestamp: event.timestamp ? new Date(event.timestamp).toISOString() : new Date().toISOString(),
        priority: 1,
        metadata: {
          platformMessageId: event.message.mid,
          threadId: senderId,
        },
      };

      // Resolve sender name asynchronously — update store entry if it changes
      resolveSenderName(senderId).then((name) => {
        if (name !== senderId) {
          notification.sender = name;
          notification.channel = name;
        }
        handleIncoming(notification);
      }).catch(() => handleIncoming(notification));
    }
  }

  // Meta requires a 200 response quickly to avoid retries
  res.sendStatus(200);
});

async function resolveSenderName(psid) {
  const token = require('../services/tokenStore').get('messenger')?.accessToken;
  if (!token || !psid) return psid;
  try {
    const res = await fetch(`https://graph.facebook.com/v19.0/${psid}?fields=name&access_token=${token}`);
    const data = await res.json();
    return data.name || psid;
  } catch {
    return psid;
  }
}

module.exports = router;
module.exports.handleIncoming = handleIncoming;
