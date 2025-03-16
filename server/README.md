curl -X POST http://192.168.178.100:7777/api/v1/data \
     -H "X-API-Token: 1234567890" \
     -H "Content-Type: application/json" \
     -d '{"measurements": [{"timestamp": 1632345678, "value": 12.34}]}'

