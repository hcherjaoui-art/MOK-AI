#include "api.h"
#include "json_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

/* ─── Configuration globale ──────────────────────────────── */

#define GEMINI_BASE_URL \
    "https://generativelanguage.googleapis.com/v1beta/models/"
#define OPENAI_BASE_URL \
    "https://api.openai.com/v1/chat/completions"
#define DEFAULT_MODEL   "gemini-1.5-flash"

static char g_api_key[512]   = {0};
static char g_model[128]     = DEFAULT_MODEL;

void api_set_key(const char *key)
{
    if (key) strncpy(g_api_key, key, sizeof(g_api_key) - 1);
}

void api_set_model(const char *model)
{
    if (model) strncpy(g_model, model, sizeof(g_model) - 1);
}

void api_init(void)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void api_cleanup(void)
{
    curl_global_cleanup();
}

/* ─── Callback libcurl : accumulation de la réponse ─────── */

static size_t write_callback(void *contents,
                              size_t size,
                              size_t nmemb,
                              void *userp)
{
    size_t real_size = size * nmemb;
    ApiResponse *resp = (ApiResponse *)userp;

    char *ptr = realloc(resp->data, resp->size + real_size + 1);
    if (!ptr) return 0;   /* OOM → libcurl retournera une erreur */

    resp->data = ptr;
    memcpy(resp->data + resp->size, contents, real_size);
    resp->size               += real_size;
    resp->data[resp->size]    = '\0';
    return real_size;
}

void api_response_free(ApiResponse *r)
{
    if (r && r->data) {
        free(r->data);
        r->data = NULL;
        r->size = 0;
    }
}

/* ─── POST JSON générique ────────────────────────────────── */

MockAIStatus api_post_json(const char   *url,
                            const char   *payload,
                            const char   *auth_header,
                            ApiResponse  *out)
{
    if (!url || !payload || !out) return MOCKAI_ERR_INPUT;

    CURL *curl = curl_easy_init();
    if (!curl) return MOCKAI_ERR_API;

    memset(out, 0, sizeof(ApiResponse));

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (auth_header)
        headers = curl_slist_append(headers, auth_header);

    curl_easy_setopt(curl, CURLOPT_URL,            url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     payload);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      out);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code < 200 || http_code >= 300) {
        api_response_free(out);
        fprintf(stderr, "[API] Erreur HTTP %ld : %s\n",
                http_code, curl_easy_strerror(res));
        return MOCKAI_ERR_API;
    }
    return MOCKAI_OK;
}

/* ─── Génération de questions via Gemini ─────────────────── */

MockAIStatus api_generate_questions(InterviewDomain domain,
                                     const char     *position,
                                     int             count,
                                     Question       *out_questions,
                                     int            *out_count)
{
    if (!position || !out_questions || !out_count) return MOCKAI_ERR_INPUT;
    if (count < 1 || count > MAX_QUESTIONS)        return MOCKAI_ERR_INPUT;

    /* Construire le prompt */
    char payload[4096];
    MockAIStatus st = json_build_question_prompt(domain, position,
                                                  count, payload,
                                                  sizeof(payload));
    if (st != MOCKAI_OK) return st;

    /* Construire l'URL Gemini */
    char url[512];
    snprintf(url, sizeof(url),
             "%s%s:generateContent?key=%s",
             GEMINI_BASE_URL, g_model, g_api_key);

    ApiResponse resp;
    st = api_post_json(url, payload, NULL, &resp);
    if (st != MOCKAI_OK) return st;

    /* Parser la réponse */
    st = json_parse_questions(resp.data, out_questions,
                               out_count, MAX_QUESTIONS);
    api_response_free(&resp);
    return st;
}

/* ─── Évaluation d'une réponse ───────────────────────────── */

MockAIStatus api_evaluate_answer(const char     *question,
                                  const char     *answer,
                                  InterviewDomain domain,
                                  float          *out_score,
                                  char           *out_feedback,
                                  size_t          feedback_len)
{
    if (!question || !answer || !out_score || !out_feedback)
        return MOCKAI_ERR_INPUT;

    char payload[8192];
    MockAIStatus st = json_build_eval_prompt(question, answer,
                                              payload, sizeof(payload));
    if (st != MOCKAI_OK) return st;

    char url[512];
    snprintf(url, sizeof(url),
             "%s%s:generateContent?key=%s",
             GEMINI_BASE_URL, g_model, g_api_key);

    ApiResponse resp;
    st = api_post_json(url, payload, NULL, &resp);
    if (st != MOCKAI_OK) return st;

    st = json_parse_evaluation(resp.data, out_score,
                                out_feedback, feedback_len);
    api_response_free(&resp);
    return st;
}

/* ─── Évaluation globale d'une session ───────────────────── */

MockAIStatus api_evaluate_session(int             session_id,
                                   const Interview *answers,
                                   int             count,
                                   Score          *out_score)
{
    if (!answers || !out_score || count <= 0)
        return MOCKAI_ERR_INPUT;

    float  total    = 0.0f;
    int    evaluated = 0;
    char   strengths[MAX_FEEDBACK_LEN]  = {0};
    char   weaknesses[MAX_FEEDBACK_LEN] = {0};

    for (int i = 0; i < count; i++) {
        float  q_score = 0.0f;
        char   feedback[MAX_FEEDBACK_LEN];

        MockAIStatus st = api_evaluate_answer(
            answers[i].question,
            answers[i].answer,
            DOMAIN_DEVELOPMENT,   /* domaine passé lors de l'appel session */
            &q_score,
            feedback,
            sizeof(feedback));

        if (st == MOCKAI_OK) {
            total += q_score;
            evaluated++;
        }
    }

    out_score->session_id         = session_id;
    out_score->questions_answered = count;
    out_score->avg_question_score = evaluated > 0
                                    ? total / evaluated : 0.0f;
    out_score->total_score        = out_score->avg_question_score * 10.0f;

    return MOCKAI_OK;
}
