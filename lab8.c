#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "uthash.h"

typedef struct word_count {
    char *word;              // key
    int count;
    UT_hash_handle hh;
} word_count_t;

typedef struct thread_args {
    word_count_t **counts;
    pthread_mutex_t *count_mutex;
    const char **words;
    int start_idx;
    int end_idx;
} thread_args_t;

/* ---------- Helper functions ---------- */

static void add_one_word(word_count_t **counts, const char *word) {
    word_count_t *entry = NULL;

    HASH_FIND_STR(*counts, word, entry);
    if (entry == NULL) {
        entry = malloc(sizeof(word_count_t));
        if (entry == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        entry->word = strdup(word);
        if (entry->word == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }

        entry->count = 1;
        HASH_ADD_KEYPTR(hh, *counts, entry->word, strlen(entry->word), entry);
    } else {
        entry->count++;
    }
}

static int sort_words(word_count_t *a, word_count_t *b) {
    return strcmp(a->word, b->word);
}

static void print_counts(word_count_t *counts) {
    printf("%-32s%s\n", "Word", "Count");

    word_count_t *curr, *tmp;
    HASH_ITER(hh, counts, curr, tmp) {
        printf("%-32s%d\n", curr->word, curr->count);
    }
}

static void free_counts(word_count_t **counts) {
    word_count_t *curr, *tmp;

    HASH_ITER(hh, *counts, curr, tmp) {
        HASH_DEL(*counts, curr);
        free(curr->word);
        free(curr);
    }
}

static thread_args_t *pack_args(
    word_count_t **counts,
    pthread_mutex_t *count_mutex,
    const char **words,
    int start_idx,
    int end_idx
) {
    thread_args_t *args = malloc(sizeof(thread_args_t));
    if (args == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    args->counts = counts;
    args->count_mutex = count_mutex;
    args->words = words;
    args->start_idx = start_idx;
    args->end_idx = end_idx;

    return args;
}

/* ---------- Task 4: thread-safe chunk counting ---------- */

static void add_word_counts_in_chunk(
    word_count_t **counts,
    pthread_mutex_t *count_mutex,
    const char **words,
    int start_idx,
    int end_idx
) {
    for (int i = start_idx; i < end_idx; i++) {
        pthread_mutex_lock(count_mutex);
        add_one_word(counts, words[i]);
        pthread_mutex_unlock(count_mutex);
    }
}

/* ---------- Thread function ---------- */

static void *counter_thread_func(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;

    add_word_counts_in_chunk(
        args->counts,
        args->count_mutex,
        args->words,
        args->start_idx,
        args->end_idx
    );

    return NULL;
}

/* ---------- Sequential version ---------- */

static void count_words_seq(word_count_t **counts, const char **words, int num_words) {
    for (int i = 0; i < num_words; i++) {
        add_one_word(counts, words[i]);
    }
}

/* ---------- Task 2: parallel version ---------- */

static void count_words_parallel(
    word_count_t **counts,
    const char **words,
    int num_words,
    int num_threads
) {
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    thread_args_t **threads_args = malloc(sizeof(thread_args_t *) * num_threads);
    pthread_mutex_t count_mutex;

    if (threads == NULL || threads_args == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&count_mutex, NULL);

    int base_chunk = num_words / num_threads;
    int remainder = num_words % num_threads;
    int start = 0;

    for (int i = 0; i < num_threads; i++) {
        int chunk_size = base_chunk + (i < remainder ? 1 : 0);
        int end = start + chunk_size;

        threads_args[i] = pack_args(counts, &count_mutex, words, start, end);

        if (pthread_create(&threads[i], NULL, counter_thread_func, threads_args[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        start = end;
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        free(threads_args[i]);
    }

    pthread_mutex_destroy(&count_mutex);
    free(threads_args);
    free(threads);
}

/* ---------- Main ---------- */

int main(void) {
    const char *words[] = {
        "the", "quick", "brown", "fox",
        "jumps", "over", "the", "lazy",
        "dog", "the", "fox", "brown",
        "the"
    };

    int num_words = (int)(sizeof(words) / sizeof(words[0]));
    int num_threads = 3;
    word_count_t *counts = NULL;

    /* Task 2:
       comment out seq and use parallel */
    // count_words_seq(&counts, words, num_words);
    count_words_parallel(&counts, words, num_words, num_threads);

    /* Task 1: sort before printing */
    HASH_SORT(counts, sort_words);
    print_counts(counts);

    free_counts(&counts);
    return 0;
}
