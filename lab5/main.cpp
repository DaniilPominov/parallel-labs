#include <iostream>
#include <fstream>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <algorithm>

// Структура для хранения информации о рекламном ролике
struct Ad {
    int id;          // ID клиента
    int duration;    // Длительность ролика в секундах
    int max_shows;   // Максимальное количество показов
    int price;       // Цена за один показ
};

// Глобальные переменные
std::vector<Ad> ads;            // Список всех рекламных роликов
int total_time = 600;//86400;         // Общее время на эфире в секундах (24 часа)
int profit = 0;                 // Общая прибыль
pthread_rwlock_t rwlock;        // Блокировка чтения-записи
std::string filename = "ads.txt"; // Файл для хранения рекламных предложений

// Функция для генерации случайного рекламного предложения
Ad generate_ad(int id) {
    return {id, rand() % 60 + 1, rand() % 10 + 1, rand() % 100 + 50};
}

// Поток "писатель"
void* writer(void* arg) {
    int id = 1;
    while (total_time>10) {
        Ad ad = generate_ad(id++);

        // Запись в файл
        pthread_rwlock_wrlock(&rwlock); // Блокировка для записи
        std::ofstream file(filename, std::ios::app);
        if (file.is_open()) {
            file << ad.id << " " << ad.duration << " " << ad.max_shows << " " << ad.price << "\n";
            file.close();
            std::cout << "Писатель: Добавлено предложение от клиента " << ad.id << std::endl;
        } else {
            std::cerr << "Ошибка открытия файла для записи!" << std::endl;
        }
        pthread_rwlock_unlock(&rwlock); // Разблокировка

        sleep(2); // Имитация задержки
    }
    return nullptr;
}

// Поток "читатель"
void* reader(void* arg) {
    while (total_time>10) {
        pthread_rwlock_wrlock(&rwlock); // Блокировка для чтения и записи
        std::ifstream file(filename);
        std::vector<Ad> new_ads;
        if (file.is_open()) {
            int id, duration, max_shows, price;
            while (file >> id >> duration >> max_shows >> price) {
                new_ads.push_back({id, duration, max_shows, price});
            }
            file.close();

            // Очистка файла после чтения
            std::ofstream clear_file(filename, std::ios::trunc);
            clear_file.close();
        } else {
            std::cerr << "Ошибка открытия файла для чтения!" << std::endl;
        }
        pthread_rwlock_unlock(&rwlock); // Разблокировка

        // Обработка новых предложений
        for (const auto& ad : new_ads) {
            if (total_time >= ad.duration * ad.max_shows) {
                total_time -= ad.duration * ad.max_shows;
                profit += ad.price * ad.max_shows;
                std::cout << "Читатель: Ролик клиента " << ad.id << " размещен. Прибыль: " << ad.price * ad.max_shows << std::endl;
            } else if (total_time >= ad.duration) {
                int possible_shows = total_time / ad.duration;
                total_time -= ad.duration * possible_shows;
                profit += ad.price * possible_shows;
                std::cout << "Читатель: Ролик клиента " << ad.id << " размещен частично. Показов: " << possible_shows << ", Прибыль: " << ad.price * possible_shows << std::endl;
            } else {
                std::cout << "Читатель: Ролик клиента " << ad.id << " не может быть размещен из-за нехватки времени." << std::endl;
            }
            std::cout << "Осталось времени " << total_time << "сек. " << std::endl;
        }

        sleep(3); // Имитация задержки
    }
    return nullptr;
}

int main() {
    // Инициализация блокировки чтения-записи
    pthread_rwlock_init(&rwlock, nullptr);

    // Создание потоков
    pthread_t writer_thread, reader_thread;
    pthread_create(&writer_thread, nullptr, writer, nullptr);
    pthread_create(&reader_thread, nullptr, reader, nullptr);

    pthread_join(writer_thread, nullptr);
    pthread_join(reader_thread, nullptr);

    // Уничтожение блокировки
    pthread_rwlock_destroy(&rwlock);

    return 0;
}