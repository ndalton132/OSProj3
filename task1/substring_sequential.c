#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define MAX 1024
#define NUM_THREADS 4 // Example number of threads, adjust as needed

int total = 0;
int n1, n2;
char* s1, * s2;
pthread_mutex_t mutex;

/* Function to read strings s1 and s2 from a file */
int readf(FILE* fp) {
    if ((fp = fopen("strings.txt", "r")) == NULL) {
        printf("ERROR: can't open string.txt!\n");
        return 0;
    }
    s1 = (char*)malloc(sizeof(char) * MAX);
    if (s1 == NULL) {
        printf("ERROR: Out of memory!\n");
        return -1;
    }
    s2 = (char*)malloc(sizeof(char) * MAX);
    if (s2 == NULL) {
        printf("ERROR: Out of memory\n");
        return -1;
    }
    /* Read s1 and s2 from the file */
    s1 = fgets(s1, MAX, fp);
    s2 = fgets(s2, MAX, fp);
    n1 = strlen(s1) - 1; /* Length of s1 */
    n2 = strlen(s2) - 1; /* Length of s2 */

    if (s1 == NULL || s2 == NULL || n1 < n2) /* When error exit */
        return -1;

    fclose(fp);
    return 1;
}

/* Function executed by each thread to search for substrings */
void* substring_search(void* arg) {
    int thread_id = *(int*)arg;
    int chunk_size = n1 / NUM_THREADS;
    int start_index = thread_id * chunk_size;
    int end_index = (thread_id == NUM_THREADS - 1) ? n1 : start_index + chunk_size + n2;

    int local_count = 0;

    /* Search for substrings in the assigned chunk */
    for (int i = start_index; i <= (end_index - n2); i++) {
        int count = 0;
        for (int j = i, k = 0; k < n2; j++, k++) {
            if (s1[j] != s2[k]) {
                break;
            }
            else
                count++;
            if (count == n2)
                local_count++; /* Find a substring in this step */
        }
    }

    /* Update total count by acquiring and releasing the mutex lock */
    pthread_mutex_lock(&mutex);
    total += local_count;
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main() {
    FILE* fp;
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    pthread_mutex_init(&mutex, NULL);

    /* Read input strings and initialize global variables */
    if (readf(fp) != 1) {
        printf("Error reading input file.\n");
        return -1;
    }

    /* Create threads to search for substrings */
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, substring_search, (void*)&thread_ids[i]);
    }

    /* Wait for all threads to finish */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Print the total number of substrings found */
    printf("The number of substrings is: %d\n", total);

    /* Clean up resources */
    pthread_mutex_destroy(&mutex);
    free(s1);
    free(s2);

    return 0;
}
