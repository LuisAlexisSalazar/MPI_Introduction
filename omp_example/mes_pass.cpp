
// g++ -g -Wall -fopenmp -o main mes_pass.cpp
// basado en la inmplemetación by cayrinsqui
#include <iostream>
#include <omp.h>
using namespace std;

int thread_count = 5;
int done_sending = 0;

class node_q
{
public:
    int message;
    int src;
    node_q *next;
};

class queue
{
public:
    int enqueue, dequeue;

    node_q *tail;
    node_q *head;
    node_q *next_p;

    queue()
    {
        enqueue = dequeue = 0;

        head = NULL;
        tail = NULL;
    }
};

queue **array_queue = new queue *[thread_count];

void Enqueue(queue *q, int dato, int src)
{
    node_q *ptr = new node_q;
    ptr->src = src;
    ptr->message = dato;
    ptr->next = NULL;

    if (q->tail == NULL)
    {
        q->head = ptr;
        q->tail = ptr;
    }
    else
    {
        q->tail->next = ptr;
        q->tail = ptr;
    }

    q->enqueue++;
}

int Dequeue(queue *q, int *dato, int *src, int sr)
{
    if (q->head == NULL)
        return 0;

    else
    {
        node_q *ptrT;
        *dato = q->head->message;
        *src = q->head->src;
        ptrT = q->head;

        if (q->head == q->tail)
            q->head = q->tail = NULL;

        else
            q->head = ptrT->next;

        delete (ptrT);
        q->dequeue++;
        return 1;
    }
}

int lookFor(queue *q, int dato, int *src)
{
    node_q *cptr = q->head;
    while (cptr)
    {
        if (cptr->message == dato)
        {
            *src = cptr->src;
            return 1;
        }
        cptr = cptr->next;
    }
    return 0;
}

void deleteQueue(queue *q)
{
    node_q *cptr;
    node_q *cptr2;
    cptr = q->head;
    while (cptr)
    {
        cptr2 = cptr;
        cptr = cptr->next;
        delete (cptr2);
    }
    q->enqueue = q->dequeue = 0;
    q->head = q->tail = NULL;
}

// void sendMessage(queue **array_cola)
void sendMessage()
{
    int mesg = rand();
    int dest = rand() % thread_count;
#pragma omp critical
    Enqueue(array_queue[dest], mesg, dest);
}

void TryR(int *queue_size, int rank)
{
    int *src = new int;
    int *mesg = new int;
    if (*queue_size == 0)
        return;
    else if (*queue_size == 1)
    {
#pragma omp critical
        Dequeue(array_queue[rank], mesg, src, rank);
    }
    else
        Dequeue(array_queue[rank], mesg, src, rank);

    cout<<"Mensaje es ->"<<*mesg<< " para el thread -> "<<*src<<endl;
   
}

int Done(int *queue_size, struct queue *qqq)
{
    *queue_size = qqq->enqueue - qqq->dequeue;
    if (*queue_size == 0 && done_sending == thread_count)
        return 1;
    else
        return 0;
}

int main(int argc, char *argv[])
{
    //CREEMOS LA COLA COMPARTIDA
    int send_max = 10;
    int sent_msgs;

    int *queue_size;

#pragma omp parallel num_threads(thread_count) private(sent_msgs, queue_size) shared(done_sending, array_queue, cout)
    {
        int my_rank = omp_get_thread_num();
        queue_size = new int(0);

        for (int i = 0; i < thread_count; i++)
            array_queue[i] = new queue;

#pragma omp barrier

        cout << "Thread ID -> " << my_rank << endl;

        for (sent_msgs = 0; sent_msgs < send_max; sent_msgs++)
            sendMessage();

#pragma omp atomic
        done_sending = done_sending + 1;
        while (!Done(queue_size, array_queue[my_rank]))
            TryR(queue_size, omp_get_thread_num());
    }

    return 0;
}