// *Compilar
// mpiCC -g -o main mpioddEven.cpp
// ?Index determina cuantos elementos de la lista queires tener [200,400,800,1600,3200]
// *Ejecución
// mpiexec -n <p> main <index> 
// mpiexec -n 2 main 0

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <mpi.h>
#include <string.h>
#include <random>
#include <fstream>
#include <algorithm>
using namespace std;

int limit_l = 1;
int limit_r = 10000;

int n_elements[5] = {200, 400, 800, 1600, 3200};
int index_n;
random_device rd;
mt19937 mt(rd());
uniform_real_distribution<> dist(limit_l, limit_r);

void writeFile(int &comm_sz, double &time)
{
   string nameFile = "times/" + to_string(n_elements[index_n]) + ".txt";
   ofstream outdata;
   outdata.open(nameFile, std::ios::out | std::ios::app);

   string data = "\n" + to_string(time);
   outdata << data;
   outdata.close();
}

void Generate_list(int local_A[], int local_n)
{
   for (int i = 0; i < local_n; i++)
      local_A[i] = dist(mt);
}

void set_array(int argc, char *argv[], int *global_n_p, int *local_n_p, int my_rank, int p, MPI_Comm comm)
{
   if (my_rank == 0)
      *global_n_p = n_elements[index_n];

   MPI_Bcast(global_n_p, 1, MPI_INT, 0, comm);

   *local_n_p = *global_n_p / p;
}

void Print_global_list(int *A, int local_A[], int local_n, int my_rank, int p, MPI_Comm comm)
{

   int i, n;

   if (my_rank == 0)
   {

      // Unico que espera recibir, 7° parametro es el destino
      MPI_Gather(local_A, local_n, MPI_INT, A, local_n, MPI_INT, 0, comm);

      cout << "Global list:\n";

      for (i = 0; i < n; i++)
         printf("%d ", A[i]);
      printf("\n\n");
      free(A);
   }
   else
   {
      MPI_Gather(local_A, local_n, MPI_INT, A, local_n, MPI_INT, 0,
                 comm);
   }
}

void getMatrix(int *A, int local_A[], int local_n, int my_rank, int p, MPI_Comm comm)
{
   if (my_rank == 0)
   {
      // Unico que espera recibir, 7° parametro es el destino
      MPI_Gather(local_A, local_n, MPI_INT, A, local_n, MPI_INT, 0, comm);
   }
   else
   {
      MPI_Gather(local_A, local_n, MPI_INT, A, local_n, MPI_INT, 0,
                 comm);
   }
}

bool ascendente(int i, int j)
{
   return i < j;
}

bool descente(int i, int j)
{
   return i > j;
}

// Uniendo los valores mayores
void Merge_high(int local_A[], int temp_B[], int temp_C[], int local_n)
{
   int ai, bi, ci;

   ai = local_n - 1;
   bi = local_n - 1;
   ci = local_n - 1;
   while (ci >= 0)
   {
      if (local_A[ai] >= temp_B[bi])
      {
         temp_C[ci] = local_A[ai];
         ci--;
         ai--;
      }
      else
      {
         temp_C[ci] = temp_B[bi];
         ci--;
         bi--;
      }
   }

   memcpy(local_A, temp_C, local_n * sizeof(int));
} 

// Uniendo los valores menores
void Merge_low(int my_keys[], int recv_keys[], int temp_keys[], int local_n)
{
   int m_i, r_i, t_i;

   m_i = r_i = t_i = 0;
   while (t_i < local_n)
   {
      if (my_keys[m_i] <= recv_keys[r_i])
      {
         temp_keys[t_i] = my_keys[m_i];
         t_i++;
         m_i++;
      }
      else
      {
         temp_keys[t_i] = recv_keys[r_i];
         t_i++;
         r_i++;
      }
   }

   memcpy(my_keys, temp_keys, local_n * sizeof(int));
} 

void Odd_even_iter(int local_A[], int temp_B[], int temp_C[], int local_n, int phase, int even_partner, int odd_partner, int my_rank, int p, MPI_Comm comm)
{
   MPI_Status status;
   //* even phase
   if (phase % 2 == 0)
   {
      if (even_partner >= 0)
      {
         MPI_Sendrecv(local_A, local_n, MPI_INT, even_partner, 0, temp_B, local_n, MPI_INT, even_partner, 0, comm, &status);

         if (my_rank % 2 != 0)
            Merge_high(local_A, temp_B, temp_C, local_n);
         else
            Merge_low(local_A, temp_B, temp_C, local_n);
      }
   }
   else
   { //* odd phase
      if (odd_partner >= 0)
      {
         MPI_Sendrecv(local_A, local_n, MPI_INT, odd_partner, 0,
                      temp_B, local_n, MPI_INT, odd_partner, 0, comm,
                      &status);
         if (my_rank % 2 != 0)
            Merge_low(local_A, temp_B, temp_C, local_n);
         else
            Merge_high(local_A, temp_B, temp_C, local_n);
      }
   }
}

void Sort(int local_A[], int local_n, int my_rank, int p, MPI_Comm comm)
{
   int phase;
   int *temp_B, *temp_C;
   int even_partner; 
   int odd_partner;  

   temp_B = new int[local_n];
   temp_C = new int[local_n];

   /* Find partners:  negative rank => do nothing during phase */
   // Si el socio es -1 el procesos debera ser inactivo
   if (my_rank % 2 != 0)
   {
      even_partner = my_rank - 1;
      odd_partner = my_rank + 1;
      if (odd_partner == p)
         odd_partner = MPI_PROC_NULL; // Idle during odd phase
   }
   else
   {
      even_partner = my_rank + 1;
      if (even_partner == p)
         even_partner = MPI_PROC_NULL; // Idle during even phase
      odd_partner = my_rank - 1;
   }

   sort(local_A, local_A + local_n, ascendente);

   for (phase = 0; phase < p; phase++)
      Odd_even_iter(local_A, temp_B, temp_C, local_n, phase, even_partner, odd_partner, my_rank, p, comm);

   free(temp_B);
   free(temp_C);
} /* Sort */


void Print_list(int local_A[], int local_n, int rank)
{
   cout << rank << " : ";
   for (int i = 0; i < local_n; i++)
      cout << local_A[i] << " ";

   cout << endl;
}

void onlyPrintList(int local_A[], int local_n)
{
   cout << "Arreglo Final" << endl;
   for (int i = 0; i < local_n; i++)
      cout << local_A[i] << " ";

   cout << endl;
}

void Print_local_lists(int local_A[], int local_n, int my_rank, int p, MPI_Comm comm)
{
   int *A;
   MPI_Status status;

   if (my_rank == 0)
   {
      A = new int[local_n];
      Print_list(local_A, local_n, my_rank);

      for (int q = 1; q < p; q++)
      {
         MPI_Recv(A, local_n, MPI_INT, q, 0, comm, &status);
         Print_list(A, local_n, q);
      }

      delete A;
   }
   else
   {
      MPI_Send(local_A, local_n, MPI_INT, 0, 0, comm);
   }
}

/*-------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
   int my_rank, p;
   int *local_A;
   int global_n;
   int local_n;
   MPI_Comm comm;

   index_n = stoi(argv[1]);

   MPI_Init(NULL, NULL);

   comm = MPI_COMM_WORLD;
   MPI_Comm_size(comm, &p);
   MPI_Comm_rank(comm, &my_rank);

   set_array(argc, argv, &global_n, &local_n, my_rank, p, comm);

   local_A = new int[local_n];

   Generate_list(local_A, local_n);

   // Print_local_lists(local_A, local_n, my_rank, p, comm);

   // ?Start Time
   MPI_Barrier(MPI_COMM_WORLD);
   double start = MPI_Wtime();

   Sort(local_A, local_n, my_rank, p, comm);

   MPI_Barrier(MPI_COMM_WORLD);
   double end = MPI_Wtime();

   int *A;
   A = new int[p * local_n];
   getMatrix(A, local_A, local_n, my_rank, p, comm);

   if (my_rank == 0)
   {
      // onlyPrintList(A, local_n*p);
       // ?Start Time
      double time = (end - start) * 1000;

      cout << "El proceso tomo " << time << " miliseconds to run." << std::endl;
      writeFile(p, time);
   }

   free(local_A);

   MPI_Finalize();

   return 0;
}
