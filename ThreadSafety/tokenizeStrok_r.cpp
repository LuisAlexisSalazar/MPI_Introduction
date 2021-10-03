//g++ -g -Wall -o main tokenizeStrok_r.cpp -lpthread
///main <threads>

/*Ejemplo de entrada
    Pease porridge hot.
    Pease porridge cold.
    Pease porridge in the pot
    Nine days old.
*/

#include <iostream>
#include <string.h>
#include <thread>
#include <semaphore.h>

using namespace std;

const int MAX = 1000;

int thread_count;
sem_t *sems;

void Tokenize(int my_rank)
{
    int count;
    int next = (my_rank + 1) % thread_count;
    char *fg_rv;
    char my_line[MAX];
    char *my_string;
    char *saveptr;

    //Lectura Secuencial
    sem_wait(&sems[my_rank]);
    fg_rv = fgets(my_line, MAX, stdin);
    sem_post(&sems[next]);

    while (fg_rv != NULL)
    {
        printf("Thread %ld > my line = %s", my_rank, my_line);
        count = 0;
        my_string = strtok_r(my_line, " \t\n", &saveptr);

        while (my_string != NULL)
        {
            printf("Thread %ld > string %d = %s\n", my_rank, count, my_string);
            count++;
            my_string = strtok_r(NULL, " \t\n", &saveptr);
        }

        sem_wait(&sems[my_rank]);
        fg_rv = fgets(my_line, MAX, stdin);
        sem_post(&sems[next]);
    }
}

int main(int argc, char *argv[])
{
    thread *thread_handles = new thread[thread_count];

    thread_count = atoi(argv[1]);

    sems = new sem_t[thread_count];
    sem_init(&sems[0], 0, 1);

    for (int i = 1; i < thread_count; i++)
        sem_init(&sems[i], 0, 0);

    cout << "Ingrese el texto completo (ctrl + v)" << endl;
    for (int i = 0; i < thread_count; i++)
        thread_handles[i] = thread(Tokenize, i);

    for (int i = 0; i < thread_count; i++)
        thread_handles[i].join();

    for (int i = 0; i < thread_count; i++)
        sem_destroy(&sems[i]);

    delete thread_handles;
    delete sems;
    return 0;
}
