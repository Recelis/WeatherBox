/**
 * @typedef {Object} Notification
 * @property {string} id
 * @property {'slack'|'teams'|'messenger'|'iot'} source
 * @property {string} sender
 * @property {string} channel
 * @property {string} preview
 * @property {string} timestamp
 * @property {0|1|2|3} priority
 * @property {Object} [metadata]
 */

const MAX_STORED = 50;
const DEDUPE_WINDOW_MS = 5 * 60 * 1000;

/** @type {Notification[]} */
const store = [];
const seenIds = new Map(); // id → timestamp (ms)

/**
 * Add a notification if it passes deduplication.
 * Returns true if the notification was accepted, false if it was a duplicate.
 * @param {Notification} notification
 * @returns {boolean}
 */
function add(notification) {
  const now = Date.now();

  // Evict expired dedupe entries
  for (const [id, ts] of seenIds) {
    if (now - ts > DEDUPE_WINDOW_MS) seenIds.delete(id);
  }

  if (seenIds.has(notification.id)) return false;

  seenIds.set(notification.id, now);
  store.unshift(notification);
  if (store.length > MAX_STORED) store.pop();
  return true;
}

/**
 * Return the N most recent notifications, sorted by priority desc then timestamp desc.
 * @param {number} [limit=8]
 * @returns {Notification[]}
 */
function getRecent(limit = 8) {
  return [...store]
    .sort((a, b) => b.priority - a.priority || new Date(b.timestamp) - new Date(a.timestamp))
    .slice(0, limit);
}

/** @returns {Notification[]} */
function getAll() {
  return [...store];
}

module.exports = { add, getRecent, getAll };
