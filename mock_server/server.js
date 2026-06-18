const express = require('express');
const cors = require('cors');
const { v4: uuidv4 } = require('uuid');
const initSqlJs = require('sql.js');
const fs = require('fs');
const path = require('path');

const app = express();
app.use(cors());
app.use(express.json());

const PORT = process.env.PORT || 8080;
const uiRoot = path.join(__dirname, '..');
app.use(express.static(uiRoot));

const QUESTIONS = {
  'Développement': [
    "Parlez d'un projet où vous avez résolu un problème technique complexe.",
    "Comment abordez-vous le débogage lorsqu'un incident se produit en production ?"
  ],
  'Cybersécurité': [
    'Expliquez la différence entre authentification et autorisation.',
    'Quelles mesures prenez-vous pour sécuriser une application web ?'
  ],
  'Data Science': [
    'Comment choisissez-vous le modèle le plus adapté ?',
    'Expliquez la différence entre biais et variance.'
  ],
  'Réseaux': [
    'Que se passe-t-il lorsque vous saisissez une URL dans un navigateur ?',
    'Quelle est la différence entre TCP et UDP ?'
  ]
};

const TOKEN_USERS = new Map();
const MEMORY_USERS = [];
const MEMORY_SESSIONS = [];

function resolveDbPath() {
  if (process.env.DB_PATH) return process.env.DB_PATH;
  const p1 = path.join(__dirname, '..', 'mockai.db');
  const p2 = path.join(__dirname, '..', '..', 'mockai.db');
  if (fs.existsSync(p1)) return p1;
  if (fs.existsSync(p2)) return p2;
  return p1;
}

function ensureParentDir(filePath) {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
}

// sql.js based database wrapper (pure JS, no native builds)
let SQL = null;
let db = null;
function saveDbToFile(dbFile) {
  try {
    const data = db.export();
    const buffer = Buffer.from(data);
    fs.writeFileSync(dbFile, buffer);
  } catch (e) {
    console.error('failed to save db', e);
  }
}

function all(sql, params) {
  const stmt = db.prepare(sql);
  try {
    if (params && params.length) stmt.bind(params);
    const rows = [];
    while (stmt.step()) rows.push(stmt.getAsObject());
    return rows;
  } finally {
    stmt.free();
  }
}

function get(sql, params) {
  const rows = all(sql, params);
  return rows && rows.length ? rows[0] : undefined;
}

function run(sql, params) {
  // sql.js doesn't expose lastInsertRowid easily; use exec for simple runs
  db.run(sql, params || []);
}

async function openDatabase(filePath) {
  ensureParentDir(filePath);
  if (!SQL) SQL = await initSqlJs({});
  let filebuffer = null;
  if (fs.existsSync(filePath)) filebuffer = fs.readFileSync(filePath);
  if (filebuffer) db = new SQL.Database(new Uint8Array(filebuffer));
  else db = new SQL.Database();

  // Enable foreign keys and create schema if missing
  db.run('PRAGMA foreign_keys = ON;');
  db.run(`
    CREATE TABLE IF NOT EXISTS users (
      id            INTEGER PRIMARY KEY AUTOINCREMENT,
      username      TEXT    NOT NULL UNIQUE,
      email         TEXT    NOT NULL UNIQUE,
      password_hash TEXT    NOT NULL,
      created_at    TEXT    NOT NULL DEFAULT (datetime('now')),
      updated_at    TEXT    NOT NULL DEFAULT (datetime('now'))
    );

    CREATE TABLE IF NOT EXISTS interviews (
      id         INTEGER PRIMARY KEY AUTOINCREMENT,
      user_id    INTEGER NOT NULL,
      domain     TEXT    NOT NULL,
      position   TEXT,
      status     TEXT    NOT NULL DEFAULT 'pending',
      started_at TEXT    NOT NULL DEFAULT (datetime('now')),
      ended_at   TEXT
    );

    CREATE TABLE IF NOT EXISTS questions (
      id            INTEGER PRIMARY KEY AUTOINCREMENT,
      interview_id  INTEGER NOT NULL,
      question_text TEXT    NOT NULL,
      order_index   INTEGER NOT NULL DEFAULT 0,
      created_at    TEXT    NOT NULL DEFAULT (datetime('now'))
    );

    CREATE TABLE IF NOT EXISTS answers (
      id           INTEGER PRIMARY KEY AUTOINCREMENT,
      question_id  INTEGER NOT NULL,
      interview_id INTEGER NOT NULL,
      answer_text  TEXT    NOT NULL,
      submitted_at TEXT    NOT NULL DEFAULT (datetime('now'))
    );

    CREATE TABLE IF NOT EXISTS results (
      id           INTEGER PRIMARY KEY AUTOINCREMENT,
      interview_id INTEGER NOT NULL UNIQUE,
      score        REAL    NOT NULL,
      strengths    TEXT    NOT NULL,
      weaknesses   TEXT    NOT NULL,
      advice       TEXT    NOT NULL,
      raw_ai_resp  TEXT,
      created_at   TEXT    NOT NULL DEFAULT (datetime('now'))
    );
  `);

  // persist initial DB to file
  saveDbToFile(filePath);
  console.log('Opened/created SQLite file at', filePath);
  return { filePath };
}

// Immediately open DB asynchronously
const resolvedPath = resolveDbPath();
openDatabase(resolvedPath).then(() => console.log('DB ready at', resolvedPath)).catch((e) => console.warn('DB open failed', e.message));

function ensureUser(username, email, passwordHash = 'local') {
  const safeUsername = username || (email ? email.split('@')[0] : 'guest');
  const safeEmail = email || 'guest@mockai.local';

  if (db) {
    const existing = get('SELECT id, username, email FROM users WHERE email = ?', [safeEmail]);
    if (existing) return existing;

    run('INSERT OR IGNORE INTO users (username, email, password_hash) VALUES (?, ?, ?)', [safeUsername, safeEmail, passwordHash]);

    let created = get('SELECT id, username, email FROM users WHERE email = ?', [safeEmail]);
    if (created) return created;

    const fallbackUsername = `${safeUsername}-${uuidv4().slice(0, 8)}`;
    run('INSERT OR IGNORE INTO users (username, email, password_hash) VALUES (?, ?, ?)', [fallbackUsername, safeEmail, passwordHash]);
    created = get('SELECT id, username, email FROM users WHERE email = ?', [safeEmail]);
    if (created) {
      saveDbToFile(resolvedPath);
      return created;
    }
  }

  return { id: uuidv4(), username: safeUsername, email: safeEmail };
}

function getTokenUser(req) {
  const authHeader = req.headers.authorization || '';
  const token = authHeader.startsWith('Bearer ') ? authHeader.slice(7) : null;
  return token && TOKEN_USERS.has(token) ? TOKEN_USERS.get(token) : null;
}

function scoreAnswers(answers) {
  const total = (answers || []).reduce((sum, answer) => {
    if (typeof answer === 'string') return sum + answer.length;
    return sum + ((answer && answer.answer) ? answer.answer.length : 0);
  }, 0);
  return Math.min(100, Math.round(total / ((answers || []).length || 1)));
}

function getAnswerText(answer) {
  if (typeof answer === 'string') return answer;
  if (answer && typeof answer === 'object') return answer.answer || '';
  return '';
}

function getQuestionText(answer, index) {
  if (answer && typeof answer === 'object' && answer.question) return answer.question;
  return `Question ${index + 1}`;
}

app.get('/domains', (req, res) => {
  if (db) {
    try {
      const rows = all('SELECT DISTINCT domain FROM interviews ORDER BY domain');
      const domains = rows.map((row) => row.domain).filter(Boolean);
      if (domains.length) return res.json(domains);
    } catch (error) {
      console.error('domains query failed', error);
    }
  }

  res.json(Object.keys(QUESTIONS));
});

app.get('/questions', (req, res) => {
  const domain = req.query.domain || 'Développement';
  if (db) {
    try {
      const rows = all(
        'SELECT q.question_text FROM questions q JOIN interviews i ON q.interview_id = i.id WHERE i.domain = ? ORDER BY q.created_at DESC LIMIT 10',
        [domain]
      );
      if (rows.length) return res.json({ domain, questions: rows.map((row) => row.question_text) });
    } catch (error) {
      console.error('questions query failed', error);
    }
  }

  res.json({ domain, questions: QUESTIONS[domain] || QUESTIONS['Développement'] });
});

app.post('/auth/register', (req, res) => {
  const { username, email, password } = req.body;
  const token = 'token-' + uuidv4();
  const user = { ...ensureUser(username, email, 'local'), password };
  MEMORY_USERS.push(user);
  TOKEN_USERS.set(token, user);
  res.json({ token, user: { id: user.id, username: user.username, email: user.email } });
});

app.post('/auth/login', (req, res) => {
  const { email, password } = req.body;
  const token = 'token-' + uuidv4();
  const user = { ...ensureUser(email ? email.split('@')[0] : 'guest', email || 'guest@mockai.local', 'local'), password };
  MEMORY_USERS.push(user);
  TOKEN_USERS.set(token, user);
  res.json({ token, user: { id: user.id, username: user.username, email: user.email } });
});

app.post('/sessions', (req, res) => {
  const { domain, answers, metadata } = req.body;
  const tokenUser = getTokenUser(req);
  const userEmail = (metadata && (metadata.user || metadata.email)) || (tokenUser && tokenUser.email) || 'guest@mockai.local';
  const userName = (metadata && metadata.username) || (tokenUser && tokenUser.username) || (userEmail ? userEmail.split('@')[0] : 'guest');
  const user = ensureUser(userName, userEmail, 'local');

  if (db) {
    try {
      run('INSERT INTO interviews (user_id, domain, position, status) VALUES (?, ?, ?, ?)', [user.id, domain || 'Développement', (metadata && metadata.position) || null, 'completed']);
      const lastInterview = all('SELECT id FROM interviews ORDER BY id DESC LIMIT 1');
      const interviewId = lastInterview && lastInterview[0] ? Number(lastInterview[0].id) : null;

      if (Array.isArray(answers) && interviewId) {
        answers.forEach((answer, index) => {
          const questionText = getQuestionText(answer, index);
          const answerText = getAnswerText(answer);
          run('INSERT INTO questions (interview_id, question_text, order_index) VALUES (?, ?, ?)', [interviewId, questionText, index]);
          const lastQ = all('SELECT id FROM questions ORDER BY id DESC LIMIT 1');
          const questionId = lastQ && lastQ[0] ? Number(lastQ[0].id) : null;
          if (questionId) run('INSERT INTO answers (question_id, interview_id, answer_text) VALUES (?, ?, ?)', [questionId, interviewId, answerText]);
        });
      }

      const score = scoreAnswers(answers);
      run('INSERT INTO results (interview_id, score, strengths, weaknesses, advice, raw_ai_resp) VALUES (?, ?, ?, ?, ?, ?)',
        [interviewId, score, JSON.stringify(['Clarté', 'Structure']), JSON.stringify(['Plus de métriques']), JSON.stringify(['Utiliser STAR']), 'generated-by-mock']
      );

      // persist DB file
      saveDbToFile(resolvedPath);

      return res.status(201).json({
        sessionId: interviewId,
        domain: domain || 'Développement',
        score,
        createdAt: new Date().toISOString()
      });
    } catch (error) {
      console.error('failed to persist session', error);
    }
  }

  const score = scoreAnswers(answers);
  const session = {
    sessionId: uuidv4(),
    domain,
    answers,
    metadata,
    score,
    createdAt: new Date().toISOString()
  };
  MEMORY_SESSIONS.push(session);
  res.status(201).json(session);
});

app.get('/sessions', (req, res) => {
  if (db) {
    try {
      const rows = all(
        'SELECT i.id as sessionId, i.domain, r.score, r.created_at as date, r.raw_ai_resp as summary FROM interviews i JOIN results r ON r.interview_id = i.id ORDER BY r.created_at DESC LIMIT 20'
      );
      return res.json(rows.map((row) => ({
        sessionId: row.sessionId,
        domain: row.domain,
        date: row.date,
        score: row.score,
        summary: row.summary
      })));
    } catch (error) {
      console.error('sessions query failed', error);
    }
  }

  res.json(MEMORY_SESSIONS.map((session) => ({
    domain: session.domain,
    date: session.createdAt,
    score: session.score,
    summary: (session.answers && session.answers[0]) || ''
  })));
});

app.get('*', (req, res, next) => {
  if (req.path.startsWith('/auth') || req.path.startsWith('/domains') || req.path.startsWith('/questions') || req.path.startsWith('/sessions')) {
    return next();
  }

  res.sendFile(path.join(uiRoot, 'index.html'));
});

app.listen(PORT, () => {
  console.log(`MockAI mock server listening on http://localhost:${PORT}`);
});
