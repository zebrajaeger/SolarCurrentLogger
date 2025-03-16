const express = require('express');
const axios = require('axios');

const app = express();
app.use(express.json()); // Middleware to parse JSON

const PORT = process.env.PORT || 7777;
const INFLUXDB_URL = process.env.INFLUXDB_URL || 'http://localhost:8086';
const INFLUXDB_BUCKET = process.env.INFLUXDB_BUCKET || 'mybucket';
const INFLUXDB_ORG = process.env.INFLUXDB_ORG || 'myorg';
const INFLUXDB_TOKEN = process.env.INFLUXDB_TOKEN || '1234567890';
const API_TOKEN = process.env.API_TOKEN || '1234567890'; // API token from environment

// Middleware for token verification
app.use((req, res, next) => {
  const clientToken = req.header('X-API-Token');

  if (!clientToken) {
    return res.status(401).json({ error: 'API token missing' });
  }

  if (clientToken !== API_TOKEN) {
    return res.status(403).json({ error: 'Invalid API token' });
  }

  next();
});

app.post('/api/v1/data', async (req, res) => {
  console.log('Received JSON data:', req.body);

  if (!req.body.measurements) {
    return res.status(400).json({ error: 'Invalid payload' });
  }

  let lines = req.body.measurements.map(m => {
    return `current value=${m.value} ${m.timestamp}`;
  });

  let data = lines.join('\n');
  console.log("Line Protocol Data:\n", data);

  try {
    const response = await axios.post(
      `${INFLUXDB_URL}/api/v2/write?org=${INFLUXDB_ORG}&bucket=${INFLUXDB_BUCKET}&precision=ms`,
      data,
      {
        headers: {
          'Content-Type': 'text/plain',
          'Authorization': `Token ${INFLUXDB_TOKEN}`
        }
      }
    );
    console.log('Data sent to InfluxDB:', response.statusText);
    res.sendStatus(200);
  } catch (error) {
    console.error('Error sending data to InfluxDB:', error.response ? error.response.data : error);
    res.status(500).json({ error: error.toString() });
  }
});

app.listen(PORT, '0.0.0.0', () => {
  console.log(`Server is running on port ${PORT}`);
  console.log(`InfluxDB URL: ${INFLUXDB_URL}`);
  console.log(`InfluxDB Bucket: ${INFLUXDB_BUCKET}`);
  console.log(`InfluxDB Org: ${INFLUXDB_ORG}`);
  console.log(`API Token: ${API_TOKEN}`);
});
