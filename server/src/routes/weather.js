const { Router } = require('express');
const { fetchWeather } = require('../services/weatherPoller');

const router = Router();

// Manual trigger — useful for testing the full pipeline without waiting an hour.
// Returns the notification that was published so you can inspect the format.
router.post('/fetch', async (_req, res) => {
  try {
    const notification = await fetchWeather();
    res.json({ ok: true, published: notification });
  } catch (err) {
    res.status(500).json({ ok: false, error: err.message });
  }
});

module.exports = router;
