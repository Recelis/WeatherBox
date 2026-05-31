/**
 * Manages Microsoft Graph change notification subscriptions for Teams channels.
 *
 * Subscriptions expire after a maximum of 60 minutes for delegated permissions,
 * so this service renews them automatically before expiry.
 */
const tokenStore = require('./tokenStore');

const GRAPH_BASE = 'https://graph.microsoft.com/v1.0';
const RENEWAL_BUFFER_MS = 5 * 60 * 1000; // renew 5 min before expiry

const activeSubscriptions = new Map(); // subscriptionId → { expiresAt, timerId }

async function graphRequest(method, path, body) {
  const tokens = tokenStore.get('teams');
  if (!tokens?.accessToken) throw new Error('Teams not authenticated');

  const res = await fetch(`${GRAPH_BASE}${path}`, {
    method,
    headers: {
      Authorization: `Bearer ${tokens.accessToken}`,
      'Content-Type': 'application/json',
    },
    body: body ? JSON.stringify(body) : undefined,
  });
  const data = await res.json();
  if (!res.ok) throw new Error(`Graph API error: ${JSON.stringify(data.error)}`);
  return data;
}

async function createSubscription(notificationUrl) {
  const expiresAt = new Date(Date.now() + 55 * 60 * 1000).toISOString(); // 55-minute window
  const sub = await graphRequest('POST', '/subscriptions', {
    changeType: 'created',
    notificationUrl,
    resource: '/teams/getAllMessages', // requires admin consent; use /chats/getAllMessages for 1:1
    expirationDateTime: expiresAt,
    clientState: process.env.TEAMS_CLIENT_SECRET?.slice(0, 16) || 'weatherbox',
  });
  scheduleRenewal(sub.id, sub.expirationDateTime, notificationUrl);
  activeSubscriptions.set(sub.id, { expiresAt: sub.expirationDateTime });
  console.log(`Teams subscription created: ${sub.id}`);
  return sub.id;
}

async function renewSubscription(subscriptionId, notificationUrl) {
  const expiresAt = new Date(Date.now() + 55 * 60 * 1000).toISOString();
  try {
    await graphRequest('PATCH', `/subscriptions/${subscriptionId}`, { expirationDateTime: expiresAt });
    scheduleRenewal(subscriptionId, expiresAt, notificationUrl);
    console.log(`Teams subscription renewed: ${subscriptionId}`);
  } catch (err) {
    console.error(`Failed to renew Teams subscription ${subscriptionId}:`, err.message);
    // Attempt to recreate
    activeSubscriptions.delete(subscriptionId);
    await createSubscription(notificationUrl);
  }
}

function scheduleRenewal(subscriptionId, expiresAtIso, notificationUrl) {
  const msUntilRenewal = new Date(expiresAtIso).getTime() - Date.now() - RENEWAL_BUFFER_MS;
  const existing = activeSubscriptions.get(subscriptionId);
  if (existing?.timerId) clearTimeout(existing.timerId);

  const timerId = setTimeout(() => renewSubscription(subscriptionId, notificationUrl), Math.max(msUntilRenewal, 0));
  activeSubscriptions.set(subscriptionId, { expiresAt: expiresAtIso, timerId });
}

async function deleteAll() {
  for (const [id, { timerId }] of activeSubscriptions) {
    if (timerId) clearTimeout(timerId);
    try { await graphRequest('DELETE', `/subscriptions/${id}`); } catch { /* best-effort */ }
  }
  activeSubscriptions.clear();
}

module.exports = { createSubscription, deleteAll };
