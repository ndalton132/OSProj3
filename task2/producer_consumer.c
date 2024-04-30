#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE 12
#define MESSAGE_FILE "message.txt"

char circular_buffer[BUFFER_SIZE];
int buffer_index = 0;
int message_index = 0;
int message_length = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty_condition = PTHREAD_COND_INITIALIZER;

/* Producer thread function */
void* producer(void* arg) {
    FILE* file;
    char ch;

    /* Open the message file */
    if ((file = fopen(MESSAGE_FILE, "r")) == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    /* Read characters from the file and add them to the circular buffer */
    while ((ch = fgetc(file)) != EOF) {
        /* Acquire the mutex lock */
        pthread_mutex_lock(&mutex);

        /* Wait while the buffer is full */
        while (buffer_index == BUFFER_SIZE) {
            pthread_cond_wait(&full_condition, &mutex);
        }

        /* Add character to the circular buffer */
        circular_buffer[buffer_index] = ch;
        buffer_index = (buffer_index + 1) % BUFFER_SIZE;
        message_index++;
        message_length++;

        /* Signal that the buffer is not empty */
        pthread_cond_signal(&empty_condition);

        /* Release the mutex lock */
        pthread_mutex_unlock(&mutex);
    }

    /* Close the file */
    fclose(file);

    /* Exit the thread */
    pthread_exit(NULL);
}

/* Consumer thread function */
void* consumer(void* arg) {
    /* Continue consuming until all messages are processed */
    while (message_index < message_length) {
        /* Acquire the mutex lock */
        pthread_mutex_lock(&mutex);

        /* Wait while the buffer is empty */
        while (buffer_index == 0) {
            pthread_cond_wait(&empty_condition, &mutex);
        }

        /* Print the character from the circular buffer */
        printf("%c", circular_buffer[(buffer_index - 1 + BUFFER_SIZE) % BUFFER_SIZE]);
        buffer_index = (buffer_index - 1 + BUFFER_SIZE) % BUFFER_SIZE;

        /* Signal that the buffer is not full */
        pthread_cond_signal(&full_condition);

        /* Release the mutex lock */
        pthread_mutex_unlock(&mutex);
    }

    /* Print newline when all messages are consumed */
    printf("\n");

    /* Exit the thread */
    pthread_exit(NULL);
}

int main() {
    pthread_t producer_thread, consumer_thread;

    /* Create producer and consumer threads */
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    /* Wait for the threads to finish */
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    return 0;
}
