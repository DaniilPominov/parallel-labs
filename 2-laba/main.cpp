#include <iostream>
#include <vector>
#include <cmath>
#include <pthread.h>

// Структура для передачи данных в поток
struct ThreadData {
    int id;               // Идентификатор потока
    int m;                // Количество строк в матрице
    int n;                // Количество столбцов в матрице
    std::vector<std::vector<double>> &A; // Ссылка на матрицу A
    std::vector<std::vector<double>> &B; // Ссылка на матрицу B
    double e;             // Точность
    bool* done;           // Указатель на флаг завершения
    pthread_mutex_t* mutex; // Мьютекс для синхронизации
	ThreadData(std::vector<std::vector<double>> &AA, std::vector<std::vector<double>> &BB) : A(AA), B(BB) {

	}
};

// Функция, выполняемая каждым потоком
void* iterate_matrix(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int id = data->id;
    int m = data->m;
    int n = data->n;
    // std::vector<std::vector<double>> A = data->A;
    // std::vector<std::vector<double>> B = data->B;
    double e = data->e;
    bool* done = data->done;
    pthread_mutex_t* mutex = data->mutex;

    // Определяем диапазон строк для текущего потока
    int start = id * m / data->m;
    int end = (id + 1) * m / data->m;

    bool local_done = true;

    // Выполняем итерацию для своего диапазона строк
    for (int i = start; i < end; ++i) {
        for (int j = 0; j < n; ++j) {
            double new_value;
            if (i < m - 1) {
                new_value = (data->A[i][j] + data->A[i + 1][j] + data->B[i][j]) / 3.0;
            } else {
                new_value = (data->A[i][j] + data->B[i][j]) / 2.0;
            }

            // Проверяем точность
            if (std::abs(new_value - data->B[i][j]) >= e) {
                local_done = false;
            }

            data->A[i][j] = new_value;
        }
    }

    // Синхронизируем обновление флага завершения
    pthread_mutex_lock(mutex);
    *done &= local_done;
    pthread_mutex_unlock(mutex);

    pthread_exit(nullptr);
}

int main() {
    int m = 4; // Количество строк
    int n = 4; // Количество столбцов
    double e = 0.01; // Точность

    // Инициализация матриц A и B
    std::vector<std::vector<double>> A(m, std::vector<double>(n, 0.0));
    std::vector<std::vector<double>> B(m, std::vector<double>(n, 0.0));

    // Заполнение матриц начальными значениями
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            A[i][j] = static_cast<double>(random()%100);
            B[i][j] = static_cast<double>(random()%100);
        }
    }

    // Вывод начальных матриц
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

    // Количество потоков
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

    // Основной цикл итераций
        // Создаем потоки
        for (int i = 0; i < num_threads; i++) {
            thread_data[i]->id = i;
            thread_data[i]->m = m;
            thread_data[i]->n = n;
            thread_data[i]->e = e;
            thread_data[i]->done = &done;
            thread_data[i]->mutex = &mutex;
            pthread_create(&threads[i], nullptr, iterate_matrix, thread_data[i]);
        }

        // Ожидаем завершения всех потоков
        for (int i = 0; i < num_threads; ++i) {
            pthread_join(threads[i], nullptr);
        }

    // Уничтожаем мьютекс
    pthread_mutex_destroy(&mutex);

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
