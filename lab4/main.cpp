#include <iostream>
#include <vector>
#include <pthread.h>
#include <chrono>
#include <queue>
#include <semaphore.h>

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
    int thread_id;
    int iteration; 
    double value;  
};

// Общая очередь для хранения промежуточных результатов
std::queue<Result> result_queue;

// Мьютекс для защиты доступа к очереди
pthread_mutex_t queue_mutex;

// Счетный семафор для ограничения количества одновременно работающих потоков
sem_t thread_semaphore;

void* iterate_matrix(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int id = data->id;
    int m = data->m;
    int n = data->n;
    double e = data->e;
    int iteration = 0;

    // Диапазон работы потока
    int start = id * m / data->t_count;
    int end = (id + 1) * m / data->t_count;

    bool done = false;
    while (!done) {
        iteration++;
        std::vector<Result> local_results; // Локальная коллекция для накопления результатов

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

                // Добавление результата в локальную коллекцию
                Result res;
                res.thread_id = id;
                res.iteration = iteration;
                res.value = new_value;
                local_results.push_back(res);
            }
        }

        // Запись всей локальной коллекции в общую очередь
        pthread_mutex_lock(&queue_mutex);
        for (const auto& res : local_results) {
            result_queue.push(res);
        }
        pthread_mutex_unlock(&queue_mutex);

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

    // Освобождение семафора после завершения работы потока
    sem_post(&thread_semaphore);

    return NULL;
}

int main() {
    srand(time(NULL));
    const int num_threads = 6;
    int m = 400; 
    int n = 400; 
    double e = 5;
    bool print_console = false; 
    int K = 3; // Максимальное количество одновременно работающих потоков

    std::vector<std::vector<double>> A(m, std::vector<double>(n, 0.0));
    std::vector<std::vector<double>> B(m, std::vector<double>(n, 0.0));

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            A[i][j] = static_cast<double>(random()%1000);
            B[i][j] = static_cast<double>(random()%1000);
        }
    }
    if(print_console){
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
    }

    // Инициализация мьютекса
    pthread_mutex_init(&queue_mutex, nullptr);

    // Инициализация счетного семафора
    sem_init(&thread_semaphore, 0, K);
    // Таймер выполнения
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

        // Ожидание, пока семафор не позволит создать новый поток
        sem_wait(&thread_semaphore);

        // Создание потока
        pthread_create(&threads[i], nullptr, iterate_matrix, thread_data[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }
    // Конец таймера
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Вывод общей очереди
    std::cout << "Общая очередь промежуточных результатов:" << std::endl;
    pthread_mutex_lock(&queue_mutex);
    while (!result_queue.empty()) {
        Result res = result_queue.front();
        result_queue.pop();
        if(print_console)
        std::cout << "Поток " << res.thread_id << ", Итерация " << res.iteration << ", Значение: " << res.value << std::endl;
    }
    pthread_mutex_unlock(&queue_mutex);

    // Уничтожение
    pthread_mutex_destroy(&queue_mutex);
    sem_destroy(&thread_semaphore);

    std::cout << "Время выполнения: " << duration.count() << " ms." << std::endl;

    for (auto tdata : thread_data) {
        delete tdata;
    }

    return 0;
}