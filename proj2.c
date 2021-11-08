#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>

// prints adequate error on stderr
void print_error(int code)
{
    char* message;
    switch (code)
    {
        case 1:
            message = "Error E01: invalid arguments! Use \"./proj2 NE NR TE TR\"\n";
            break;
        case 2:
            message = "Error E02: arguments are not integers!\n";
            break;
        case 3:
            message = "Error E03: argument out of range! 0<NE<1000 & 0<NR<20 & 0<=TE<=1000 & 0<=RE<=1000\n";
            break;
    }

    fprintf(stderr, "%s", message);
    exit(1);
}

// returns true if input is an integer or a string of integers
bool is_integer(char* string)
{
    bool return_value = true;
    for (size_t i = 0; i < strlen(string); i++)
    {
        if (string[i] < 48 || string[i] > 57)
            return_value = false;
    }
    return return_value;
}

// parses arguments into variables
void parse_args(char* argv[], int* NE, int* NR, int* TE, int* TR)
{
    *NE = atoi(argv[1]);
    *NR = atoi(argv[2]);
    *TE = atoi(argv[3]);
    *TR = atoi(argv[4]);
    return;
}

// function that is called by process Santa
void santa_func(FILE* fp, int* counter, sem_t* semaphores, int* predicates, int NR)
{
    // start
    sem_wait(&semaphores[0]);
    fprintf(fp, "%d: Santa: going to sleep\n", *counter);
    fflush(fp);
    counter[0]++;
    sem_post(&semaphores[0]);

    // lifecycle loop
    int christmas = 1;
    while (christmas != 0)
    {
        // wait for signal
        sem_wait(&semaphores[2]);
        if (predicates[1] == 0)     // if Santa should close workshop
        {
            christmas = 0;
            predicates[0] = 0;
            sem_wait(&semaphores[5]);
            sem_wait(&semaphores[0]);
            fprintf(fp, "%d: Santa: closing workshop\n", *counter);
            sem_post(&semaphores[9]);   // holiday started
            fflush(fp);
            counter[0]++;
            sem_post(&semaphores[0]);

            // hitching reindeers
            for (int i = 0; i < NR; i++)
            {
                sem_wait(&semaphores[6]);
                counter[2] = i;
                sem_post(&semaphores[6]);
                sem_post(&semaphores[7]);
                sem_wait(&semaphores[4]);
            }

            // releasing elves in queue
            sem_post(&semaphores[3]);
            sem_post(&semaphores[3]);
            sem_post(&semaphores[3]);
        }
        else
        {
            // help elves
            sem_wait(&semaphores[0]);
            fprintf(fp, "%d: Santa: helping elves\n", *counter);
            fflush(fp);
            counter[0]++;
            sem_post(&semaphores[0]);
            counter[1] = 0;

            // helping elves
            for (int i = 0; i < 3; i++)
            {
                sem_wait(&semaphores[6]);
                counter[3] = i;
                sem_post(&semaphores[6]);
                sem_post(&semaphores[3]);
                sem_post(&semaphores[1]);
                sem_wait(&semaphores[8]);
            }

            // wait until sleep
            sem_wait(&semaphores[0]);
            fprintf(fp, "%d: Santa: going to sleep\n", *counter);
            fflush(fp);
            counter[0]++;
            sem_post(&semaphores[0]);
        }
    }

    // starting Christmas
    sem_wait(&semaphores[5]);
    sem_wait(&semaphores[0]);
    fprintf(fp, "%d: Santa: Christmas started\n", *counter);
    fflush(fp);
    counter[0]++;
    sem_post(&semaphores[0]);
    // end
    return;
}

// function that is called by process Elf
void elf_func(FILE* fp, int* counter, int elf_n, sem_t* semaphores, int* predicates, int TE)
{
    // start
    sem_wait(&semaphores[0]);
    fprintf(fp, "%d: Elf %d: started\n", *counter, elf_n);
    fflush(fp);
    counter[0]++;
    sem_post(&semaphores[0]);

    // the cycle starts
    int count = 0;
    int random_number = 0;
    int workshop = 1;
    while (workshop != 0)
    {
        // sleep
        srand(count + elf_n);
        count++;
        random_number = rand() % (TE + 1);
        usleep(1000*random_number);

        // need help
        sem_wait(&semaphores[0]);
        fprintf(fp, "%d: Elf %d: need help\n", *counter, elf_n);
        fflush(fp);
        counter[0]++;
        sem_post(&semaphores[0]);

        // wait in queue
        sem_wait(&semaphores[1]);
        sem_wait(&semaphores[6]);
        counter[1]++;
        sem_post(&semaphores[6]);

        if (predicates[0] != 0)  // if the workshop is open
        {        
            if (counter[1] == 3) // if 3 elves are in queue
            {
                // notify Santa
                sem_post(&semaphores[2]);
            }
            // get help
            sem_wait(&semaphores[3]);
            if (predicates[0] != 0)     // if the workshop is open
            {
                sem_wait(&semaphores[0]);
                fprintf(fp, "%d: Elf %d: get help\n", *counter, elf_n);
                fflush(fp);
                counter[0]++;
                sem_post(&semaphores[0]);

                if (counter[3] == 2)    // if last one leaving
                {
                    // notify Santa
                    sem_wait(&semaphores[6]);
                    counter[3] = 0;
                    sem_post(&semaphores[6]);
                    sem_post(&semaphores[8]);
                }
                else
                    sem_post(&semaphores[8]);
            }
        }
        if (predicates[0] == 0)
            workshop = 0;
    }
    // take holidays
    sem_wait(&semaphores[9]);
    sem_post(&semaphores[1]);
    sem_wait(&semaphores[0]);
    fprintf(fp, "%d: Elf %d: taking holidays\n", *counter, elf_n);
    fflush(fp);
    counter[0]++;
    sem_post(&semaphores[0]);
    sem_post(&semaphores[9]);

    // end
    return;
}

// function that is called by process Reindeer
void rd_func(FILE* fp, int* counter, int rd_n, sem_t* semaphores, int* predicates, int NR, int TR)
{   
    // start
    sem_wait(&semaphores[0]);
    fprintf(fp, "%d: RD %d: rstarted\n", *counter, rd_n);
    fflush(fp);
    counter[0]++;
    sem_post(&semaphores[0]);

    // sleep
    srand(rd_n);
    int random_number = rand() % ((TR/2) + 1);
    random_number += (TR/2);
    usleep(1000*random_number);

    // return home
    sem_wait(&semaphores[4]);
    sem_wait(&semaphores[0]);
    fprintf(fp, "%d: RD %d: return home\n", *counter, rd_n);
    fflush(fp);
    counter[0]++;
    sem_post(&semaphores[0]);
    sem_wait(&semaphores[6]);
    counter[2]++;
    sem_post(&semaphores[6]);

    // last reindeer awakens Santa
    if (counter[2] == NR)
    {
        predicates[1] = 0;
        sem_post(&semaphores[5]);
        sem_post(&semaphores[2]);
    }

    // get hitched
    sem_wait(&semaphores[7]);
    sem_wait(&semaphores[0]);
    fprintf(fp, "%d: RD %d: get hitched\n", *counter, rd_n);
    fflush(fp);
    counter[0]++;
    sem_post(&semaphores[0]);

    // last reindeer informs Santa
    if (counter[2] == (NR - 1))
    {
        sem_post(&semaphores[5]);
        sem_post(&semaphores[4]);
    }
    else
    {
        sem_post(&semaphores[4]);
    }
    // end
    return;
}

// main() function
int main(int argc, char* argv[])
{
    // input checking
    if (argc != 5)
    {
        print_error(1);
    }
    else if (is_integer(argv[1]) == false || is_integer(argv[2]) == false || is_integer(argv[3]) == false || is_integer(argv[4]) == false)
    {
        print_error(2);
    }
    else if (atoi(argv[1]) <= 0 || atoi(argv[1]) >= 1000 ||
            atoi(argv[2]) <= 0 || atoi(argv[2]) >= 20 ||
            atoi(argv[3]) < 0 || atoi(argv[3]) > 1000 ||
            atoi(argv[4]) < 0 || atoi(argv[4]) > 1000)
    {
        print_error(3);
    }

    // declaration and initialisation of some important variables
    int NE, NR, TE, TR;
    parse_args(argv, &NE, &NR, &TE, &TR);

    // creating proj2.out
    FILE* fp = fopen("proj2.out", "w+");

    // creating and initialising semaphores
    sem_t elves_in_sem;
    sem_t santa_mutex;
    sem_t counter_sem;
    sem_t elves_out_sem;
    sem_t rd_in_sem;
    sem_t rd_mutex;
    sem_t counter_mutex;
    sem_t rd_out_sem;
    sem_t santa_out_mutex;
    sem_t holiday;

    sem_init(&elves_in_sem, 1, 3);
    sem_init(&santa_mutex, 1, 0);
    sem_init(&counter_sem, 1, 1);
    sem_init(&elves_out_sem, 1, 0);
    sem_init(&rd_in_sem, 1, NR);
    sem_init(&rd_mutex, 1, 0);
    sem_init(&counter_mutex, 1, 1);
    sem_init(&rd_out_sem, 1, 0);
    sem_init(&santa_out_mutex, 1, 0);
    sem_init(&holiday, 1, 0);

    // creating and initialising shared memory
    int* counter = mmap(NULL, 4*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    counter[0] = 1;     // total counter
    counter[1] = 0;     // counter for elves
    counter[2] = 0;     // counter for reindeers
    counter[3] = 0;     // temp counter

    int* predicates = mmap(NULL, 2*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    predicates[0] = 1;  // workshop_open
    predicates[1] = 1;  // rd_ready      

    sem_t* semaphores = mmap(NULL, 10*sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    semaphores[0] = counter_sem;
    semaphores[1] = elves_in_sem;
    semaphores[2] = santa_mutex;
    semaphores[3] = elves_out_sem;
    semaphores[4] = rd_in_sem;
    semaphores[5] = rd_mutex;
    semaphores[6] = counter_mutex;
    semaphores[7] = rd_out_sem;
    semaphores[8] = santa_out_mutex;
    semaphores[9] = holiday;

    // creating processes
    int santa_id = fork();
    int elf_id = 1;
    int elf_number;
    int rd_id = 1;
    int rd_number;
    if (santa_id != 0)
    {
        // elves
        for (int i = 0; i < NE; i++)
        {
            if (elf_id != 0)
            {
                elf_id = fork();
                elf_number = i + 1;
            }
        }

        // reindeers
        for (int i = 0; i < NR; i++)
        {
            if (elf_id != 0 && rd_id != 0)
            {
                rd_id = fork();
                rd_number = i + 1;
            }
        }
    }

    // process Santa
    if (santa_id == 0)
    {
        santa_func(fp, counter, semaphores, predicates, NR);
    }
    // process Elf
    else if (elf_id == 0)
    {
        elf_func(fp, counter, elf_number, semaphores, predicates, TE);
    }
    // process Reindeer
    else if (rd_id == 0)
    {
        rd_func(fp, counter, rd_number, semaphores, predicates, NR, TR);
    }
    // waiting for all child processes to terminate
    else
    {
        int process_count = 1 + 1 + NE + NR;
        while (wait(NULL) != -1)
        {
            process_count--;
            if (process_count == 1)
            {
                // unmapping
                munmap(counter, 4*sizeof(int));
                munmap(predicates, 2*sizeof(int));
                munmap(semaphores, 10*sizeof(sem_t));

                // destroying semaphores
                sem_destroy(&elves_in_sem);
                sem_destroy(&santa_mutex);
                sem_destroy(&counter_sem);
                sem_destroy(&elves_out_sem);
                sem_destroy(&rd_in_sem);
                sem_destroy(&rd_mutex);
                sem_destroy(&counter_mutex);
                sem_destroy(&rd_out_sem);
                sem_destroy(&santa_out_mutex);
                sem_destroy(&holiday);

                // exiting program
                fclose(fp);
                return 0;
            }
        }
    }
}