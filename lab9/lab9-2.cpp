#include <iostream>
#include <vector>
#include <cmath>
#include <mpi.h>

using namespace std;

const double EPSILON = 0.1;

void distribute_data(int m, int n, int process, int num_procs, 
                    vector<double>& A, vector<double>& B,
                    vector<double>& local_A, vector<double>& local_B) {
    int* sendcounts = new int[num_procs];
    int* displs = new int[num_procs];
    
    int rows_per_proc = m / num_procs;
    int remainder = m % num_procs;

    for(int i = 0; i < num_procs; ++i) {
        sendcounts[i] = (i < remainder) ? (rows_per_proc + 1) * n : rows_per_proc * n;
        displs[i] = (i == 0) ? 0 : displs[i-1] + sendcounts[i-1];
    }

    if(process == 0) {
        MPI_Scatterv(A.data(), sendcounts, displs, MPI_DOUBLE,
                    local_A.data(), local_A.size(), MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Scatterv(B.data(), sendcounts, displs, MPI_DOUBLE,
                    local_B.data(), local_B.size(), MPI_DOUBLE, 0, MPI_COMM_WORLD);
    } else {
        MPI_Scatterv(nullptr, nullptr, nullptr, MPI_DOUBLE,
                    local_A.data(), local_A.size(), MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Scatterv(nullptr, nullptr, nullptr, MPI_DOUBLE,
                    local_B.data(), local_B.size(), MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    delete[] sendcounts;
    delete[] displs;
}

bool iterate(vector<double>& local_A, vector<double>& local_B, 
            int m, int n, double e, int global_start, int local_rows) {
    bool local_converged = true;

    for(int i = 0; i < local_rows; ++i) {
        for(int j = 0; j < n; ++j) {
            int idx = i * n + j;
            double new_val;
            int global_row = global_start + i;
            
            if(global_row < m - 1) {
                new_val = (local_A[idx] + local_B[idx] + local_B[idx]) / 3.0;
            } else {
                new_val = (local_A[idx] + local_B[idx]) / 2.0;
            }
            
            local_A[idx] = new_val;
            if(fabs(new_val - local_B[idx]) > e) {
                local_converged = false;
            }
        }
    }
    return local_converged;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int m = 4096;
    int n = 4096;
    double e = EPSILON;

    vector<double> A, B;
    vector<double> local_A, local_B;
    int local_rows = 0;
    int global_start = 0;

    if(rank == 0) {
        A.resize(m * n);
        B.resize(m * n);
        
        for(int i = 0; i < m * n; ++i) {
            A[i] = static_cast<double>(rand()) / RAND_MAX;
            B[i] = static_cast<double>(rand()) / RAND_MAX;
        }
    }

    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&e, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    int rows_per_proc = m / size;
    int remainder = m % size;
    local_rows = (rank < remainder) ? rows_per_proc + 1 : rows_per_proc;
    local_A.resize(local_rows * n);
    local_B.resize(local_rows * n);

    distribute_data(m, n, rank, size, A, B, local_A, local_B);

    for(int i = 0; i < rank; ++i) {
        global_start += (i < remainder) ? rows_per_proc + 1 : rows_per_proc;
    }

    bool global_converged = false;
    while(!global_converged) {
        bool local_converged = iterate(local_A, local_B, m, n, e, global_start, local_rows);
        MPI_Allreduce(&local_converged, &global_converged, 1, MPI_C_BOOL, MPI_LAND, MPI_COMM_WORLD);
    }

    vector<double> result;
    if(rank == 0) {
        result.resize(m * n);
    }

    int* recvcounts = new int[size];
    int* displs = new int[size];
    int offset = 0;
    
    for(int i = 0; i < size; ++i) {
        recvcounts[i] = (i < remainder) ? (rows_per_proc + 1) * n : rows_per_proc * n;
        displs[i] = offset;
        offset += recvcounts[i];
    }

    MPI_Gatherv(local_A.data(), local_rows * n, MPI_DOUBLE,
               result.data(), recvcounts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if(rank == 0) {
        cout << "Convergence achieved!" << endl;
    }

    delete[] recvcounts;
    delete[] displs;

    MPI_Finalize();
    return 0;
}