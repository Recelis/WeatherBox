/**
 * Manages Microsoft Graph change notification subscriptions using delegated
 * permissions — reads messages as the signed-in user, no admin consent needed.
 *
 * Subscribes to:
 *   - /chats/getAllMessages   (all 1:1 and group chats — Chat.Read delegated)
 *   - Each team channel listed in TEAMS_CHANNEL_IDS env var (ChannelMessage.Read delegated)
 *
 * Subscriptions expire after 60 minutes max; this service renews them automatically.
 * Access tokens (~1 hour lifetime) are refreshed before they expire.
 */
const tokenStore = require('./tokenStore');

const GRAPH_BASE = 'https://graph.microsoft.com/v1.0';
const RENEWAL_BUFFER_MS = 5 * 60 * 1000;

// Map of subscriptionId → { expiresAt, timerId }
const activeSubscriptions = new Map();

async function getValidToken() {
  const tokens = tokenStore.get('teams');
  if (!tokens?.accessToken) throw new Error('Teams not authenticated — visit /auth/teams first');

  // Refresh if within 5 minutes of expiry
  if (tokens.expiresAt && new Date(tokens.expiresAt).getTime() - Date.now() < RENEWAL_BUFFER_MS) {
    return refreshAccessToken(tokens);
  }
  return tokens.accessToken;
}

async function refreshAccessToken(tokens) {
  const { TEAMS_TENANT_ID, TEAMS_CLIENT_ID, TEAMS_CLIENT_SECRET } = process.env;
  const res = await fetch(`https://login.microsoftonline.com/${TEAMS_TENANT_ID}/oauth2/v2.0/token`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: new URLSearchParams({
      client_id: TEAMS_CLIENT_ID,
      client_secret: TEAMS_CLIENT_SECRET,
      grant_type: 'refresh_token',
      refresh_token: tokens.refreshToken,
      scope: 'ChannelMessage.Read Chat.Read offline_access',
    }),
  });
  const data = await res.json();
  if (data.error) throw new Error(`Token refresh failed: ${data.error_description}`);

  tokenStore.save('teams', {
    accessToken: data.access_token,
    refreshToken: data.refresh_token || tokens.refreshToken,
    expiresAt: new Date(Date.now() + data.expires_in * 1000).toISOString(),
  });
  return data.access_token;
}

async function graphRequest(method, path, body) {
  const accessToken = await getValidToken();
  const res = await fetch(`${GRAPH_BASE}${path}`, {
    method,
    headers: { Authorization: `Bearer ${accessToken}`, 'Content-Type': 'application/json' },
    body: body ? JSON.stringify(body) : undefined,
  });
  const data = await res.json();
  if (!res.ok) throw new Error(`Graph API error: ${JSON.stringify(data.error)}`);
  return data;
}

async function subscribe(resource, notificationUrl) {
  const expiresAt = new Date(Date.now() + 55 * 60 * 1000).toISOString();
  const sub = await graphRequest('POST', '/subscriptions', {
    changeType: 'created',
    notificationUrl,
    resource,
    expirationDateTime: expiresAt,
    clientState: process.env.TEAMS_CLIENT_SECRET?.slice(0, 16) || 'weatherbox',
  });
  scheduleRenewal(sub.id, sub.expirationDateTime, resource, notificationUrl);
  console.log(`Teams subscription created: ${resource} → ${sub.id}`);
  return sub.id;
}

async function renew(subscriptionId, resource, notificationUrl) {
  const expiresAt = new Date(Date.now() + 55 * 60 * 1000).toISOString();
  try {
    await graphRequest('PATCH', `/subscriptions/${subscriptionId}`, { expirationDateTime: expiresAt });
    scheduleRenewal(subscriptionId, expiresAt, resource, notificationUrl);
    console.log(`Teams subscription renewed: ${subscriptionId}`);
  } catch (err) {
    console.error(`Renewal failed for ${subscriptionId}, recreating:`, err.message);
    activeSubscriptions.delete(subscriptionId);
    await subscribe(resource, notificationUrl);
  }
}

function scheduleRenewal(subscriptionId, expiresAtIso, resource, notificationUrl) {
  const delay = Math.max(new Date(expiresAtIso).getTime() - Date.now() - RENEWAL_BUFFER_MS, 0);
  const existing = activeSubscriptions.get(subscriptionId);
  if (existing?.timerId) clearTimeout(existing.timerId);
  const timerId = setTimeout(() => renew(subscriptionId, resource, notificationUrl), delay);
  activeSubscriptions.set(subscriptionId, { expiresAt: expiresAtIso, timerId });
}

/**
 * Call once on server start after Teams auth is confirmed.
 * Subscribes to all chats and any specific channels configured via
 * the TEAMS_CHANNEL_IDS env var (comma-separated "teamId/channelId" pairs).
 *
 * @param {string} notificationUrl - Full public HTTPS URL for POST /webhooks/teams
 */
async function createSubscriptions(notificationUrl) {
  // All 1:1 and group chats (Chat.Read delegated — no admin needed)
  await subscribe('/chats/getAllMessages', notificationUrl);

  // Optional: specific team channels from env var
  // Format: TEAMS_CHANNEL_IDS=teamId1/channelId1,teamId2/channelId2
  const channelIds = (process.env.TEAMS_CHANNEL_IDS || '').split(',').filter(Boolean);
  for (const entry of channelIds) {
    const [teamId, channelId] = entry.trim().split('/');
    if (teamId && channelId) {
      await subscribe(`/teams/${teamId}/channels/${channelId}/messages`, notificationUrl);
    }
  }
}

async function deleteAll() {
  for (const [id, { timerId }] of activeSubscriptions) {
    if (timerId) clearTimeout(timerId);
    try { await graphRequest('DELETE', `/subscriptions/${id}`); } catch { /* best-effort */ }
  }
  activeSubscriptions.clear();
}

module.exports = { createSubscriptions, deleteAll };
