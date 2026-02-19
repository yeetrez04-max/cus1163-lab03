#include "process_manager.h"

/*
 * run_basic_demo()
 * one producer (1-5) + one consumer (sum)
 */
int run_basic_demo(void) {
    int pipe_fd[2];
    pid_t producer_pid, consumer_pid;
    int status;

    printf("\nParent process (PID: %d) creating children...\n", getpid());

    // TODO 1: pipe
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return -1;
    }

    // TODO 2: fork producer
    producer_pid = fork();
    if (producer_pid == 0) {
        // child: producer writes
        close(pipe_fd[0]);                 // don't need read side
        producer_process(pipe_fd[1], 1);   // starts at 1
        exit(0); // producer_process already exits, but whatever
    } else if (producer_pid < 0) {
        perror("fork producer");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
    } else {
        printf("Created producer child (PID: %d)\n", producer_pid);
    }

    // TODO 3: fork consumer
    consumer_pid = fork();
    if (consumer_pid == 0) {
        // child: consumer reads
        close(pipe_fd[1]);                 // don't need write side
        consumer_process(pipe_fd[0], 0);
        exit(0);
    } else if (consumer_pid < 0) {
        perror("fork consumer");
        // try not to leave zombie producer
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        waitpid(producer_pid, &status, 0);
        return -1;
    } else {
        printf("Created consumer child (PID: %d)\n", consumer_pid);
    }

    // TODO 4: parent cleanup + wait
    // parent doesn't use pipe
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    // wait producer first (order doesn't matter too much)
    if (waitpid(producer_pid, &status, 0) == -1) {
        perror("waitpid producer");
    } else {
        if (WIFEXITED(status)) {
            printf("\nProducer child (PID: %d) exited with status %d\n",
                   producer_pid, WEXITSTATUS(status));
        } else {
            printf("\nProducer child (PID: %d) exited weirdly\n", producer_pid);
        }
    }

    if (waitpid(consumer_pid, &status, 0) == -1) {
        perror("waitpid consumer");
    } else {
        if (WIFEXITED(status)) {
            printf("Consumer child (PID: %d) exited with status %d\n",
                   consumer_pid, WEXITSTATUS(status));
        } else {
            printf("Consumer child (PID: %d) exited weirdly\n", consumer_pid);
        }
    }

    return 0;
}

/*
 * run_multiple_pairs(num_pairs)
 * each pair gets a pipe and a range
 * pair 1: 1-5, pair 2: 6-10, etc
 */
int run_multiple_pairs(int num_pairs) {
    pid_t pids[10];
    int pid_count = 0;

    printf("\nParent creating %d producer-consumer pairs...\n", num_pairs);

    // just in case they pass something too big
    if (num_pairs > 5) num_pairs = 5;
    if (num_pairs < 0) num_pairs = 0;

    // TODO 5: loop create pairs
    for (int i = 0; i < num_pairs; i++) {
        int pipe_fd[2];
        pid_t prod, cons;

        printf("\n=== Pair %d ===\n", i + 1);

        if (pipe(pipe_fd) == -1) {
            perror("pipe");
            return -1;
        }

        prod = fork();
        if (prod == 0) {
            close(pipe_fd[0]);
            producer_process(pipe_fd[1], i * 5 + 1);
            exit(0);
        } else if (prod < 0) {
            perror("fork producer");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            return -1;
        } else {
            pids[pid_count++] = prod;
        }

        cons = fork();
        if (cons == 0) {
            close(pipe_fd[1]);
            consumer_process(pipe_fd[0], i + 1);
            exit(0);
        } else if (cons < 0) {
            perror("fork consumer");
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            return -1;
        } else {
            pids[pid_count++] = cons;
        }

        // parent closes both ends (important!)
        close(pipe_fd[0]);
        close(pipe_fd[1]);
    }

    // TODO 6: wait all kids
    for (int i = 0; i < pid_count; i++) {
        int status;
        pid_t done = waitpid(pids[i], &status, 0);
        if (done == -1) {
            perror("waitpid");
        } else {
            if (WIFEXITED(status)) {
                printf("Child (PID: %d) exited with status %d\n",
                       done, WEXITSTATUS(status));
            } else {
                printf("Child (PID: %d) exited weirdly\n", done);
            }
        }
    }

    printf("\nAll pairs completed successfully!\n");
    return 0;
}

/*
 * Producer Process - Sends 5 sequential numbers starting from start_num
 */
void producer_process(int write_fd, int start_num) {
    printf("Producer (PID: %d) starting...\n", getpid());

    for (int i = 0; i < NUM_VALUES; i++) {
        int number = start_num + i;

        if (write(write_fd, &number, sizeof(number)) != sizeof(number)) {
            perror("write");
            exit(1);
        }

        printf("Producer: Sent number %d\n", number);
        usleep(100000);
    }

    printf("Producer: Finished sending %d numbers\n", NUM_VALUES);
    close(write_fd);
    exit(0);
}

/*
 * Consumer Process - Receives numbers and calculates sum
 */
void consumer_process(int read_fd, int pair_id) {
    int number;
    int count = 0;
    int sum = 0;

    printf("Consumer (PID: %d) starting...\n", getpid());

    while (read(read_fd, &number, sizeof(number)) > 0) {
        count++;
        sum += number;
        printf("Consumer: Received %d, running sum: %d\n", number, sum);
    }

    printf("Consumer: Final sum: %d\n", sum);
    close(read_fd);
    exit(0);
}
