//g++ -g -Wall -o main tokenizeStrtok.cpp -lpthread
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

    //Lectura Secuencial
    sem_wait(&sems[my_rank]);
    fg_rv = fgets(my_line, MAX, stdin);
    sem_post(&sems[next]);

    while (fg_rv != NULL)
    {
        cout << "Thread " << my_rank << " > "
             << "my line =" << my_line << endl;

        count = 0;
        my_string = strtok(my_line, " \t\n");

        while (my_string != NULL)
        {
            count++;
            cout << "Thread " << my_rank << " > string " << count << " = " << my_string << endl;
            my_string = strtok(NULL, " \t\n");
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

    // sems = (sem_t *)malloc(thread_count * sizeof(sem_t));
    sems = new sem_t[thread_count];
    
    //Valor disntinto a cero es referido a que se podra compartir entre procesos o threads
    //pero si el valor es 0 que es compartidos solo entre threads

    //Valor de la señal
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
