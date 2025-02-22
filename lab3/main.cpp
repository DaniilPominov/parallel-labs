#include <iostream>
#include <vector>
#include <pthread.h>
#include <chrono>
#include <queue>
#include <semaphore.h> // Для POSIX-семафоров

struct ThreadData {
    int id;               
    int m;                
    int n;       
    int t_count;          
    std::vector<std::vector<double>> &A; 
    std::vector<std::vector<double>> &B; 
    double e;            
    ThreadData(std::vector<std::vector<double>> &AA, std::vector<std::vector<double>> &BB) : A(AA), B(BB) {}
};

// Структура для хранения промежуточных результатов
struct Result {
    int thread_id; // Идентификатор потока
    int iteration; // Номер итерации
    double value;  // Значение
};

// Общая очередь для хранения промежуточных результатов
std::queue<Result> result_queue;

// Двоичный семафор для защиты доступа к очереди
sem_t queue_semaphore;

void* iterate_matrix(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int id = data->id;
    int m = data->m;
    int n = data->n;
    double e = data->e;
    int iteration = 0; // Счетчик итераций

    // Диапазон работы потока
    int start = id * m / data->t_count;
    int end = (id + 1) * m / data->t_count;

    bool done = false;
    while (!done) {
        iteration++;
        for (int i = start; i < end; i++) {
            for (int j = 0; j < n; j++) {
                double new_value;
                if (i < m - 1) {
                    //new_value = (data->A[i][j] + data->A[i + 1][j] + data->B[i][j]) / 3.0;
                    new_value = (data->A[i][j] + data->B[i][j] + data->B[i][j]) / 3.0;
                } else {
                    new_value = (data->A[i][j] + data->B[i][j]) / 2.0;
                }
                data->A[i][j] = new_value;

                // Запись промежуточного результата в очередь
                Result res;
                res.thread_id = id;
                res.iteration = iteration;
                res.value = new_value;

                // Захват семафора для защиты очереди
                sem_wait(&queue_semaphore);
                result_queue.push(res);
                sem_post(&queue_semaphore);
            }
        }

        // Проверка условия завершения
        done = true;
        for (int i = start; i < end; i++) {
            for (int j = 0; j < n; j++) {
                double error = std::abs(data->A[i][j] - data->B[i][j]);
                if (error > e) {
                    done = false;
                    break;
                }
            }
            if (!done) break;
        }
    }

    return NULL;
}

int main() {
    srand(time(NULL));
    const int num_threads = 2;
    int m = 4; 
    int n = 4; 
    double e = 5; 

    std::vector<std::vector<double>> A(m, std::vector<double>(n, 0.0));
    std::vector<std::vector<double>> B(m, std::vector<double>(n, 0.0));

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            A[i][j] = static_cast<double>(random()%1000);
            B[i][j] = static_cast<double>(random()%1000);
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

    // Инициализация семафора
    sem_init(&queue_semaphore, 0, 1); // Инициализация семафора значением 1

    auto start = std::chrono::high_resolution_clock::now();
    pthread_t threads[num_threads];
    std::vector<ThreadData*> thread_data;

    for (int i = 0; i < num_threads; i++) {
        auto tdata = new ThreadData(A, B);
        thread_data.push_back(tdata);
    }

    for (int i = 0; i < num_threads; i++) {
        thread_data[i]->id = i;
        thread_data[i]->m = m;
        thread_data[i]->n = n;
        thread_data[i]->e = e;
        thread_data[i]->t_count = num_threads;
        pthread_create(&threads[i], nullptr, iterate_matrix, thread_data[i]);
    }

    // Ожидание завершения всех потоков
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Вывод общей очереди
    std::cout << "Общая очередь промежуточных результатов:" << std::endl;
    while (!result_queue.empty()) {
        Result res = result_queue.front();
        result_queue.pop();
        std::cout << "Поток " << res.thread_id << ", Итерация " << res.iteration << ", Значение: " << res.value << std::endl;
    }

    // Уничтожение семафора
    sem_destroy(&queue_semaphore);

    std::cout << "Время выполнения: " << duration.count() << " ms." << std::endl;

    // Освобождение памяти
    for (auto tdata : thread_data) {
        delete tdata;
    }

    return 0;
}