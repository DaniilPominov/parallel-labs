#include <iostream>
#include <cstring>
#include <mpi.h>

using namespace std;

bool is_vowel(char c) {
    c = tolower(c);
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');
}

int count_vowels(const char* text, int len) {
    int count = 0;
    for (int i = 0; i < len; ++i) {
        if (is_vowel(text[i])) {
            ++count;
        }
    }
    return count;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    char* text = nullptr;
    int length = 0;

    if (rank == 0) {
        string str;
        cout << "Type an english string:" << endl;
        getline(cin, str);
        length = str.size();
        text = new char[length + 1];
        strcpy(text, str.c_str());
    }

    MPI_Bcast(&length, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int chunk_size = length / size;
    int remainder = length % size;
    int start, end;

    if (rank < remainder) {
        start = rank * (chunk_size + 1);
        end = start + chunk_size + 1;
    } else {
        start = rank * chunk_size + remainder;
        end = start + chunk_size;
    }

    char* local_text = new char[end - start + 1];
    
    if (rank == 0) {
        for (int i = 1; i < size; ++i) {
            int i_start, i_end;
            if (i < remainder) {
                i_start = i * (chunk_size + 1);
                i_end = i_start + chunk_size + 1;
            } else {
                i_start = i * chunk_size + remainder;
                i_end = i_start + chunk_size;
            }
            //root sends data to other processes 
            MPI_Send(text + i_start, i_end - i_start, MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }
        memcpy(local_text, text + start, end - start);
        delete[] text;
    } else {
        MPI_Recv(local_text, end - start, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    local_text[end - start] = '\0';
    std::cout<<local_text<<rank<<'\n';
    int local_count = count_vowels(local_text, end - start);
    delete[] local_text;

    int total_count;
    //*total_count = sum local_counts => root
    MPI_Reduce(&local_count, &total_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        cout << "Total vowels: " << total_count << endl;
    }

    MPI_Finalize();
    return 0;
}