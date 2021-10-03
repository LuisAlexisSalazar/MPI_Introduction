//g++ mutexNode.cpp  -o main -lpthread -std=c++11
// /main <threads>
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

/* Return values of Advance_ptrs */
const int IN_LIST = 1;
const int EMPTY_LIST = -1;
const int END_OF_LIST = 0;




class list_node_s
{
public:
    int data;
    class list_node_s *next;
    mutex mutex_node;
};

/* Shared variables */
list_node_s *head = NULL;
mutex head_mutex;
int thread_count;
int total_ops;
double insert_percent;
double search_percent;
double delete_percent;
mutex count_mutex;
int member_total = 0, insert_total = 0, delete_total = 0;

void Get_input(int *inserts_in_main_p)
{

    *inserts_in_main_p = 1000;
    total_ops = 100000;

    search_percent = 0.8;
    insert_percent = 0.1;
    delete_percent = 1.0 - (search_percent + insert_percent);
}

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
//Inicializar los punteros a el current y pred(anterior)
void Init_ptrs(list_node_s **curr_pp,list_node_s **pred_pp)
{
    *pred_pp = NULL;
    head_mutex.lock();
    *curr_pp = head;
    if (*curr_pp != NULL)
        (*curr_pp)->mutex_node.lock();
    // pthread_mutex_unlock(&head_mutex);
}

/*-----------------------------------------------------------------*/
// Function:  Avanzar bloquenado los punteros actuales y pred(anterior)

int Advance_ptrs(list_node_s **curr_pp, list_node_s **pred_pp)
{
    int rv = IN_LIST;
    list_node_s *curr_p = *curr_pp;
    list_node_s *pred_p = *pred_pp;

    if (curr_p == NULL)
    {
        if (pred_p == NULL)
        {
            head_mutex.unlock();
            return EMPTY_LIST;
        }
        else
        { /* Not at head of list */
            return END_OF_LIST;
        }
    }
    else
    { // *curr_pp != NULL
        if (curr_p->next != NULL)
            curr_p->next->mutex_node.lock();
        else
            rv = END_OF_LIST;
        if (pred_p != NULL)
            pred_p->mutex_node.unlock();
        else
            head_mutex.unlock();
        *pred_pp = curr_p;
        *curr_pp = curr_p->next;
        return rv;
    }
}

/*-----------------------------------------------------------------*/
bool Is_empty(void)
{
    if (head == NULL)
        return true;
    else
        return false;
}


int Insert(int value)
{
    list_node_s *curr;
    list_node_s *pred;
    list_node_s *temp;
    int rv = 1;

    Init_ptrs(&curr, &pred);

    while (curr != NULL && curr->data < value)
    {
        Advance_ptrs(&curr, &pred);
    }

    if (curr == NULL || curr->data > value)
    {
        temp = new list_node_s;
        // pthread_mutex_init(&(temp->mutex_node), NULL);

        temp->data = value;
        temp->next = curr;
        if (curr != NULL)
            curr->mutex_node.unlock();
        if (pred == NULL)
        {
            // Inserting in head of list
            head = temp;
            head_mutex.unlock();
        }
        else
        {
            pred->next = temp;
            pred->mutex_node.unlock();
        }
    }
    else
    { /* value in list */
        if (curr != NULL)
            curr->mutex_node.unlock();
        if (pred != NULL)
            pred->mutex_node.unlock();
        else
            head_mutex.unlock();
        rv = 0;
    }

    return rv;
}

/*-----------------------------------------------------------------*/
int Member(int value)
{
    list_node_s *temp, *old_temp;
    head_mutex.lock();

    temp = head;
    if (temp != NULL)
        temp->mutex_node.lock();

    head_mutex.unlock();

    while (temp != NULL && temp->data < value)
    {
        if (temp->next != NULL)
            temp->next->mutex_node.lock();
        old_temp = temp;
        temp = temp->next;
        old_temp->mutex_node.unlock();
    }

    if (temp == NULL || temp->data > value)
    {
        if (temp != NULL)
            temp->mutex_node.unlock();
        return 0;
    }
    else
    { /* temp != NULL && temp->data <= value */
        temp->mutex_node.unlock();
        return 1;
    }
}

/*-----------------------------------------------------------------*/
/* Deletes value from list */
/* If value is in list, return 1, else return 0 */
int Delete(int value)
{
    list_node_s *curr;
    list_node_s *pred;
    int rv = 1;

    Init_ptrs(&curr, &pred);

    /* Find value */
    while (curr != NULL && curr->data < value)
    {
        Advance_ptrs(&curr, &pred);
    }

    if (curr != NULL && curr->data == value)
    {
        if (pred == NULL)
        {
            head = curr->next;

            head_mutex.unlock();
            curr->mutex_node.unlock();

            curr->mutex_node.~mutex();
            delete curr;
        }
        else
        { /* pred != NULL */
            pred->next = curr->next;
            pred->mutex_node.unlock();

            curr->mutex_node.unlock();
            curr->mutex_node.~mutex();
            delete curr;
        }
    }
    else
    { /* Not in list */
        if (pred != NULL)
            pred->mutex_node.unlock();
        if (curr != NULL)
            curr->mutex_node.unlock();
        if (curr == head)
            head_mutex.unlock();
        rv = 0;
    }

    return rv;
}

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
        delete current;
        current = following;
        following = current->next;
    }
    delete current;
}


/*-----------------------------------------------------------------*/
void Thread_work(int rank)
{
    long my_rank = rank;
    int i, val;
    double which_op;
    unsigned seed = my_rank + 1;
    int my_member = 0, my_insert = 0, my_delete = 0;
    int ops_per_thread = total_ops / thread_count;

    for (i = 0; i < ops_per_thread; i++)
    {
        which_op = randOperation(mt);
        val = dist(mt);

        if (which_op < search_percent)
        {
            Member(val);
            my_member++;
        }
        else if (which_op < search_percent + insert_percent)
        {
            Insert(val);
            my_insert++;
        }
        else
        {
            Delete(val);
            my_delete++;
        }
    }

    count_mutex.lock();
    member_total += my_member;
    insert_total += my_insert;
    delete_total += my_delete;
    count_mutex.unlock();

} /* Thread_work */

/*-----------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  
    int key, success;
    int inserts_in_main;
    unsigned seed = 1;
    double start, finish;

    thread_count = atoi(argv[1]);

    Get_input(&inserts_in_main);

    long i = 0;
    long attempts = 0;
    // pthread_mutex_init(&head_mutex, NULL);
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

    auto begin = std::chrono::high_resolution_clock::now();

    for (i = 0; i < thread_count; i++)
        thread_handles[i] = thread(Thread_work, i);

    for (i = 0; i < thread_count; i++)
        thread_handles[i].join();

    auto end = std::chrono::high_resolution_clock::now();

     auto timer= std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count()*1e-9;

    cout << "Total de Operaciones " << total_ops << endl;
    cout << "Operaciones de Member (Buscar) " << member_total << endl;
    cout << "Operaciones de insercción " << insert_total << endl;
    cout << "Operaciones de elimnación " << delete_total << endl;
    cout << "Total de tiempo en segundos tomado fue de " << timer << endl;

    writeFile(timer, "mutexNode");


    Free_list();
    head_mutex.~mutex();
    count_mutex.~mutex();
    delete[] thread_handles;
    
    return 0;
} /* main */
