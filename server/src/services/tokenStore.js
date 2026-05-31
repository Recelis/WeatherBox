/**
 * Persists OAuth tokens to .tokens.json (gitignored).
 * Each platform stores whatever fields its OAuth response returns.
 * The file is read/written synchronously on startup and after each OAuth completion.
 */
const fs = require('fs');
const path = require('path');

const TOKEN_PATH = path.join(__dirname, '../../.tokens.json');

function load() {
  try {
    return JSON.parse(fs.readFileSync(TOKEN_PATH, 'utf8'));
  } catch {
    return {};
  }
}

function save(platform, data) {
  const all = load();
  all[platform] = { ...data, savedAt: new Date().toISOString() };
  fs.writeFileSync(TOKEN_PATH, JSON.stringify(all, null, 2));
}

function get(platform) {
  return load()[platform] || null;
}

function isConnected(platform) {
  const entry = get(platform);
  return entry != null && (entry.accessToken || entry.botToken);
}

module.exports = { save, get, isConnected };
