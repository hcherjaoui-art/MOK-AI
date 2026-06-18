// MockAI frontend prototype logic.
// This file simulates navigation, interview flow, and mock analytics without any backend calls.

const pages = document.querySelectorAll('[data-page]');
const navButtons = document.querySelectorAll('[data-nav]');
const topNavButtons = document.querySelectorAll('.nav-link');
const domainCards = document.querySelectorAll('.domain-card');
const beginInterviewBtn = document.getElementById('beginInterviewBtn');
const nextQuestionBtn = document.getElementById('nextQuestionBtn');
const questionText = document.getElementById('questionText');
const questionProgress = document.getElementById('questionProgress');
const interviewProgressText = document.getElementById('interviewProgressText');
const interviewDomainTitle = document.getElementById('interviewDomainTitle');
const domainTag = document.getElementById('domainTag');
const answerInput = document.getElementById('answerInput');
const finalScore = document.getElementById('finalScore');
const resultDomain = document.getElementById('resultDomain');
const strengthList = document.getElementById('strengthList');
const weaknessList = document.getElementById('weaknessList');
const recommendationList = document.getElementById('recommendationList');
const progressStats = document.getElementById('progressStats');
const sessionList = document.getElementById('sessionList');

const questionBank = {
  'Software Development': [
    'Tell me about a project where you solved a complex technical problem.',
    'How do you approach debugging when a production issue appears?',
    'Explain how you would design a scalable API for a student platform.',
    'What trade-offs do you consider when choosing between speed and maintainability?',
    'Describe a time you improved performance in a system you worked on.'
  ],
  'Cybersecurity': [
    'How would you explain the difference between authentication and authorization?',
    'What steps do you take to secure a web application from common attacks?',
    'How would you respond to a suspected account compromise?',
    'What is the principle of least privilege and why does it matter?',
    'Describe how you would assess the security of an exposed API endpoint.'
  ],
  'Data Science': [
    'How do you decide which model is most appropriate for a problem?',
    'Explain the difference between bias and variance in simple terms.',
    'How would you handle missing values in a dataset?',
    'Describe a data project where visualization changed the outcome.',
    'What metrics would you use to evaluate a classification model?'
  ],
  'Networking': [
    'Explain what happens when you type a website URL into a browser.',
    'How would you troubleshoot slow network performance for a team?',
    'What is the difference between TCP and UDP?',
    'How do VLANs help organize enterprise networks?',
    'Describe the role of DNS in a modern network.'
  ]
};

const mockSessions = [
  {
    domain: 'Software Development',
    date: 'Today',
    score: 87,
    summary: 'Strong technical reasoning with room for more concise storytelling.'
  },
  {
    domain: 'Data Science',
    date: 'Yesterday',
    score: 81,
    summary: 'Solid analytics answer structure and clear metric selection.'
  },
  {
    domain: 'Cybersecurity',
    date: '2 days ago',
    score: 76,
    summary: 'Demonstrated secure thinking but could expand threat mitigation detail.'
  },
  {
    domain: 'Networking',
    date: 'Last week',
    score: 84,
    summary: 'Good protocol understanding with confident troubleshooting steps.'
  }
];

const progressMetrics = [
  { label: 'Average Score', value: '82' },
  { label: 'Sessions Completed', value: '12' },
  { label: 'Best Domain', value: 'Software' },
  { label: 'Streak Days', value: '7' }
];

let selectedDomain = 'Software Development';
let currentQuestionIndex = 0;

// --- Simple auth UI / state (client-side mock) ---
const signInBtn = document.getElementById('signInBtn');
const authModal = document.getElementById('authModal');
const authClose = document.getElementById('authClose');
const loginForm = document.getElementById('loginForm');
const registerForm = document.getElementById('registerForm');
const showRegister = document.getElementById('showRegister');
const showLogin = document.getElementById('showLogin');

function setUserDisplay() {
  const user = JSON.parse(localStorage.getItem('mockai_user') || 'null');
  if (user && user.name) {
    signInBtn.textContent = user.name;
    signInBtn.classList.add('active');
  } else {
    signInBtn.textContent = 'Connexion';
    signInBtn.classList.remove('active');
  }
}

function openAuth(showLoginForm = true) {
  authModal.setAttribute('aria-hidden', 'false');
  authModal.style.display = 'flex';
  if (showLoginForm) {
    loginForm.style.display = '';
    registerForm.style.display = 'none';
  } else {
    loginForm.style.display = 'none';
    registerForm.style.display = '';
  }
}

function closeAuth() {
  authModal.setAttribute('aria-hidden', 'true');
  authModal.style.display = 'none';
}

signInBtn && signInBtn.addEventListener('click', () => {
  const user = JSON.parse(localStorage.getItem('mockai_user') || 'null');
  if (user && user.name) {
    // clicking while signed in signs out
    localStorage.removeItem('mockai_user');
    setUserDisplay();
    return;
  }
  openAuth(true);
});

authClose && authClose.addEventListener('click', closeAuth);
authModal && authModal.addEventListener('click', (e) => { if (e.target === authModal) closeAuth(); });
showRegister && showRegister.addEventListener('click', () => openAuth(false));
showLogin && showLogin.addEventListener('click', () => openAuth(true));

loginForm && loginForm.addEventListener('submit', (e) => {
  e.preventDefault();
  const email = document.getElementById('loginEmail').value.trim();
  const pwd = document.getElementById('loginPassword').value;
  // Mock authentication: accept any credentials and store a simple user object
  const user = { name: email.split('@')[0], email };
  localStorage.setItem('mockai_user', JSON.stringify(user));
  setUserDisplay();
  closeAuth();
});

registerForm && registerForm.addEventListener('submit', (e) => {
  e.preventDefault();
  const name = document.getElementById('regName').value.trim();
  const email = document.getElementById('regEmail').value.trim();
  const pwd = document.getElementById('regPassword').value;
  const user = { name: name || email.split('@')[0], email };
  localStorage.setItem('mockai_user', JSON.stringify(user));
  setUserDisplay();
  closeAuth();
});

setUserDisplay();

function showPage(pageId) {
  pages.forEach((page) => {
    page.classList.toggle('page-active', page.id === pageId);
  });

  topNavButtons.forEach((button) => {
    button.classList.toggle('active', button.dataset.nav === pageId || (pageId === 'interview' && button.dataset.nav === 'domains'));
  });

  window.scrollTo({ top: 0, behavior: 'smooth' });
}

function getCurrentQuestions() {
  return questionBank[selectedDomain] || questionBank['Software Development'];
}

function updateDomainSelection() {
  domainCards.forEach((card) => {
    card.classList.toggle('selected', card.dataset.domain === selectedDomain);
  });
}

function renderQuestion() {
  const questions = getCurrentQuestions();
  const questionNumber = currentQuestionIndex + 1;

  questionText.textContent = questions[currentQuestionIndex];
  interviewDomainTitle.textContent = `${selectedDomain} Interview`;
  domainTag.textContent = selectedDomain;
  interviewProgressText.textContent = `Question ${questionNumber} of ${questions.length}`;
  questionProgress.style.width = `${(questionNumber / questions.length) * 100}%`;
  answerInput.value = '';
}

function buildResultLists() {
  const strengths = [
    'Clear structure and logical flow in responses',
    'Good technical vocabulary for the chosen domain',
    'Balanced confidence with practical examples'
  ];

  const weaknesses = [
    'Could include more measurable outcomes',
    'Some answers can be more concise',
    'Add stronger examples of collaboration or impact'
  ];

  const recommendations = [
    'Use the STAR method to structure future responses.',
    'Practice summarizing complex ideas in 60 to 90 seconds.',
    'Add metrics, outcomes, and stakeholder impact to every answer.'
  ];

  strengthList.innerHTML = strengths.map((item) => `<li>${item}</li>`).join('');
  weaknessList.innerHTML = weaknesses.map((item) => `<li>${item}</li>`).join('');
  recommendationList.innerHTML = recommendations.map((item) => `<li>${item}</li>`).join('');
}

function renderDashboard() {
  progressStats.innerHTML = progressMetrics
    .map((metric) => `
      <div class="stat-card">
        <strong>${metric.value}</strong>
        <span>${metric.label}</span>
      </div>
    `)
    .join('');

  sessionList.innerHTML = mockSessions
    .map((session) => `
      <div class="session-item">
        <div class="session-topic">
          <strong>${session.domain}</strong>
          <span>${session.summary}</span>
        </div>
        <div class="session-meta">${session.date}</div>
        <div class="session-score">${session.score}/100</div>
      </div>
    `)
    .join('');
}

function calculateMockScore(answer, domain, questionIndex) {
  const lengthScore = Math.min(24, Math.round(answer.trim().split(/\s+/).filter(Boolean).length * 1.8));
  const structureScore = 18 + questionIndex * 2;
  const domainBoost = domain === 'Software Development' ? 18 : domain === 'Data Science' ? 16 : domain === 'Cybersecurity' ? 15 : 17;
  const clarityScore = answer.length > 60 ? 14 : 8;
  return Math.min(100, 45 + lengthScore + structureScore + domainBoost + clarityScore);
}

function goToResults() {
  const answer = answerInput.value || 'I would approach this by outlining the problem, gathering context, and presenting a clear solution.';
  const score = calculateMockScore(answer, selectedDomain, currentQuestionIndex);

  finalScore.textContent = score;
  resultDomain.textContent = selectedDomain;
  buildResultLists();
  showPage('results');
}

navButtons.forEach((button) => {
  button.addEventListener('click', (event) => {
    const target = button.dataset.nav;

    if (button.tagName === 'A') {
      event.preventDefault();
    }

    if (target === 'domains') {
      showPage('domains');
      return;
    }

    if (target === 'landing') {
      showPage('landing');
      return;
    }

    if (target === 'history') {
      renderDashboard();
      showPage('history');
    }
  });
});

domainCards.forEach((card) => {
  card.addEventListener('click', () => {
    selectedDomain = card.dataset.domain;
    currentQuestionIndex = 0;
    updateDomainSelection();
  });
});

beginInterviewBtn.addEventListener('click', () => {
  currentQuestionIndex = 0;
  showPage('interview');
  renderQuestion();
});

nextQuestionBtn.addEventListener('click', () => {
  const questions = getCurrentQuestions();

  if (currentQuestionIndex < questions.length - 1) {
    currentQuestionIndex += 1;
    renderQuestion();
    answerInput.focus();
    return;
  }

  goToResults();
});

updateDomainSelection();
renderDashboard();
showPage('landing');