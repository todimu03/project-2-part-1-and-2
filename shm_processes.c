#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

void DadLoop(int *BankAccount, sem_t *mutex);
void MomLoop(int *BankAccount, sem_t *mutex);
void StudentLoop(int *BankAccount, sem_t *mutex);

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Use: %s <num_parents (1 or 2)> <num_children>\n", argv[0]);
        exit(1);
    }
    int num_parents = atoi(argv[1]);
    int num_children = atoi(argv[2]);
    if (num_parents < 1 || num_parents > 2 || num_children < 1) {
        printf("Invalid arguments\n");
        exit(1);
    }

    // Unbuffered output so messages appear immediately in order
    setbuf(stdout, NULL);

    // Create shared memory for the BankAccount (one integer)
    int ShmID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        perror("shmget");
        exit(1);
    }
    int *BankAccount = (int *) shmat(ShmID, NULL, 0);
    if (BankAccount == (int *) -1) {
        perror("shmat");
        exit(1);
    }
    shmctl(ShmID, IPC_RMID, NULL); // mark for deletion when all detach
    *BankAccount = 0;              // initial balance

    // Create a named semaphore for mutual exclusion
    char sem_name[256];
    sprintf(sem_name, "/psdd_mutex_%d", getpid());
    sem_unlink(sem_name);
    sem_t *mutex = sem_open(sem_name, O_CREAT, 0644, 1);
    if (mutex == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }
    sem_unlink(sem_name); // unlink immediately; cleaned up when processes exit

    // Fork Lovable Mom if requested
    if (num_parents == 2) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            // Child: Mom
            srand(getpid());
            MomLoop(BankAccount, mutex);
            exit(0);
        }
    }

    // Fork N Poor Students
    for (int i = 0; i < num_children; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            // Child: Student
            srand(getpid());
            StudentLoop(BankAccount, mutex);
            exit(0);
        }
    }

    // Parent: Dear Old Dad
    srand(getpid());
    DadLoop(BankAccount, mutex);

    // Should never reach here
    return 0;
}

void DadLoop(int *BankAccount, sem_t *mutex)
{
    int localBalance = 0;
    while (1) {
        sleep(rand() % 6); // 0–5 seconds
        printf("Dear Old Dad: Attempting to Check Balance\n");
        int r = rand();
        if (r % 2 == 0) { // even
            sem_wait(mutex);
            localBalance = *BankAccount;
            if (localBalance < 100) {
                // Try to deposit
                int amount = rand() % 101 + 100; // 100..200
                if (amount % 2 == 0) {
                    localBalance += amount;
                    *BankAccount = localBalance;
                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n", amount, localBalance);
                } else {
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }
            } else {
                printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
            }
            sem_post(mutex);
        } else { // odd
            sem_wait(mutex);
            localBalance = *BankAccount;
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
            sem_post(mutex);
        }
    }
}

void MomLoop(int *BankAccount, sem_t *mutex)
{
    int localBalance = 0;
    while (1) {
        sleep(rand() % 11); // 0–10 seconds
        printf("Loveable Mom: Attempting to Check Balance\n");
        sem_wait(mutex);
        localBalance = *BankAccount;
        if (localBalance <= 100) {
            int amount = rand() % 101 + 125; // 125..225
            localBalance += amount;
            *BankAccount = localBalance;
            printf("Lovable Mom: Deposits $%d / Balance = $%d\n", amount, localBalance);
        }
        sem_post(mutex);
    }
}

void StudentLoop(int *BankAccount, sem_t *mutex)
{
    int localBalance = 0;
    while (1) {
        sleep(rand() % 6); // 0–5 seconds
        printf("Poor Student: Attempting to Check Balance\n");
        int r = rand();
        if (r % 2 == 0) { // even
            sem_wait(mutex);
            localBalance = *BankAccount;
            int need = rand() % 51 + 50; // 50..100
            printf("Poor Student needs $%d\n", need);
            if (need <= localBalance) {
                localBalance -= need;
                *BankAccount = localBalance;
                printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, localBalance);
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
            }
            sem_post(mutex);
        } else { // odd
            sem_wait(mutex);
            localBalance = *BankAccount;
            printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
            sem_post(mutex);
        }
    }
}