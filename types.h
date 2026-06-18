#ifndef MOCKAI_TYPES_H
#define MOCKAI_TYPES_H

#include <time.h>

/* ─── Constantes ─────────────────────────────────────────── */
#define MAX_NAME_LEN       128
#define MAX_EMAIL_LEN      256
#define MAX_PASSWORD_LEN   256
#define MAX_DOMAIN_LEN     64
#define MAX_QUESTION_LEN   1024
#define MAX_ANSWER_LEN     4096
#define MAX_FEEDBACK_LEN   2048
#define MAX_TOKEN_LEN      512
#define MAX_QUESTIONS      20

/* ─── Codes de retour ────────────────────────────────────── */
typedef enum {
    MOCKAI_OK           =  0,
    MOCKAI_ERR_MEMORY   = -1,
    MOCKAI_ERR_DB       = -2,
    MOCKAI_ERR_API      = -3,
    MOCKAI_ERR_INPUT    = -4,
    MOCKAI_ERR_AUTH     = -5,
    MOCKAI_ERR_NOTFOUND = -6,
    MOCKAI_ERR_JSON     = -7
} MockAIStatus;

/* ─── Domaines d'entretien ───────────────────────────────── */
typedef enum {
    DOMAIN_DEVELOPMENT    = 0,
    DOMAIN_CYBERSECURITY  = 1,
    DOMAIN_DATA_SCIENCE   = 2,
    DOMAIN_NETWORKS       = 3,
    DOMAIN_DEVOPS         = 4,
    DOMAIN_EMBEDDED       = 5,
    DOMAIN_COUNT
} InterviewDomain;

static const char *DOMAIN_NAMES[DOMAIN_COUNT] = {
    "Développement",
    "Cybersécurité",
    "Data Science",
    "Réseaux",
    "DevOps",
    "Systèmes Embarqués"
};

/* ─── Structures principales ─────────────────────────────── */

typedef struct {
    int             id;
    char            username[MAX_NAME_LEN];
    char            email[MAX_EMAIL_LEN];
    char            password_hash[MAX_PASSWORD_LEN];
    char            token[MAX_TOKEN_LEN];
    time_t          created_at;
    int             total_sessions;
    float           avg_score;
} User;

typedef struct {
    char    text[MAX_QUESTION_LEN];
    int     order_index;
} Question;

typedef struct {
    int             id;
    int             user_id;
    InterviewDomain domain;
    char            position[MAX_NAME_LEN];
    time_t          started_at;
    time_t          ended_at;
    int             is_completed;
    int             question_count;
    Question        questions[MAX_QUESTIONS];
} Session;

typedef struct {
    int     session_id;
    int     question_index;
    char    question[MAX_QUESTION_LEN];
    char    answer[MAX_ANSWER_LEN];
    float   question_score;     /* 0.0 – 10.0 */
    char    feedback[MAX_FEEDBACK_LEN];
} Interview;

typedef struct {
    int     session_id;
    float   total_score;        /* 0.0 – 100.0 */
    float   avg_question_score;
    int     questions_answered;
    char    strengths[MAX_FEEDBACK_LEN];
    char    weaknesses[MAX_FEEDBACK_LEN];
    char    advice[MAX_FEEDBACK_LEN];
    time_t  evaluated_at;
} Score;

typedef struct {
    int     user_id;
    int     total_sessions;
    float   avg_score;
    float   best_score;
    float   worst_score;
    int     sessions_per_domain[DOMAIN_COUNT];
    float   avg_score_per_domain[DOMAIN_COUNT];
    char    best_domain[MAX_DOMAIN_LEN];
    char    worst_domain[MAX_DOMAIN_LEN];
} UserStats;

#endif /* MOCKAI_TYPES_H */
