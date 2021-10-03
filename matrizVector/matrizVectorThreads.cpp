#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <utility>
// #include <mutex>
#include <random>
#include <fstream>
using namespace std;

int LIMIT_L = 1;
int LIMIT_R = 1000;
random_device rd;
mt19937 mt(rd());
uniform_real_distribution<double> dist(LIMIT_L, LIMIT_R);

/* Global variables */
int thread_count;
int m, n;
double *A;
double *x;
double *y;

vector<pair<int, int>> amountMatrix;

pair<int, int> firstPair(8000000, 8);
pair<int, int> secondtPair(8000, 8000);
pair<int, int> thridPair(8, 8000000);

void writeFile(double &time, int threads, int indexWork)
{
    string name = to_string(amountMatrix[indexWork].first) + "x" + to_string(amountMatrix[indexWork].second);

    string nameFile = "times/" + name + ".txt";
    ofstream outdata;
    outdata.open(nameFile, std::ios::out | std::ios::app);

    string data = "\n" + to_string(time);
    outdata << data;
    outdata.close();
}

void randomMatrix(double A[], int m, int n)
{
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            A[i * n + j] = dist(mt);
}

void randomVector(double x[], int n)
{
    for (int i = 0; i < n; i++)
        x[i] == dist(mt);
}

// Matriz mxn
// Cada Thread multiplica nx1 column vector
void Pth_mat_vect(int my_rank)
{
    int local_m = m / thread_count;
    int my_first_row = my_rank * local_m;
    int my_last_row = (my_rank + 1) * local_m - 1;

    cout << "Local m: " << local_m << endl;
    cout << "Firsst Row m: " << my_first_row << endl;
    cout << "Last Row: " << my_last_row << endl;

    for (int i = my_first_row; i <= my_last_row; i++)
    {
        y[i] = 0.0;
        for (int j = 0; j < n; j++)
            y[i] += A[i * n + j] * x[j];
    }
}

void Print_matrix(double A[], int m, int n)
{
    cout << "Matriz" << endl;
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            cout << A[i * n + j] << endl;
}

void Print_vector(double y[], int m)
{
    cout << "Vector:" << endl;
    for (int i = 0; i < m; i++)
        cout << y[i] << endl;
    // cout << endl;
}

void setVariables(int index_work)
{
    m = amountMatrix[index_work].first;
    n = amountMatrix[index_work].second;

    x = new double[n];
    y = new double[m];
    A = new double[m * n];
}

/*------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    amountMatrix.push_back(firstPair);
    amountMatrix.push_back(secondtPair);
    amountMatrix.push_back(thridPair);

    thread_count = atoi(argv[1]);
    int index_work = atoi(argv[2]);

    setVariables(index_work);

    thread *thread_handles = new thread[thread_count];

    randomMatrix(A, m, n);
    // Print_matrix(A, m, n);

    randomVector(x, n);
    // Print_vector(x, n);

    auto begin = std::chrono::high_resolution_clock::now();

    for (int index_t = 0; index_t < thread_count; index_t++)
        thread_handles[index_t] = thread(Pth_mat_vect, index_t);

    for (int index_t = 0; index_t < thread_count; index_t++)
        thread_handles[index_t].join();

    auto end = std::chrono::high_resolution_clock::now();

    auto timer = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() * 1e-9;

    cout << "Tiempo: " << timer << endl;

    writeFile(timer, thread_count, index_work);

    delete[] thread_handles;

    return 0;
}
