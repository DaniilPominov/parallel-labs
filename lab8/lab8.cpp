#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_DISHES 10
#define MAX_DISH_NAME 50
#define NUM_THREADS 3

// Структура для хранения базы данных блюд
struct Dish{
    char name[MAX_DISH_NAME];
    int price;
};

// Структура для передачи данных в поток
struct ThreadData {
    int thread_id;
    Dish* database;
    int db_len;
    Dish* selected_dishes;
    int* selections_count;
    pthread_barrier_t* barrier;
    int* stop_flag;
};

// База данных блюд
Dish dish_database[] = {
    {"Пицца", 350},
    {"Суп", 200},
    {"Салат", 150},
    {"Стейк", 500},
    {"Рыба", 400},
    {"Паста", 300},
    {"Десерт", 250},
    {"Напиток", 100},
    {"Закуска", 180},
    {"Бургер", 320}
};
int database_len = sizeof(dish_database) / sizeof(dish_database[0]);

// Функция для выбора случайных блюд
void select_random_dishes(Dish* database, int db_len, Dish* selected, int* count) {
    *count = rand() % 3 + 1;
    for (int i = 0; i < *count; i++) {
        selected[i] = database[rand() % db_len];
    }
}
int check_common_dish(Dish* selected_dishes, int* selections_count) {
    // Проверяем все блюда первого потока
    for (int i = 0; i < selections_count[0]; i++) {
        Dish dish = selected_dishes[0 * MAX_DISHES + i];
        int found_in_second = 0;
        int found_in_third = 0;
        
        // Ищем это блюдо во втором потоке
        for (int j = 0; j < selections_count[1]; j++) {
            if (strcmp(dish.name, selected_dishes[1 * MAX_DISHES + j].name) == 0) {
                found_in_second = 1;
                break;
            }
        }
        
        // Ищем это блюдо в третьем потоке
        for (int k = 0; k < selections_count[2]; k++) {
            if (strcmp(dish.name, selected_dishes[2 * MAX_DISHES + k].name) == 0) {
                found_in_third = 1;
                break;
            }
        }
        
        // Если блюдо найдено во всех трех потоках
        if (found_in_second && found_in_third) {
            return 1;
        }
    }
    return 0;
}

void* thread_function(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    while (1) {
        pthread_barrier_wait(data->barrier);
        
        if (*(data->stop_flag)) {
            break;
        }
        
        select_random_dishes(data->database, data->db_len, 
                           data->selected_dishes + data->thread_id * MAX_DISHES, 
                           &data->selections_count[data->thread_id]);
        
        int ret = pthread_barrier_wait(data->barrier);
        
        if (ret == PTHREAD_BARRIER_SERIAL_THREAD) {
            if (check_common_dish(data->selected_dishes, data->selections_count)) {
                *(data->stop_flag) = 1;
            }
        }
        
        pthread_barrier_wait(data->barrier);
    }
    
    return NULL;
}

int main() {
    srand(time(NULL));
    
    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    
    // Выделяем память для хранения выбранных блюд
    Dish* selected_dishes = (Dish*)malloc(NUM_THREADS * MAX_DISHES * sizeof(Dish));
    int* selections_count = (int*)malloc(NUM_THREADS * sizeof(int));
    int stop_flag = 0;
    
    // Инициализируем барьер
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    
    // Создаем потоки
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].database = dish_database;
        thread_data[i].db_len = database_len;
        thread_data[i].selected_dishes = selected_dishes;
        thread_data[i].selections_count = selections_count;
        thread_data[i].barrier = &barrier;
        thread_data[i].stop_flag = &stop_flag;
        
        pthread_create(&threads[i], NULL, thread_function, &thread_data[i]);
    }
    
    // Ждем завершения потоков
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\nВыбранные блюда:\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("Поток %d:\n", i + 1);
        for (int j = 0; j < selections_count[i]; j++) {
            printf("  %s - %d руб.\n", 
                   selected_dishes[i * MAX_DISHES + j].name, 
                   selected_dishes[i * MAX_DISHES + j].price);
        }
    }
    
    // Освобождаем ресурсы
    free(selected_dishes);
    free(selections_count);
    pthread_barrier_destroy(&barrier);
    
    return 0;
}