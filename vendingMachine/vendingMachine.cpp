//  g++ -g -Wall -fopenmp -o main vendingMachine.cpp
// ./main 
#include <iostream>
#include <omp.h>
#include <queue>
#include <random>

using namespace std;


//?Solo Trabajador al inicio
void fill_machine(queue<int> *machine, int amount_support)
{
    for (int i = 0; i < amount_support; i++)
        machine->push(1);
}


int main()
{
    int thread_count = 5;
    int amount_request_support = 5;
    queue<int> *machine_queue;
   
    int amount_request_per_client = 20;
    int own_request_done;

    int satisfaction_client = 0;
    int total_Request_attended = 0;
    
    omp_lock_t queue_access;
    omp_init_lock(&queue_access);

    #pragma omp parallel num_threads(thread_count) default(none) shared(amount_request_per_client, amount_request_support, machine_queue, thread_count, satisfaction_client, cout,total_Request_attended,queue_access) private(own_request_done)
    {
        own_request_done = 0;
        int my_rank = omp_get_thread_num();

        if (my_rank == 0)
        {
            machine_queue = new queue<int>;
            fill_machine(machine_queue, amount_request_support);
        }
        #pragma omp barrier
        
        if (my_rank == 0)
        {
           
            while (satisfaction_client != thread_count - 1){

                omp_set_lock(&queue_access);
                if (machine_queue->size() == 0)
                         for (int i = 0; i < amount_request_support; i++)
                            machine_queue->push(1);
                omp_unset_lock(&queue_access);

            }
        }
        else
        {   
            //Consumir
            while (own_request_done != amount_request_per_client)
            {
    
                omp_set_lock(&queue_access);
                    if (machine_queue->size() != 0)
                        machine_queue->pop();
                omp_unset_lock(&queue_access);

                own_request_done++;

                #pragma omp atomic
                total_Request_attended++;
                
            }
            
            #pragma omp atomic
            satisfaction_client++;
        }
    }

    omp_destroy_lock(&queue_access);
    cout<<"Total de Mensajes Consumidos: "<<total_Request_attended<<endl;
        
    return 0;
}