#include <mpi.h>       // Библиотека для работы с MPI
#include <iostream>    // Потоковый ввод/вывод
#include <vector>      // Динамические массивы (вектора)
#include <string>      // Работа со строками
#include <limits>      // Для получения минимального/максимального значения чисел
#include <cstring>     // Для работы со строками C-типа (strncpy)

// Структура для хранения данных о сотруднике
struct Employee {
    std::string name;  // Имя сотрудника
    int result;        // Результат (например, количество заготовок)
};

// Структура для отделения
struct Department {
    std::string name;              // Название отделения
    std::vector<Employee> employees; // Список сотрудников отделения
};

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);  // Инициализация MPI

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);  // Получение номера текущего процесса
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);  // Получение общего количества процессов

    // Только процесс с рангом 0 инициализирует данные
    std::vector<Department> departments;
    if (world_rank == 0) {
        departments = {
            {"Отделение 1", { {"Иван", 120}, {"Петр", 150}, {"Сергей", 130} }},
            {"Отделение 2", { {"Андрей", 200}, {"Михаил", 180}, {"Владимир", 210} }},
            {"Отделение 3", { {"Олег", 170}, {"Дмитрий", 160}, {"Евгений", 180} }}
        };
    }

    Department myDepartment;  // Локальное отделение для каждого процесса

    if (world_rank < 3) {  // Только 3 процесса участвуют (по количеству отделений)
        if (world_rank == 0) {
            // Процесс 0 отправляет другим процессам данные об их отделениях
            for (int i = 1; i < 3; ++i) {
                for (const auto& emp : departments[i].employees) {
                    int len = emp.name.length();  // Длина имени
                    MPI_Send(&len, 1, MPI_INT, i, 0, MPI_COMM_WORLD);  // Отправка длины имени
                    MPI_Send(emp.name.c_str(), len, MPI_CHAR, i, 0, MPI_COMM_WORLD);  // Отправка имени
                    MPI_Send(&emp.result, 1, MPI_INT, i, 0, MPI_COMM_WORLD);  // Отправка результата
                }
            }
            myDepartment = departments[0];  // Процесс 0 работает со своим отделением
        } else {
            // Остальные процессы получают данные о своём отделении от процесса 0
            Department dept;
            dept.name = "Отделение " + std::to_string(world_rank + 1);  // Название отделения

            for (int j = 0; j < 3; ++j) {  // Получаем 3 сотрудников
                int len;
                MPI_Recv(&len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  // Получение длины имени
                char buffer[100];
                MPI_Recv(buffer, len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  // Получение имени
                buffer[len] = '\0';  // Завершаем строку
                int result;
                MPI_Recv(&result, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  // Получение результата
                dept.employees.push_back({std::string(buffer), result});  // Добавляем сотрудника
            }
            myDepartment = dept;  // Сохраняем своё отделение
        }

        // Поиск лучшего сотрудника в своём отделении
        Employee bestEmp = {"", std::numeric_limits<int>::min()};
        for (const auto& emp : myDepartment.employees) {
            if (emp.result > bestEmp.result) {
                bestEmp = emp;  // Обновляем, если найден лучший
            }
        }

        // Вывод лучшего сотрудника своего отделения
        std::cout << "Лучший в " << myDepartment.name << ": " << bestEmp.name
                  << " с результатом " << bestEmp.result
                  << " (Процесс " << world_rank << ")\n";

        // Структура для отправки данных на процесс 0
        struct {
            int result;       // Результат (используется для сравнения)
            char name[50];    // Имя сотрудника
        } local_best, global_best;
        // Заполняем данные локального лучшего
        local_best.result = bestEmp.result;
        strncpy(local_best.name, bestEmp.name.c_str(), sizeof(local_best.name) - 1);
        local_best.name[sizeof(local_best.name) - 1] = '\0';  // Гарантируем нуль-терминатор

        // Собираем лучших сотрудников на процесс 0 с выбором максимального результата
        MPI_Reduce(&local_best, &global_best, 1, MPI_2INT, MPI_MAXLOC, 0, MPI_COMM_WORLD);

        // Процесс 0 выводит абсолютного победителя
        if (world_rank == 0) {
            std::cout << "\nАбсолютный победитель: " << global_best.name
                      << " с результатом " << global_best.result << "\n";
        }
    }

    MPI_Finalize();  // Завершение работы MPI
    return 0;
}