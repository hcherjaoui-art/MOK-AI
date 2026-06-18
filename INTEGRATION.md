# Integration / API Contract (placeholder)

This file lists a minimal REST contract Ahmed can implement so the frontend can call real endpoints. Replace mock data in `script.js` with fetch calls to these endpoints.

Base URL example: `https://api.example.com` (PLACEHOLDER)

Endpoints

- GET /domains
  - Response: 200
  - Body: ["Software Development","Data Science","Cybersecurity","Networking"]

- GET /questions?domain={domain}
  - Response: 200
  - Body: { "domain": "Software Development", "questions": ["q1","q2", ...] }

- POST /sessions
  - Create a recorded interview session
  - Request: {
      "domain": "Software Development",
      "answers": ["answer1","answer2", ...],
      "metadata": { "user": "student@example.com", "duration": 420 }
    }
  - Response: 201
  - Body: { "sessionId": "uuid", "score": 87, "createdAt": "2026-06-18T...Z" }

- GET /sessions
  - List previous sessions
  - Response: 200
  - Body: [{ "sessionId":"...", "domain":"...","score":87,"summary":"...","date":"..." }, ...]

Integration notes for frontend changes

- Replace `mockSessions` in `script.js` with a GET `/sessions` call and populate session list.
- Replace `questionBank` with fetching `/questions?domain=...` when a domain is selected.
- When interview completes, POST `/sessions` with collected answers and use returned `score` to show results.
- Keep CORS in mind: ensure the backend sends `Access-Control-Allow-Origin: *` (or restrict to the deployed frontend origin).

Sample fetch pattern (pseudo):

```js
const res = await fetch(`${API_BASE}/questions?domain=${encodeURIComponent(domain)}`);
const data = await res.json();
// use data.questions
```
