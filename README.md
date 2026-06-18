# MockAI – Backend C

Plateforme intelligente de simulation d'entretiens basée sur l'IA.

## Architecture du projet

```
mockai/
├── include/          # En-têtes (interfaces publiques)
│   ├── types.h       # Structures et types communs
│   ├── user.h        # Gestion des utilisateurs
│   ├── session.h     # Gestion des sessions
│   ├── interview.h   # Gestion des entretiens
│   ├── score.h       # Gestion des scores
│   ├── stats.h       # Statistiques utilisateur
│   ├── api.h         # Communication IA (libcurl)
│   └── json_parser.h # Parsing des réponses JSON
│
├── src/              # Implémentations
│   ├── main.c        # Point d'entrée
│   ├── user.c        # Logique métier – utilisateurs
│   ├── session.c     # Logique métier – sessions
│   ├── interview.c   # Logique métier + validation entrées
│   ├── score.c       # Calcul et stockage des scores
│   ├── stats.c       # Agrégations statistiques
│   ├── api.c         # Requêtes REST via libcurl
│   └── json_parser.c # Parsing / construction JSON
│
├── tests/            # Tests unitaires
│   ├── test.h        # Framework de test maison
│   ├── test_user.c   # Tests module user
│   ├── test_core.c   # Tests session/interview/score/json
│   └── db_stub.c     # Stub DB pour tests isolés
│
├── docs/             # Documentation
│   ├── README.md     # Ce fichier
│   └── API.md        # Contrat d'interface avec le frontend
│
└── Makefile
```

## Responsabilités

| Membre | Rôle |
|--------|------|
| **Ghali** | Backend C, structures, API REST, logique métier, tests |
| **Ahmed** | SQLite (local), Supabase (cloud), sécurité DB |

## Dépendances

- **libcurl** – Communication HTTP avec l'API Gemini/OpenAI
- **libsqlite3** – Base de données locale (couche Ahmed)
- **libm** – Calculs flottants (stats)

### Installation (Ubuntu/Debian)

```bash
sudo apt install libcurl4-openssl-dev libsqlite3-dev
```

## Compilation

```bash
# Binaire principal
make

# Mode debug (AddressSanitizer + UBSan)
make debug

# Tests unitaires
make test

# Nettoyage
make clean
```

## Configuration

```bash
export MOCKAI_API_KEY="votre_clé_gemini_ici"
./mockai
```

## Flux d'une session

```
1. user_register() / user_login()
      ↓
2. session_start(domain, position)
      ↓
3. api_generate_questions()  → libcurl → Gemini API
      ↓
4. [Utilisateur répond]
      ↓
5. interview_save_answer()   → validation + sanitize
      ↓
6. api_evaluate_session()    → libcurl → Gemini API
      ↓
7. score_save()
      ↓
8. user_update_stats()
      ↓
9. stats_to_json()           → Frontend (Ahmed)
```

## Sécurité des données

- Validation de toutes les entrées utilisateur (`interview_validate_answer`, `interview_sanitize_answer`)
- Échappement JSON avant envoi à l'API (`json_escape_string`)
- Hash du mot de passe avant stockage
- Effacement mémoire des données sensibles avant `free()`
- Double protection : sanitize côté C + paramètres préparés côté DB (Ahmed)

## Tests

Les tests unitaires couvrent :
- Validation email/password
- Hash déterministe des mots de passe
- Gestion des tokens
- Calcul de scores et moyennes
- Labels de notes
- Parsing JSON (extraction, erreurs, cas limites)
- Sanitisation des réponses utilisateur
- Robustesse aux pointeurs NULL
