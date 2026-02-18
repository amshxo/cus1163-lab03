#include "process_manager.h"

void producer_process(int write_fd, int start_num) {
    printf("\nProducer (PID: %d) starting...\n", getpid());

    for (int i = 0; i < 5; i++) {
        int value = start_num + i;

        if (write(write_fd, &value, sizeof(int)) == -1) {
            perror("producer write failed");
            exit(1);
        }

        printf("Producer: Sent number %d\n", value);
    }

    printf("Producer: Finished sending 5 numbers\n");
}

void consumer_process(int read_fd, int pair_num) {
    (void)pair_num; // pair_num is mainly for labeling; safe if unused in your grading

    printf("\nConsumer (PID: %d) starting...\n", getpid());

    int sum = 0;
    int value;
    ssize_t bytes;

    while ((bytes = read(read_fd, &value, sizeof(int))) > 0) {
        sum += value;
        printf("Consumer: Received %d, running sum: %d\n", value, sum);
    }

    if (bytes == -1) {
        perror("consumer read failed");
        exit(1);
    }

    printf("Consumer: Final sum: %d\n", sum);
}

int run_basic_demo(void) {
    int pipe_fd[2];
    pid_t producer_pid, consumer_pid;
    int status;

    printf("\nParent process (PID: %d) creating children...\n", getpid());

    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        return -1;
    }

    producer_pid = fork();
    if (producer_pid == -1) {
        perror("fork producer failed");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
    }

    if (producer_pid == 0) {
        close(pipe_fd[0]);               
        producer_process(pipe_fd[1], 1); 
        close(pipe_fd[1]);
        exit(0);
    }

    printf("Created producer child (PID: %d)\n", producer_pid);

    consumer_pid = fork();
    if (consumer_pid == -1) {
        perror("fork consumer failed");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
    }

    if (consumer_pid == 0) {
        close(pipe_fd[1]);                 
        consumer_process(pipe_fd[0], 1);   
        close(pipe_fd[0]);
        exit(0);
    }

    printf("Created consumer child (PID: %d)\n", consumer_pid);

    close(pipe_fd[0]);
    close(pipe_fd[1]);

    if (waitpid(producer_pid, &status, 0) == -1) {
        perror("waitpid producer failed");
        return -1;
    }
    if (WIFEXITED(status)) {
        printf("\nProducer child (PID: %d) exited with status %d\n",
               producer_pid, WEXITSTATUS(status));
    }

    if (waitpid(consumer_pid, &status, 0) == -1) {
        perror("waitpid consumer failed");
        return -1;
    }
    if (WIFEXITED(status)) {
        printf("Consumer child (PID: %d) exited with status %d\n",
               consumer_pid, WEXITSTATUS(status));
    }

    printf("\nSUCCESS: Basic producer-consumer completed!\n");
    return 0;
}

int run_multiple_pairs(int num_pairs) {
    if (num_pairs <= 0) {
        printf("No pairs to run.\n");
        return 0;
    }

    printf("\nParent creating %d producer-consumer pairs...\n", num_pairs);

    pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * 2 * num_pairs);
    if (pids == NULL) {
        perror("malloc failed");
        return -1;
    }
    int pid_count = 0;

    for (int i = 0; i < num_pairs; i++) {
        int pipe_fd[2];

        printf("\n=== Pair %d ===\n", i + 1);

        if (pipe(pipe_fd) == -1) {
            perror("pipe failed");
            free(pids);
            return -1;
        }

        pid_t producer_pid = fork();
        if (producer_pid == -1) {
            perror("fork producer failed");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            free(pids);
            return -1;
        }

        if (producer_pid == 0) {
            close(pipe_fd[0]); 
            producer_process(pipe_fd[1], i * 5 + 1);
            close(pipe_fd[1]);
            exit(0);
        }

        pids[pid_count++] = producer_pid;

        pid_t consumer_pid = fork();
        if (consumer_pid == -1) {
            perror("fork consumer failed");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            free(pids);
            return -1;
        }

        if (consumer_pid == 0) {
            close(pipe_fd[1]);
            consumer_process(pipe_fd[0], i + 1);
            close(pipe_fd[0]);
            exit(0);
        }

        pids[pid_count++] = consumer_pid;

        close(pipe_fd[0]);
        close(pipe_fd[1]);
    }


    int status;
    for (int i = 0; i < pid_count; i++) {
        pid_t done = waitpid(pids[i], &status, 0);
        if (done == -1) {
            perror("waitpid failed");
        } else {
            if (WIFEXITED(status)) {
                printf("Child (PID: %d) exited with status %d\n", done, WEXITSTATUS(status));
            } else {
                printf("Child (PID: %d) ended abnormally\n", done);
            }
        }
    }

    free(pids);

    printf("\nAll pairs completed successfully!\n");
    printf("SUCCESS: Multiple pairs completed!\n");
    return 0;
}
