#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mpi.h>
#include <ctime>
#include <array>

constexpr int MAX_DISHES = 10;
constexpr int MAX_DISH_NAME = 50;
constexpr int NUM_PROCESSES = 3;

struct Dish {
    char name[MAX_DISH_NAME];
    int price;
};

const std::array<Dish, 10> dish_database = {{
    {"Пицца", 350}, {"Суп", 200}, {"Салат", 150}, 
    {"Стейк", 500}, {"Рыба", 400}, {"Паста", 300}, 
    {"Десерт", 250}, {"Напиток", 100}, {"Закуска", 180}, 
    {"Бургер", 320}
}};

MPI_Datatype MPI_Dish_type;

void create_dish_type() {
    int blocklengths[2] = {MAX_DISH_NAME, 1};
    MPI_Datatype types[2] = {MPI_CHAR, MPI_INT};
    MPI_Aint offsets[2];
    
    offsets[0] = offsetof(Dish, name);
    offsets[1] = offsetof(Dish, price);
    
    MPI_Type_create_struct(2, blocklengths, offsets, types, &MPI_Dish_type);
    MPI_Type_commit(&MPI_Dish_type);
}

void select_random_dishes(Dish* selected, int* count) {
    *count = rand() % 3 + 1;
    for(int i = 0; i < *count; i++) 
        selected[i] = dish_database[rand() % dish_database.size()];
}

int check_common(Dish* data, int* counts, int* displs) {
    for(int i = 0; i < counts[0]; i++) {
        Dish* d0 = &data[displs[0] + i];
        int found = 1;
        
        for(int p = 1; p < NUM_PROCESSES; p++) {
            int exists = 0;
            for(int j = 0; j < counts[p]; j++) {
                Dish* dp = &data[displs[p] + j];
                if(strcmp(d0->name, dp->name) == 0) {
                    exists = 1;
                    break;
                }
            }
            found &= exists;
        }
        if(found) return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if(size != NUM_PROCESSES) {
        if(rank == 0) fprintf(stderr, "Requires exactly %d processes\n", NUM_PROCESSES);
        MPI_Finalize();
        return 1;
    }
    
    create_dish_type();
    srand(time(NULL) + rank);
    
    Dish local_dishes[MAX_DISHES];
    int local_count;
    int stop = 0;
    
    Dish* global_dishes = nullptr;
    int global_counts[NUM_PROCESSES] = {0};
    int displs[NUM_PROCESSES] = {0};
    
    while(!stop) {
        // Выбор блюд
        select_random_dishes(local_dishes, &local_count);
        
        // Сбор количества выбранных блюд
        MPI_Gather(&local_count, 1, MPI_INT, global_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        // Расчет смещений
        if(rank == 0) {
            for(int i = 1; i < NUM_PROCESSES; i++)
                displs[i] = displs[i-1] + global_counts[i-1];
            global_dishes = new Dish[displs[2] + global_counts[2]];
        }
        
        // Сбор данных о блюдах
        MPI_Gatherv(local_dishes, local_count, MPI_Dish_type, 
                   global_dishes, global_counts, displs, MPI_Dish_type, 0, MPI_COMM_WORLD);
        
        // Проверка
        int found = 0;
        if(rank == 0) {
            found = check_common(global_dishes, global_counts, displs);
            if(!found) delete[] global_dishes;
        }
        
        // Проверка сходимости
        MPI_Bcast(&found, 1, MPI_INT, 0, MPI_COMM_WORLD);
        stop = found;
    }
    
    // Вывод результатов
    if(rank == 0) {
        printf("\nОбщие блюда найдены! Результаты:\n");
        for(int p = 0; p < NUM_PROCESSES; p++) {
            printf("Процесс %d:\n", p+1);
            for(int i = 0; i < global_counts[p]; i++) {
                Dish* d = &global_dishes[displs[p] + i];
                printf("  %-10s %4d руб.\n", d->name, d->price);
            }
        }
        delete[] global_dishes;
    }
    
    MPI_Type_free(&MPI_Dish_type);
    MPI_Finalize();
    return 0;
}