/*
  list-forming.c:
  Each thread generates a data node, attaches it to a global list. This is repeated for K times.
  There are num_threads threads. The value of "num_threads" is input by the student.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sched.h>

#define K 200 /* Generate a data node for K times in each thread */
#define BATCH_SIZE 50 /* Number of nodes to accumulate before appending to the global list */

struct Node
{
    int data;
    struct Node* next;
};

struct list
{
    struct Node* header;
    struct Node* tail;
};

pthread_mutex_t mutex_lock; /* Mutex lock for thread synchronization */
struct list* List; /* Global list to which nodes are appended */
int* data_buffer; /* Buffer to accumulate data nodes before appending to the global list */
int data_buffer_index = 0; /* Index to keep track of the current position in the data buffer */

/* Function to bind thread to a specific CPU */
void bind_thread_to_cpu(int cpuid)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);

    CPU_SET(cpuid, &mask);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask))
    {
        fprintf(stderr, "sched_setaffinity");
        exit(EXIT_FAILURE);
    }
}

/* Function to append accumulated nodes to the global list */
void append_to_global_list()
{
    pthread_mutex_lock(&mutex_lock);

    for (int i = 0; i < data_buffer_index; i++)
    {
        struct Node* ptr = (struct Node*)malloc(sizeof(struct Node));
        if (ptr == NULL)
        {
            printf("Node allocation failed!\n");
            exit(EXIT_FAILURE);
        }
        ptr->data = data_buffer[i];
        ptr->next = NULL;

        if (List->header == NULL)
        {
            List->header = List->tail = ptr;
        }
        else
        {
            List->tail->next = ptr;
            List->tail = ptr;
        }
    }

    pthread_mutex_unlock(&mutex_lock);

    /* Reset buffer index */
    data_buffer_index = 0;
}

/* Function executed by each producer thread */
void* producer_thread(void* arg)
{
    bind_thread_to_cpu(*((int*)arg)); /* Bind this thread to a CPU */

    int counter = 0;

    /* Generate and accumulate K nodes to the thread-local buffer */
    while (counter < K)
    {
        data_buffer[data_buffer_index++] = 1; /* Generate data */
        counter++;

        if (data_buffer_index == BATCH_SIZE)
        {
            /* Append accumulated nodes to the global list */
            append_to_global_list();
        }
    }

    /* Append remaining nodes to the global list */
    append_to_global_list();

    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    int i, num_threads;

    int NUM_PROCS; /* Number of CPU */
    int* cpu_array = NULL;

    struct Node* tmp, * next;
    struct timeval starttime, endtime;

    if (argc == 1)
    {
        printf("ERROR: please provide an input arg (the number of threads)\n");
        exit(1);
    }

    num_threads = atoi(argv[1]); /* Read num_threads from user */
    pthread_t producer[num_threads];
    NUM_PROCS = sysconf(_SC_NPROCESSORS_CONF); /* Get number of CPU */
    if (NUM_PROCS > 0)
    {
        cpu_array = (int*)malloc(NUM_PROCS * sizeof(int));
        if (cpu_array == NULL)
        {
            printf("Allocation failed!\n");
            exit(0);
        }
        else
        {
            for (i = 0; i < NUM_PROCS; i++)
                cpu_array[i] = i;
        }
    }

    pthread_mutex_init(&mutex_lock, NULL);

    List = (struct list*)malloc(sizeof(struct list));
    if (List == NULL)
    {
        printf("End here\n");
        exit(0);
    }
    List->header = List->tail = NULL;

    /* Allocate memory for thread-local data buffer */
    data_buffer = (int*)malloc(BATCH_SIZE * sizeof(int));
    if (data_buffer == NULL)
    {
        printf("Allocation failed!\n");
        exit(0);
    }

    gettimeofday(&starttime, NULL); /* Get program start time */
    for (i = 0; i < num_threads; i++)
    {
        pthread_create(&(producer[i]), NULL, (void*)producer_thread, &cpu_array[i % NUM_PROCS]);
    }

    for (i = 0; i < num_threads; i++)
    {
        if (producer[i] != 0)
        {
            pthread_join(producer[i], NULL);
        }
    }

    gettimeofday(&endtime, NULL); /* Get the finish time */

    if (List->header != NULL)
    {
        next = tmp = List->header;
        while (tmp != NULL)
        {
            next = tmp->next;
            free(tmp);
            tmp = next;
        }
    }
    if (cpu_array != NULL)
        free(cpu_array);

    /* Free the data buffer */
    free(data_buffer);

    /* Calculate program runtime */
    printf("Total run time is %ld microseconds.\n", (endtime.tv_sec - starttime.tv_sec) * 1000000 + (endtime.tv_usec - starttime.tv_usec));
    return 0;
}
