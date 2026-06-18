MockAI mock server

This is a tiny mock backend to test the MockAI frontend.

Requirements
- Node.js 24+ or any Node version that exposes `node:sqlite`.

Install and run

```powershell
cd mock_server
npm install
npm start
```

The server exposes these endpoints:
- `GET /domains` -> array of domain names
- `GET /questions?domain=...` -> { domain, questions }
- `POST /auth/register` -> { token, user }
- `POST /auth/login` -> { token, user }
- `POST /sessions` -> stores a session and returns it
- `GET /sessions` -> list of stored sessions

CORS is enabled and the server listens on port 8080 by default.

Database integration
- The server will attempt to open an existing `mockai.db` SQLite database. By default it looks in these locations (in order):
	- `mock_server/../mockai.db`
	- `mock_server/../../mockai.db`
- You can override the DB path with the `DB_PATH` environment variable, e.g.: `DB_PATH=C:\path\to\mockai.db npm start`.
- When a DB is present the endpoints will persist `users`, `interviews`, `questions`, `answers`, and `results` into the real database used by the C backend.

Implementation note
- The server now uses Node's built-in `node:sqlite` module instead of `better-sqlite3`, so there is no native addon to compile on Windows.
- If you previously installed dependencies, rerun `npm install` once so the lockfile and local `node_modules` match the reduced dependency set.
