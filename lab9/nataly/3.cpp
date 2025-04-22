#include <mpi.h>
#include <iostream>
#include <set>
#include <vector>
#include <cstdlib>
#include <ctime>

#define NUM_COUNT 10  // Количество уникальных чисел в каждом множестве

// Функция генерации множества уникальных случайных чисел
std::set<int> generateSet() {
    std::set<int> s;
    while (s.size() < NUM_COUNT) {
        s.insert(rand() % 100); // Генерация случайного числа от 0 до 99
    }
    return s;
}

// Функция вычисления разности двух множеств
std::set<int> difference(const std::set<int>& s1, const std::set<int>& s2) {
    std::set<int> result;
    for (int num : s1) {
        if (s2.find(num) == s2.end()) { // Если элемент отсутствует во втором множестве, добавляем в результат
            result.insert(num);
        }
    }
    return result;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv); // Инициализация MPI
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Получение ранга текущего процесса
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Получение общего числа процессов

    // Проверка, достаточно ли процессов для работы
    if (size < 3) {
        if (rank == 0) {
            std::cerr << "Ошибка: необходимо минимум 3 процесса." << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    srand(time(0) + rank); // Инициализация генератора случайных чисел с учетом ранга процесса
    std::set<int> localSet = generateSet(); // Генерация множества для данного процесса
    std::vector<int> buffer(localSet.begin(), localSet.end()); // Преобразование множества в вектор для передачи
    int recvBuffer[NUM_COUNT]; // Буфер для приема данных

    // Передача данных между процессами
    if (rank == 0) {
        MPI_Send(buffer.data(), NUM_COUNT, MPI_INT, 1, 0, MPI_COMM_WORLD); // Отправка множества процессу 1
        MPI_Send(buffer.data(), NUM_COUNT, MPI_INT, 2, 0, MPI_COMM_WORLD); // Отправка множества процессу 2
    } else {
        MPI_Recv(recvBuffer, NUM_COUNT, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Прием множества от процесса 0
        std::set<int> receivedSet(recvBuffer, recvBuffer + NUM_COUNT); // Преобразование полученного массива в множество
        std::set<int> diff = difference(localSet, receivedSet); // Вычисление разности множеств
        
        // Вывод результата разности множеств для данного процесса
        std::cout << "Процесс " << rank << " разность множеств: " << diff.size() << " элементов." << std::endl;
    }

    MPI_Finalize(); // Завершение работы MPI
    return 0;
}