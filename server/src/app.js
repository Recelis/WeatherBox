const express = require('express');
const path = require('path');

const webhookRoutes = require('./routes/webhooks');
const authRoutes = require('./routes/auth');
const dashboardRoutes = require('./routes/dashboard');

const app = express();

app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(express.static(path.join(__dirname, '../public')));

app.use('/webhooks', webhookRoutes);
app.use('/auth', authRoutes);
app.use('/dashboard', dashboardRoutes);

app.get('/health', (_req, res) => res.json({ status: 'ok' }));

module.exports = app;
