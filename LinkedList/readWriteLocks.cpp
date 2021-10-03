//g++ mutexNode.cpp  -o main -lpthread -std=c++11
// /main <threads>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <random>
#include <fstream>

using namespace std;
using chrono::duration;
using chrono::duration_cast;
using chrono::high_resolution_clock;
using chrono::milliseconds;
using chrono::seconds;

double LIMIT_L = 1.0;
double LIMIT_R = 10000000.0;
random_device rd;
mt19937 mt(rd());
uniform_real_distribution<double> dist(LIMIT_L, LIMIT_R);
uniform_real_distribution<> randOperation(0.0, 1.0);

class list_node_s
{
public:
    int data;
    class list_node_s *next;
    mutex mutex_node;
};

/* Shared variables */
list_node_s *head = NULL;
int thread_count;
int total_ops;
double insert_percent;
double search_percent;
double delete_percent;

// Read-Write Lock
pthread_rwlock_t rwlock;
mutex count_mutex;

int member_count = 0, insert_count = 0, delete_count = 0;

void writeFile(double &time, string name)
{
    string nameFile = "times/" + name + ".txt";
    ofstream outdata;
    outdata.open(nameFile, std::ios::out | std::ios::app);

    string data = "\n" + to_string(time);
    outdata << data;
    outdata.close();
}


/*-----------------------------------------------------------------*/
void Get_input(int *inserts_in_main_p)
{

    *inserts_in_main_p = 1000;
    total_ops = 100000;

    search_percent = 0.8;
    insert_percent = 0.1;
    delete_percent = 1.0 - (search_percent + insert_percent);
} /* Get_input */

int Insert(int value)
{
    list_node_s *curr = head;
    list_node_s *pred = NULL;
    list_node_s *temp;
    int rv = 1;

    while (curr != NULL && curr->data < value)
    {
        pred = curr;
        curr = curr->next;
    }

    if (curr == NULL || curr->data > value)
    {
        temp = new list_node_s;
        temp->data = value;
        temp->next = curr;
        if (pred == NULL)
            head = temp;
        else
            pred->next = temp;
    }
    else
        rv = 0;
    return rv;
}

bool Member(int value)
{
    list_node_s *temp;

    temp = head;
    while (temp != NULL && temp->data < value)
        temp = temp->next;

    if (temp == NULL || temp->data > value)
        return false;
    else
        return true;
}

int Delete(int value)
{
    list_node_s *curr = head;
    list_node_s *pred = NULL;
    int rv = 1;

    while (curr != NULL && curr->data < value)
    {
        pred = curr;
        curr = curr->next;
    }

    if (curr != NULL && curr->data == value)
    {
        if (pred == NULL)
        { /* first element in list */
            head = curr->next;
            delete curr;
        }
        else
        {
            pred->next = curr->next;
            delete curr;
        }
    }
    else
        rv = 0;

    return rv;
}


bool Is_empty()
{
    if (head == NULL)
        return true;
    else
        return false;
}

/*-----------------------------------------------------------------*/
void Free_list(void)
{
    list_node_s *current;
    list_node_s *following;

    if (Is_empty())
        return;
    current = head;
    following = current->next;
    while (following != NULL)
    {
        delete current;
        current = following;
        following = current->next;
    }
    delete current;
}



void Thread_work(long rank)
{
    long my_rank = rank;
    int val;
    double which_op;
    int my_member_count = 0, my_insert_count = 0, my_delete_count = 0;
    int ops_per_thread = total_ops / thread_count;

    for (int i = 0; i < ops_per_thread; i++)
    {
        which_op = randOperation(mt);
        val = dist(mt);

        if (which_op < search_percent)
        {
            //reading
            pthread_rwlock_rdlock(&rwlock);
            Member(val);
            //Liberación ambos
            pthread_rwlock_unlock(&rwlock);
            my_member_count++;
        }
        else if (which_op < search_percent + insert_percent)
        {
            //writting
            pthread_rwlock_wrlock(&rwlock);
            Insert(val);
            //Liberación ambos
            pthread_rwlock_unlock(&rwlock);
            my_insert_count++;
        }
        else
        {
            //writting
            pthread_rwlock_wrlock(&rwlock);
            Delete(val);
            pthread_rwlock_unlock(&rwlock);
            my_delete_count++;
        }
    } 

    count_mutex.lock();
    member_count += my_member_count;
    insert_count += my_insert_count;
    delete_count += my_delete_count;
    count_mutex.unlock();

   
}

/*-----------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    long i;
    int key, success, attempts;
    int inserts_in_main;

    thread_count = atoi(argv[1]);
    Get_input(&inserts_in_main);

                             
    i = attempts = 0;
    while (i < inserts_in_main && attempts < 2 * inserts_in_main)
    {
        key = dist(mt);
        success = Insert(key);
        attempts++;
        if (success)
            i++;
    }
    cout << "Se insertaron " << i << " elementos en la lista" << endl;

    thread *thread_handles = new thread[thread_count];
    
    pthread_rwlock_init(&rwlock, NULL);

    auto begin = std::chrono::high_resolution_clock::now();

    for (i = 0; i < thread_count; i++)
        thread_handles[i]= thread(Thread_work,i);

    for (i = 0; i < thread_count; i++)
        thread_handles[i].join();
    
    auto end = std::chrono::high_resolution_clock::now();

     auto timer= std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count()*1e-9;

    cout << "Total de Operaciones " << total_ops << endl;
    cout << "Operaciones de Member (Buscar) " << member_count << endl;
    cout << "Operaciones de insercción " << insert_count << endl;
    cout << "Operaciones de elimnación " << delete_count << endl;
    cout << "Total de tiempo en segundos tomado fue de " << timer << endl;

    writeFile(timer, "readWriteLock");

    Free_list();
    pthread_rwlock_destroy(&rwlock);
    count_mutex.~mutex();
    delete[] thread_handles;

   return 0;
}
