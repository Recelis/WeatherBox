# WeatherBox Server

Node.js bridge that receives webhooks from Slack, MS Teams, and Facebook Messenger, applies notification rules, and forwards messages to the ESP32 via AWS IoT MQTT.

---

## Prerequisites

- Node.js 18+
- A Raspberry Pi (or any always-on Linux machine) on your local network
- AWS IoT Thing certificates (already in your project's `cdk/` stack)

---

## Setup

### 1. Install dependencies

```bash
cd server
npm install
```

### 2. Configure environment

```bash
cp .env.example .env
```

Edit `.env` and fill in every value. See the per-platform sections below for where to find each credential.

### 3. Add AWS IoT certificates

Create a `server/certs/` directory (gitignored) and copy the three certificate files downloaded when you created your IoT Thing:

```
server/certs/
  AmazonRootCA1.pem
  device.pem.crt
  private.pem.key
```

Update the paths in `.env` if you put them elsewhere.

### 4. Start the server

```bash
# Development (auto-restart on changes)
npm run dev

# Production
npm start
```

The server starts on `http://localhost:3000`. Visit `/health` to confirm it's running.

---

## Exposing to the internet (required for webhooks)

Slack, Teams, and Messenger all require a **public HTTPS URL** to deliver webhook events. The recommended approach for a Pi at home is **Cloudflare Tunnel** — free, no port forwarding, automatic HTTPS.

### Cloudflare Tunnel setup

**One-time install** (on the Pi):

```bash
# Download and install cloudflared
curl -L https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm64.deb -o cloudflared.deb
sudo dpkg -i cloudflared.deb
```

**Start a temporary tunnel** (for testing — URL changes on each run):

```bash
cloudflared tunnel --url http://localhost:3000
```

Copy the `https://xxxx.trycloudflare.com` URL it prints — use this as your webhook base URL when registering with each platform.

**Persistent tunnel** (recommended for production — same URL every time):

1. Log in: `cloudflared tunnel login`
2. Create a named tunnel: `cloudflared tunnel create weatherbox`
3. Create `~/.cloudflared/config.yml`:
   ```yaml
   tunnel: weatherbox
   credentials-file: /home/pi/.cloudflared/<tunnel-id>.json
   ingress:
     - hostname: weatherbox.yourdomain.com
       service: http://localhost:3000
     - service: http_status:404
   ```
4. Route DNS: `cloudflared tunnel route dns weatherbox weatherbox.yourdomain.com`
5. Run: `cloudflared tunnel run weatherbox`

Your webhook base URL will be `https://weatherbox.yourdomain.com`.

---

## Running as a systemd service (auto-start on boot)

Create `/etc/systemd/system/weatherbox.service`:

```ini
[Unit]
Description=WeatherBox notification bridge
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/WeatherBox/server
ExecStart=/usr/bin/node src/index.js
Restart=on-failure
RestartSec=5
EnvironmentFile=/home/pi/WeatherBox/server/.env

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable weatherbox
sudo systemctl start weatherbox
sudo systemctl status weatherbox
```

Do the same for the Cloudflare tunnel if you want it to auto-start too:

```bash
sudo cloudflared service install
sudo systemctl enable cloudflared
sudo systemctl start cloudflared
```

---

## Platform setup guides

### Slack

1. Go to [api.slack.com/apps](https://api.slack.com/apps) → **Create New App** → **From scratch**
2. Name it (e.g. "WeatherBox"), pick your workspace
3. **OAuth & Permissions** → Scopes → Bot Token Scopes → add:
   - `channels:history`, `channels:read`, `groups:history`, `im:history`
4. **Install to Workspace** → copy the **Bot User OAuth Token** → set `SLACK_BOT_TOKEN` in `.env`
5. **Basic Information** → App Credentials → copy **Signing Secret** → set `SLACK_SIGNING_SECRET`
6. **Event Subscriptions** → Enable Events → Request URL: `https://your-tunnel/webhooks/slack`
7. Subscribe to bot events: `message.channels`, `message.im`
8. Reinstall the app when prompted

### MS Teams

> **No admin consent required** — this integration uses delegated permissions (reads messages as you, not as a service account).

1. Go to [portal.azure.com](https://portal.azure.com) → **Azure Active Directory** → **App registrations** → **New registration**
2. Name it (e.g. "WeatherBox"), Supported account types: **Single tenant**
3. Redirect URI (Web): `https://your-tunnel/auth/teams/callback`
4. Copy **Application (client) ID** → `TEAMS_CLIENT_ID`, **Directory (tenant) ID** → `TEAMS_TENANT_ID`
5. **Certificates & secrets** → **New client secret** → copy value → `TEAMS_CLIENT_SECRET`
6. **API permissions** → Add the following **Delegated** Microsoft Graph permissions:
   - `Chat.Read` — all your 1:1 and group chats (no admin consent needed)
   - `ChannelMessage.Read` — specific team channels you configure (no admin consent needed)
   - `offline_access` — allows token refresh without re-login
7. Visit `https://your-tunnel/auth/teams` and sign in with your Teams account
8. **Optional — Teams channel messages**: To also receive messages from specific channels (not just chats), find the team ID and channel ID via [Graph Explorer](https://developer.microsoft.com/en-us/graph/graph-explorer) (`GET /me/joinedTeams`, then `GET /teams/{id}/channels`) and add them to `TEAMS_CHANNEL_IDS` in `.env`

### Facebook Messenger

1. Go to [developers.facebook.com](https://developers.facebook.com) → **My Apps** → **Create App** → **Business**
2. Add **Messenger** product
3. **Settings** → copy **App ID** → `MESSENGER_APP_ID`, **App Secret** → `MESSENGER_APP_SECRET`
4. Choose a verify token (any string) → `MESSENGER_VERIFY_TOKEN`
5. **Webhooks** → **Add Callback URL**: `https://your-tunnel/webhooks/messenger`, paste verify token
6. Subscribe to: `messages`, `messaging_postbacks`
7. Generate a **Page Access Token** for your page (used in the Messenger integration PR)

---

## Dashboard

Once the server is running:

| URL | Description |
|-----|-------------|
| `/health` | Server health check |
| `/auth/status` | Which platforms are connected |
| `/dashboard/rules` | Rules API (GET/POST) |
| `/dashboard/feed` | Recent notifications (JSON) |

Browser UI pages for rules management and live feed are added in later PRs (AC9, AC10).
