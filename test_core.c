#include "test.h"
#include "../include/types.h"
#include "../include/session.h"
#include "../include/interview.h"
#include "../include/score.h"
#include "../include/stats.h"
#include "../include/json_parser.h"
#include <string.h>
#include <math.h>

/* ══════════════════════════════════════════
   SESSION
   ══════════════════════════════════════════ */

static void test_session_domain_name(void)
{
    TEST_SUITE("session_domain_name");

    ASSERT(strcmp(session_domain_name(DOMAIN_DEVELOPMENT),   "Développement") == 0);
    ASSERT(strcmp(session_domain_name(DOMAIN_CYBERSECURITY), "Cybersécurité") == 0);
    ASSERT(strcmp(session_domain_name(DOMAIN_DATA_SCIENCE),  "Data Science")  == 0);
    ASSERT(strcmp(session_domain_name(DOMAIN_NETWORKS),      "Réseaux")       == 0);
    ASSERT(strcmp(session_domain_name((InterviewDomain)99),  "Inconnu")       == 0);
}

static void test_session_is_active(void)
{
    TEST_SUITE("session_is_active");

    Session s;
    memset(&s, 0, sizeof(s));

    /* Pas encore démarrée */
    ASSERT(session_is_active(&s) == 0);

    /* Active */
    s.started_at   = 1000;
    s.is_completed = 0;
    ASSERT(session_is_active(&s) == 1);

    /* Terminée */
    s.is_completed = 1;
    ASSERT(session_is_active(&s) == 0);

    /* NULL guard */
    ASSERT(session_is_active(NULL) == 0);
}

static void test_session_questions(void)
{
    TEST_SUITE("session_set_questions / session_get_question");

    Session s;
    memset(&s, 0, sizeof(s));

    Question qs[3];
    memset(qs, 0, sizeof(qs));
    strncpy(qs[0].text, "Qu'est-ce qu'un pointeur?", MAX_QUESTION_LEN - 1);
    strncpy(qs[1].text, "Expliquez malloc/free.",    MAX_QUESTION_LEN - 1);
    strncpy(qs[2].text, "Qu'est-ce qu'une struct?",  MAX_QUESTION_LEN - 1);

    MockAIStatus st = session_set_questions(&s, qs, 3);
    ASSERT_EQ(st, MOCKAI_OK);
    ASSERT_EQ(s.question_count, 3);

    Question *q0 = session_get_question(&s, 0);
    ASSERT_NOTNULL(q0);
    ASSERT(strcmp(q0->text, "Qu'est-ce qu'un pointeur?") == 0);

    /* Hors bornes */
    ASSERT_NULL(session_get_question(&s, 10));
    ASSERT_NULL(session_get_question(NULL, 0));

    /* Count invalide */
    st = session_set_questions(&s, qs, MAX_QUESTIONS + 1);
    ASSERT_EQ(st, MOCKAI_ERR_INPUT);
}

/* ══════════════════════════════════════════
   INTERVIEW
   ══════════════════════════════════════════ */

static void test_interview_validation(void)
{
    TEST_SUITE("interview_validate_answer");

    ASSERT(interview_validate_answer("Réponse suffisamment longue.") == 1);
    ASSERT(interview_validate_answer(NULL)   == 0);
    ASSERT(interview_validate_answer("trop") == 0);   /* < 10 chars */

    /* Réponse trop longue */
    char big[MAX_ANSWER_LEN + 10];
    memset(big, 'a', sizeof(big) - 1);
    big[sizeof(big) - 1] = '\0';
    ASSERT(interview_validate_answer(big) == 0);
}

static void test_interview_sanitize(void)
{
    TEST_SUITE("interview_sanitize_answer");

    char out[256];

    /* Texte normal */
    MockAIStatus st = interview_sanitize_answer("Réponse normale.", out, sizeof(out));
    ASSERT_EQ(st, MOCKAI_OK);
    ASSERT(strlen(out) > 0);

    /* Caractères de contrôle retirés */
    interview_sanitize_answer("Test\x01\x02normal", out, sizeof(out));
    ASSERT(strstr(out, "\x01") == NULL);
    ASSERT(strstr(out, "\x02") == NULL);

    /* NULL guard */
    st = interview_sanitize_answer(NULL, out, sizeof(out));
    ASSERT_EQ(st, MOCKAI_ERR_INPUT);
}

/* ══════════════════════════════════════════
   SCORE
   ══════════════════════════════════════════ */

static void test_score_compute_average(void)
{
    TEST_SUITE("score_compute_average");

    Interview answers[3];
    memset(answers, 0, sizeof(answers));
    answers[0].question_score = 8.0f;
    answers[1].question_score = 6.0f;
    answers[2].question_score = 7.0f;

    float avg = score_compute_average(answers, 3);
    ASSERT(fabsf(avg - 7.0f) < 0.01f);

    /* Aucun réponse évaluée */
    answers[0].question_score = -1.0f;
    answers[1].question_score = -1.0f;
    answers[2].question_score = -1.0f;
    avg = score_compute_average(answers, 3);
    ASSERT(fabsf(avg) < 0.01f);

    /* NULL / count = 0 */
    ASSERT(fabsf(score_compute_average(NULL, 3))     < 0.01f);
    ASSERT(fabsf(score_compute_average(answers, 0))  < 0.01f);
}

static void test_score_grade_label(void)
{
    TEST_SUITE("score_grade_label");

    ASSERT(strcmp(score_grade_label(90.0f), "Excellent")      == 0);
    ASSERT(strcmp(score_grade_label(75.0f), "Bien")           == 0);
    ASSERT(strcmp(score_grade_label(60.0f), "Satisfaisant")   == 0);
    ASSERT(strcmp(score_grade_label(45.0f), "À améliorer")    == 0);
    ASSERT(strcmp(score_grade_label(20.0f), "Insuffisant")    == 0);
}

/* ══════════════════════════════════════════
   JSON PARSER
   ══════════════════════════════════════════ */

static void test_json_get_string(void)
{
    TEST_SUITE("json_get_string");

    const char *json = "{\"name\":\"MockAI\",\"version\":\"1.0\"}";
    char out[64];

    MockAIStatus st = json_get_string(json, "name", out, sizeof(out));
    ASSERT_EQ(st, MOCKAI_OK);
    ASSERT(strcmp(out, "MockAI") == 0);

    st = json_get_string(json, "version", out, sizeof(out));
    ASSERT_EQ(st, MOCKAI_OK);
    ASSERT(strcmp(out, "1.0") == 0);

    st = json_get_string(json, "missing", out, sizeof(out));
    ASSERT_EQ(st, MOCKAI_ERR_JSON);
}

static void test_json_get_float(void)
{
    TEST_SUITE("json_get_float");

    const char *json = "{\"score\":7.5,\"total\":100.0}";
    float val;

    MockAIStatus st = json_get_float(json, "score", &val);
    ASSERT_EQ(st, MOCKAI_OK);
    ASSERT(fabsf(val - 7.5f) < 0.001f);

    st = json_get_float(json, "missing", &val);
    ASSERT_EQ(st, MOCKAI_ERR_JSON);
}

static void test_json_escape(void)
{
    TEST_SUITE("json_escape_string");

    char out[256];
    MockAIStatus st = json_escape_string("He said \"hello\"", out, sizeof(out));
    ASSERT_EQ(st, MOCKAI_OK);
    ASSERT(strstr(out, "\\\"") != NULL);

    /* Newlines */
    json_escape_string("line1\nline2", out, sizeof(out));
    ASSERT(strstr(out, "\\n") != NULL);
}

static void test_json_build_prompt(void)
{
    TEST_SUITE("json_build_question_prompt");

    char buf[4096];
    MockAIStatus st = json_build_question_prompt(DOMAIN_DEVELOPMENT,
                                                  "Dev C Backend",
                                                  5, buf, sizeof(buf));
    ASSERT_EQ(st, MOCKAI_OK);
    ASSERT(strlen(buf) > 0);
    ASSERT(strstr(buf, "contents") != NULL);
    ASSERT(strstr(buf, "Dev C Backend") != NULL);
}

/* ══════════════════════════════════════════
   MAIN
   ══════════════════════════════════════════ */

int main(void)
{
    printf("╔══════════════════════════════════════════╗\n");
    printf("║   Tests unitaires : session/score/json   ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    test_session_domain_name();
    test_session_is_active();
    test_session_questions();

    test_interview_validation();
    test_interview_sanitize();

    test_score_compute_average();
    test_score_grade_label();

    test_json_get_string();
    test_json_get_float();
    test_json_escape();
    test_json_build_prompt();

    TEST_SUMMARY();
    TEST_EXIT();
}
