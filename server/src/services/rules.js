const fs = require('fs');
const path = require('path');

const RULES_PATH = path.join(__dirname, '../../rules.json');

let rules = [];
let maxMessages = 8;
let deduplicateWindowSeconds = 300;

function load() {
  try {
    const raw = JSON.parse(fs.readFileSync(RULES_PATH, 'utf8'));
    rules = raw.rules || [];
    maxMessages = raw.maxMessages ?? 8;
    deduplicateWindowSeconds = raw.deduplicateWindowSeconds ?? 300;
    console.log(`Rules loaded: ${rules.length} rule(s)`);
  } catch (err) {
    console.error('Failed to load rules.json — all messages will be dropped:', err.message);
    rules = [];
  }
}

function save(updated) {
  fs.writeFileSync(RULES_PATH, JSON.stringify(updated, null, 2));
  load();
}

function getRules() {
  return { rules, maxMessages, deduplicateWindowSeconds };
}

/**
 * Evaluate a notification against the loaded rules.
 * Returns the matched rule (with priority applied) or null if no rule matches.
 *
 * Rule fields (all optional except source):
 *   source       — 'slack' | 'teams' | 'messenger' | 'iot'
 *   channel      — exact match on notification.channel (case-insensitive)
 *   sender       — exact match on notification.sender (case-insensitive)
 *   priority     — override priority to this value when rule matches
 *
 * Evaluation is top-down; first match wins.
 *
 * @param {import('./notificationStore').Notification} notification
 * @returns {{ priority: number } | null}
 */
function evaluate(notification) {
  for (const rule of rules) {
    if (rule.source && rule.source !== notification.source) continue;
    if (rule.channel && rule.channel.toLowerCase() !== notification.channel.toLowerCase()) continue;
    if (rule.sender && rule.sender.toLowerCase() !== notification.sender.toLowerCase()) continue;
    return { priority: rule.priority ?? notification.priority };
  }
  return null;
}

// Watch rules.json for changes so edits via the dashboard take effect immediately
fs.watchFile(RULES_PATH, { interval: 1000 }, () => {
  console.log('rules.json changed — reloading');
  load();
});

load();

module.exports = { load, save, getRules, evaluate };
