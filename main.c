#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "user.h"
#include "session.h"
#include "interview.h"
#include "score.h"
#include "stats.h"
#include "api.h"

/* ─── Prototypes DB (Ahmed) ──────────────────────────────── */
extern MockAIStatus db_init(const char *db_path);
extern void         db_close(void);

/* ─── Flux principal d'une session MockAI ────────────────── */

static void print_separator(void)
{
    puts("══════════════════════════════════════════");
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    print_separator();
    puts("         MockAI – Backend C v1.0");
    print_separator();

    /* 1. Initialisation */
    if (db_init("mockai.db") != MOCKAI_OK) {
        fprintf(stderr, "[ERREUR] Impossible d'ouvrir la base de données.\n");
        return EXIT_FAILURE;
    }

    const char *api_key = getenv("MOCKAI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "[ERREUR] Variable MOCKAI_API_KEY non définie.\n");
        db_close();
        return EXIT_FAILURE;
    }
    api_init();
    api_set_key(api_key);
    api_set_model("gemini-1.5-flash");

    /* 2. Inscription / Connexion */
    User user;
    MockAIStatus st = user_register("ghali", "ghali@mockai.io",
                                     "SecurePass1", &user);
    if (st != MOCKAI_OK && st != MOCKAI_ERR_DB) {
        /* L'utilisateur existe peut-être déjà : tenter un login */
        st = user_login("ghali@mockai.io", "SecurePass1", &user);
    }
    if (st != MOCKAI_OK) {
        fprintf(stderr, "[ERREUR] Authentification échouée (%d).\n", st);
        goto cleanup;
    }
    printf("[OK] Connecté en tant que : %s\n", user.username);

    /* 3. Démarrer une session */
    Session session;
    st = session_start(user.id, DOMAIN_DEVELOPMENT,
                       "Développeur C Backend", &session);
    if (st != MOCKAI_OK) {
        fprintf(stderr, "[ERREUR] Impossible de démarrer la session.\n");
        goto cleanup;
    }
    printf("[OK] Session #%d démarrée – domaine : %s\n",
           session.id, session_domain_name(session.domain));

    /* 4. Générer les questions via l'IA */
    int q_count = 0;
    st = api_generate_questions(session.domain, session.position,
                                 5, session.questions, &q_count);
    if (st != MOCKAI_OK) {
        fprintf(stderr, "[ERREUR] Génération des questions échouée.\n");
        goto cleanup;
    }
    session_set_questions(&session, session.questions, q_count);
    printf("[OK] %d questions générées.\n\n", q_count);

    /* 5. Simuler des réponses (en production : saisie utilisateur) */
    const char *demo_answer =
        "J'utilise des structures et des pointeurs pour gérer "
        "la mémoire dynamiquement avec malloc et free. "
        "Je fais attention aux fuites mémoire en utilisant "
        "valgrind pendant le développement.";

    for (int i = 0; i < q_count; i++) {
        Question *q = session_get_question(&session, i);
        if (!q) continue;

        printf("Q%d : %s\n", i + 1, q->text);

        st = interview_save_answer(session.id, i,
                                    q->text, demo_answer);
        if (st != MOCKAI_OK)
            fprintf(stderr, "  [AVERTISSEMENT] Sauvegarde réponse %d.\n", i);
    }

    /* 6. Évaluation par l'IA */
    print_separator();
    puts("Évaluation en cours...");

    Interview *answers = NULL;
    int answer_count   = 0;
    st = interview_get_answers(session.id, &answers, &answer_count);
    if (st != MOCKAI_OK) goto cleanup;

    Score final_score;
    st = api_evaluate_session(session.id, answers,
                               answer_count, &final_score);
    if (st == MOCKAI_OK) {
        score_save(&final_score);
        user_update_stats(user.id, final_score.total_score);
    }
    free(answers);

    /* 7. Terminer la session */
    session_complete(session.id);

    /* 8. Afficher le score */
    Score stored;
    if (score_get_by_session(session.id, &stored) == MOCKAI_OK) {
        print_separator();
        printf("Score final   : %.1f / 100 (%s)\n",
               stored.total_score,
               score_grade_label(stored.total_score));
        if (strlen(stored.strengths) > 0)
            printf("Points forts  : %s\n", stored.strengths);
        if (strlen(stored.weaknesses) > 0)
            printf("Points faibles: %s\n", stored.weaknesses);
        if (strlen(stored.advice) > 0)
            printf("Conseils      : %s\n", stored.advice);
    }

    /* 9. Statistiques globales */
    UserStats stats;
    if (stats_compute(user.id, &stats) == MOCKAI_OK) {
        print_separator();
        printf("Statistiques  : %d session(s), moy. %.1f%%\n",
               stats.total_sessions, stats.avg_score);
        printf("Meilleur domaine : %s\n", stats.best_domain);
    }

cleanup:
    api_cleanup();
    db_close();
    print_separator();
    return (st == MOCKAI_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
