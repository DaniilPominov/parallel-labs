#include <iostream>
#include <vector>
#include <cmath>
#include <pthread.h>

struct ThreadData {
    int id;               
    int m;                
    int n;                
    std::vector<std::vector<double>> &A; 
    std::vector<std::vector<double>> &B; 
    double e;            
	ThreadData(std::vector<std::vector<double>> &AA, std::vector<std::vector<double>> &BB) : A(AA), B(BB) {

	}
};

void* iterate_matrix(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int id = data->id;
    int m = data->m;
    int n = data->n;
    double e = data->e;
    //диапазон 
    int start = id * m / data->m;
    int end = (id + 1) * m / data->m;


    for (int i = start; i < end; ++i) {
        for (int j = 0; j < n; ++j) {
            double new_value;
            if (i < m - 1) {
                new_value = (data->A[i][j] + data->A[i + 1][j] + data->B[i][j]) / 3.0;
            } else {
                new_value = (data->A[i][j] + data->B[i][j]) / 2.0;
            }

            if (std::abs(new_value - data->B[i][j]) >= e) {
            }

            data->A[i][j] = new_value;
        }
    }


    pthread_exit(nullptr);
}

int main() {
    srand(time(NULL));
    int m = 4; 
    int n = 4; 
    double e = 0.01; 

    std::vector<std::vector<double>> A(m, std::vector<double>(n, 0.0));
    std::vector<std::vector<double>> B(m, std::vector<double>(n, 0.0));

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            A[i][j] = static_cast<double>(random()%100);
            B[i][j] = static_cast<double>(random()%100);
        }
    }

    std::cout << "Initial matrix A:" << std::endl;
    for (const auto& row : A) {
        for (double val : row) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "Initial matrix B:" << std::endl;
    for (const auto& row : B) {
        for (double val : row) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }

    const int num_threads = 4;
    pthread_t threads[num_threads];
    std::vector<ThreadData*> thread_data;

	for(int i = 0; i<num_threads; i++){
		auto tdata = new ThreadData(A,B);
		thread_data.push_back(tdata);
	}
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, nullptr);

    bool done = false;

        for (int i = 0; i < num_threads; i++) {
            thread_data[i]->id = i;
            thread_data[i]->m = m;
            thread_data[i]->n = n;
            thread_data[i]->e = e;
            pthread_create(&threads[i], nullptr, iterate_matrix, thread_data[i]);
        }

        // Ожидаем завершения всех потоков
        for (int i = 0; i < num_threads; ++i) {
            pthread_join(threads[i], nullptr);
        }

    // Уничтожаем мьютекс

    // Вывод итоговой матрицы A
    std::cout << "Final matrix A:" << std::endl;
    for (const auto& row : A) {
        for (double val : row) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
