#include "mpi.h"
#include <iostream>
#include "stdlib.h"
#include "time.h"

int main()
{
    MPI_Init(NULL, NULL);
    int *A, N, sum, SUM;
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    std::cout<<rank;
    if (rank == 0)
    {
        srand(time(NULL));
        std::cout << "type element count:";
        std::cin >> N;
        if (N < size)
            N = size;
        A = new int[N];
        for (int i = 0; i < N; i++)
        {
            A[i] = random() % 11;
            std::cout << A[i] << " \n";
        }
    }
    MPI_Bcast((void *)&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0)
        A = new int[N];
    MPI_Bcast((void *)&A, N, MPI_INT, 0, MPI_COMM_WORLD); // 0 brd A for all
    sum = 0;
    for (int i = rank; i < N; i += size)
    {
        sum += A[i];
    }
    // non root sends sub sums for root
    if (rank != 0)
        MPI_Send((void *)&sum, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
    if (rank == 0)
    {
        SUM = sum;
        MPI_Status st;
        for (int i = 1; i < size; i++)
        {
            MPI_Recv((void *)&sum, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &st);
            SUM += sum;
        }
        std::cout << "sum=" << SUM;
    }
    delete (A);
    MPI_Finalize();
    return 0;
}