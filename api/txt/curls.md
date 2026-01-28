curl http://localhost:3600/ping

curl -X POST http://localhost:3600/users -H "Content-Type: application/json" -d '{"id": 42, "name": "Alice", "active": true}'

curl -X POST http://localhost:3600/ping

curl http://localhost:3600/asdf