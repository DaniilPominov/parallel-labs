#include <iostream>
#include <fstream>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <algorithm>
// Структура для хранения информации о рекламном ролике
struct Ad {
    int id;          // ID клиента
    int duration;    // Длительность ролика в секундах
    int max_shows;   // Максимальное количество показов
    int price;       // Цена за один показ
};

// Глобальные переменные
int total_time = 864;         // Общее время на эфире в секундах 86400 = 24 часа
int profit = 0;                 // Общая прибыль
std::string filename = "ads.txt"; // Файл для хранения рекламных предложений
pthread_cond_t cond_non_empty = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;

// Функция для генерации случайного рекламного предложения
Ad generate_ad(int id) {
    return {id, rand() % 60 + 1, rand() % 10 + 1, rand() % 100 + 50};
}

// Поток "писатель"
void* writer(void* arg) {
    srand(time(NULL));
    int id = 1;
    while (total_time>10) {
        // Запись в файл
        pthread_mutex_lock(&cond_mutex); // Ожидание пока не прочитают
        std::ofstream file(filename, std::ios::trunc);
        if (file.is_open()) {
            for(int i = 0; i < random()%10+1; i++) {
            Ad ad = generate_ad(id++);
            file << ad.id << " " << ad.duration << " " << ad.max_shows << " " << ad.price << "\n";
            std::cout << "Писатель: Добавлено предложение от клиента " << ad.id << std::endl;
            }
            file.close();
        } else {
            std::cerr << "Ошибка открытия файла для записи!" << std::endl;
        }

        pthread_mutex_unlock(&cond_mutex);
        pthread_cond_signal(&cond_non_empty);
    }
    return nullptr;
}

// Поток "читатель"
void* reader(void* arg) {
    while (total_time>10) {
        pthread_mutex_lock(&cond_mutex); // Ожидание пока будет что прочитать
        pthread_cond_wait(&cond_non_empty, &cond_mutex);
        
        std::ifstream file(filename);
        std::vector<Ad> new_ads;
        if (file.is_open()) {
            int id, duration, max_shows, price;
            while (file >> id >> duration >> max_shows >> price) {
                new_ads.push_back({id, duration, max_shows, price});
            }
            file.close();

        } else {
            std::cerr << "Ошибка открытия файла для чтения!" << std::endl;
        }
        pthread_mutex_unlock(&cond_mutex); // Разблокировка
        
        // Обработка новых предложений
        
        // Сортировка по price * max_shows / duration = monet per second (от большего к меньшему)
        std::sort(new_ads.begin(), new_ads.end(), [](const Ad& a, const Ad& b) {
        return (a.price * a.max_shows)/a.duration > (b.price * b.max_shows)/b.duration;
        });

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

    }
    return nullptr;
}

int main() {
    // Создание потоков
    pthread_t writer_thread, reader_thread;
    pthread_cond_init(&cond_non_empty,NULL);
    pthread_create(&writer_thread, nullptr, writer, nullptr);
    pthread_create(&reader_thread, nullptr, reader, nullptr);

    pthread_join(writer_thread, nullptr);
    pthread_join(reader_thread, nullptr);
    std::cout << "Общая прибыль: " << profit << " денег." << std::endl;
    // Уничтожение блокировки
    pthread_mutex_destroy(&cond_mutex);
    pthread_cond_destroy(&cond_non_empty);

    return 0;
}