
//g++ mutexEntireList.cpp  -o main -lpthread -std=c++11
// /main <Threads>
//https://www.educative.io/blog/modern-multithreading-and-concurrency-in-cpp

#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <random>
#include<fstream>
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



void writeFile(double &time,string name)
{
    string nameFile = "times/" + name + ".txt";
    ofstream outdata;
    outdata.open(nameFile, std::ios::out | std::ios::app);

    string data = "\n" + to_string(time);
    outdata << data;
    outdata.close();
}



class list_node_s
{
public:
    int data;
    class list_node_s *next;
};

/* Shared variables */
list_node_s *head = NULL;
int amount_thread;
int total_ops;
double insert_percent;
double search_percent;
double delete_percent;

mutex mutex_p;
mutex count_mutex;

// Estadisticas de operaciones
int member_total = 0, insert_total = 0, delete_total = 0;

/*-----------------------------------------------------------------*/
void Get_input(int *inserts_in_main_p)
{

    *inserts_in_main_p = 1000;
    total_ops = 100000;

    search_percent = 0.8;
    insert_percent = 0.1;
    delete_percent = 1.0 - (search_percent + insert_percent);
}

bool Insert(int value)
{
    list_node_s *curr = head;
    list_node_s *pred = NULL;
    list_node_s *temp;
    bool rv = true;

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
    {
        rv = false;
    }

    return rv;
}

/*-----------------------------------------------------------------*/
int Member(int value)
{
    list_node_s *temp;
    temp = head;

    while (temp != NULL && temp->data < value)
        temp = temp->next;

    if (temp == NULL || temp->data > value)
        return 0;
    else
        return 1;
}

bool Delete(int value)
{
    list_node_s *curr = head;
    list_node_s *pred = NULL;

    bool rv = true;

    /* Find value */
    while (curr != NULL && curr->data < value)
    {
        pred = curr;
        curr = curr->next;
    }

    if (curr != NULL && curr->data == value)
    {
        /* first element in list */
        if (pred == NULL)
        {
            head = curr->next;
            delete (curr);
        }
        else
        {
            pred->next = curr->next;
            delete (curr);
        }
    }
    else
        rv = false;
    return rv;
}
/*-----------------------------------------------------------------*/
bool Is_empty()
{
    if (head == NULL)
        return true;
    else
        return false;
}

/*-----------------------------------------------------------------*/
void Free_list()
{
    list_node_s *current;
    list_node_s *following;

    if (Is_empty())
        return;

    current = head;
    following = current->next;
    while (following != NULL)
    {
        delete (current);
        current = following;
        following = current->next;
    }

    delete (current);
}

/*-----------------------------------------------------------------*/
void Thread_work(int rank)
{
    cout << "Thread: " << rank << endl;

    long my_rank = rank;
    int val;
    double which_op;
    int my_member = 0, my_insert = 0, my_delete = 0;
    int ops_per_thread = total_ops / amount_thread;

    for (int i = 0; i < ops_per_thread; i++)
    {
        which_op = randOperation(mt);
        val = dist(mt);

        if (which_op < search_percent)
        {
            mutex_p.lock();
            Member(val);
            mutex_p.unlock();
            my_member++;
        }
        else if (which_op < search_percent + insert_percent)
        {
            mutex_p.lock();
            Insert(val);
            mutex_p.unlock();
            my_insert++;
        }
        else
        { /* delete */
            mutex_p.lock();
            Delete(val);
            mutex_p.unlock();
            my_delete++;
        }
    }

    count_mutex.lock();
    member_total += my_member;
    insert_total += my_insert;
    delete_total += my_delete;
    count_mutex.unlock();
}

/*-----------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    //*Thread Principal no hace operaciones
    long i;
    int key, attempts;
    bool success;

    amount_thread = atoi(argv[1]);

    thread *thread_handles = new thread[amount_thread];
    int inserts_in_main;

  
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

    
    auto begin = std::chrono::high_resolution_clock::now();

    for (i = 0; i < amount_thread; i++)
        thread_handles[i] = thread(Thread_work, i);

    for (i = 0; i < amount_thread; i++)
        thread_handles[i].join();

    auto end = std::chrono::high_resolution_clock::now();

    auto timer= std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count()*1e-9;

    cout << "Total de Operaciones " << total_ops << endl;
    cout << "Operaciones de Member (Buscar) " << member_total << endl;
    cout << "Operaciones de insercción " << insert_total << endl;
    cout << "Operaciones de elimnación " << delete_total << endl;
    cout << "Total de tiempo en segundos tomado fue de " << timer << endl;

    writeFile(timer, "mutexEntireList");

    Free_list();
    mutex_p.~mutex();
    count_mutex.~mutex();
    delete[] thread_handles;

    return 0;
}
