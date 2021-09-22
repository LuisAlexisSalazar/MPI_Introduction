// mpiCC -g -o main mpimultMV.cpp
// mpiexec -n p ./main index
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <iostream>
#include <random>
#include <fstream>
#include <limits>
using namespace std;

//1024 2048 4096 8192 16384
int row_colum[5] = {1024, 2048, 4096, 8192, 16384};
int index_n;

double limit_l = 1.0;
double limit_r = 1000.0;
random_device rd;
mt19937 mt(rd());
uniform_real_distribution<double> dist(limit_l, limit_r);


void writeFile(int &comm_sz, double &time)
{
    string nameFile = "times/" + to_string(row_colum[index_n]) + ".txt";
    ofstream outdata;
    outdata.open(nameFile, std::ios::out | std::ios::app);

    string data = "\n" + to_string(time);
    outdata << data;
    outdata.close();
}

/*-------------------------------------------------------------------
 * Function:  Get_dims
 * Purpose:   Get the dimensions of the matrix and the vectors from
 *            stdin.
 * In args:   my_rank:   calling processes rank in comm
 *            comm_sz:   number of processes in comm
 *            comm:      communicator containing all processes calling
 *                       Get_dims
 * Out args:  m_p:       global number of rows of A and components in y
 *            local_m_p: local number of rows of A and components of y
 *            n_p:       global number of cols of A and components of x
 *            local_n_p: local number of components of x */
void Get_dims(int *m_p, int *local_m_p, int *n_p, int *local_n_p, int my_rank, int comm_sz, MPI_Comm comm)
{
    int local_ok = 1;

    if (my_rank == 0)
    {
        *m_p = row_colum[index_n];
        *n_p = row_colum[index_n];
    }

    //!Broadcast: Enviamos y recibimos
    MPI_Bcast(m_p, 1, MPI_INT, 0, comm);
    MPI_Bcast(n_p, 1, MPI_INT, 0, comm);

    *local_m_p = *m_p / comm_sz;
    *local_n_p = *n_p / comm_sz;
}

/*-------------------------------------------------------------------
 * Function:   Allocate_arrays
 * Purpose:    Allocate storage for local parts of A, x, and y
 * In args:    local_m:    local number of rows of A and components of y
 *             n:          global and local number of cols of A and global
 *                         number of components of x
 *             local_n:    local number of components of x
 *             comm:       communicator containing all calling processes
 * Out args:   local_A_pp: local storage for matrix (m/comm_sz rows, n cols)
 *             local_x_pp: local storage for x (n/comm_sz components)
 *             local_y_pp: local_storage for y (m/comm_sz components) 
 */
void Allocate_arrays(double **local_A_pp, double **local_x_pp, double **local_y_pp, int local_m, int n, int local_n, MPI_Comm comm)
{

    *local_A_pp = new double[local_m * n];
    *local_x_pp = new double[local_n];
    *local_y_pp = new double[local_m];
}

/*-------------------------------------------------------------------
 * Function:  Read_matrix
 * Purpose:   Read in the matrix and distribute among the processes
 *            using a block row distribution
 * In args:   prompt:  description of matrix (e.g., "A")
 *            m:       global number of rows of A
 *            local_m: local number of rows of A
 *            n:       global and local number of cols of A
 *            my_rank: process rank in communicator comm
 *            comm:    communicator containing processes calling
 *                     Read_matrix
 * Out args:  local_A: the local matrix */
void Read_matrix(double local_A[], int m, int local_m, int n, int my_rank, MPI_Comm comm)
{
    double *A = NULL;
    int local_ok = 1;
    int i, j;

    if (my_rank == 0)
    {
        A = new double[m * n];
        if (A == NULL)
            local_ok = 0;

        for (i = 0; i < m; i++)
            for (j = 0; j < n; j++)
                A[i * n + j] = dist(mt);

        // !Scatter:distribución de datos y recibimiento de datos partidos
        MPI_Scatter(A, local_m * n, MPI_DOUBLE, local_A, local_m * n, MPI_DOUBLE, 0, comm);
        free(A);
    }

    else
    {
        MPI_Scatter(A, local_m * n, MPI_DOUBLE, local_A, local_m * n, MPI_DOUBLE, 0, comm);
    }
}

/*-------------------------------------------------------------------
 * Function:  Read_vector
 * Purpose:   Read a vector from stdin and distribute among the
 *            processes using a block distribution
 * In args:   prompt:  description of vector (e.g., "x")
 *            n:       global order of vector
 *            local_n: local order of vector (n/comm_sz) */
void Read_vector(double local_vec[], int n, int local_n, int my_rank, MPI_Comm comm)
{
    double *vec = NULL;
    int i, local_ok = 1;

    if (my_rank == 0)
    {
        vec = new double[n];
        if (vec == NULL)
            local_ok = 0;

        for (i = 0; i < n; i++)
            vec[i] = dist(mt);

        // !Scateer
        MPI_Scatter(vec, local_n, MPI_DOUBLE, local_vec, local_n, MPI_DOUBLE, 0, comm);
        free(vec);
    }
    else
    {
        MPI_Scatter(vec, local_n, MPI_DOUBLE, local_vec, local_n, MPI_DOUBLE, 0, comm);
    }
}

void Get_VectorResult(double local_vec[], int n, int local_n, int my_rank, MPI_Comm comm)
{
    double *vec = NULL;
    int i, local_ok = 1;

    if (my_rank == 0)
    {
        vec = new double[n];

        // !Recolección de datos de un conjunto
        MPI_Gather(local_vec, local_n, MPI_DOUBLE, vec, local_n, MPI_DOUBLE, 0, comm);

        // printf("\nThe vector y\n");
        // for (i = 0; i < n; i++)
        //     printf("%f ", vec[i]);
        printf("\n");
        free(vec);
    }
    else
    {
        MPI_Gather(local_vec, local_n, MPI_DOUBLE, vec, local_n, MPI_DOUBLE, 0, comm);
    }
}

/*-------------------------------------------------------------------
 * Function:  Mat_vect_mult
 * Purpose:   Multiply a matrix A by a vector x.  The matrix is distributed
 *            by block rows and the vectors are distributed by blocks
 * In args:   local_A:  calling process' rows of matrix A
 *            local_x:  calling process' components of vector x
 *            local_m:  calling process' number of rows 
 *            n:        global (and local) number of columns
 *            local_n:  calling process' number of components of x
 *            comm:     communicator containing all calling processes */
void Mat_vect_mult(double local_A[], double local_x[], double local_y[], int local_m, int n, int local_n, MPI_Comm comm)
{
    double *x;
    int local_i, j;
    int local_ok = 1;

    x = new double[n];

    //!Allgatger:Recolecta todos los datos y envía a todo el proceso lo que se recolecto
    MPI_Allgather(local_x, local_n, MPI_DOUBLE, x, local_n, MPI_DOUBLE, comm);

    for (local_i = 0; local_i < local_m; local_i++)
    {
        local_y[local_i] = 0.0;
        for (j = 0; j < n; j++)
            local_y[local_i] += local_A[local_i * n + j] * x[j];
    }
    free(x);
}

/*-------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    index_n = stoi(argv[1]);

    double *local_A;
    double *local_x;
    double *local_y;
    int m, local_m, n, local_n;
    int my_rank, comm_sz;
    MPI_Comm comm;

    MPI_Init(NULL, NULL);
    comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &comm_sz);
    MPI_Comm_rank(comm, &my_rank);

    Get_dims(&m, &local_m, &n, &local_n, my_rank, comm_sz, comm);
    Allocate_arrays(&local_A, &local_x, &local_y, local_m, n, local_n, comm);
    Read_matrix(local_A, m, local_m, n, my_rank, comm);
    Read_vector(local_x, n, local_n, my_rank, comm);

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    Mat_vect_mult(local_A, local_x, local_y, local_m, n, local_n, comm);

    MPI_Barrier(MPI_COMM_WORLD);
    double end = MPI_Wtime();
    if (my_rank == 0)
    {
        double time = (end - start) * 1000;

        cout << "El proceso tomo " << time << " miliseconds to run." << std::endl;

        writeFile(comm_sz, time);
    }
    Get_VectorResult(local_y, m, local_m, my_rank, comm);

    free(local_A);
    free(local_x);
    free(local_y);
    MPI_Finalize();

    return 0;
}
