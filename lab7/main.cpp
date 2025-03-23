#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <forward_list>
#include <iostream>
// Простейший односвязный список
class Node {
    public:
    int x, y, z;
};

// Глобальные переменные списка
std::forward_list<Node> nodes;
int list_length = 0;
// Синхронизация
pthread_rwlock_t list_rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_non_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_min_two = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_min_three = PTHREAD_COND_INITIALIZER;

// Поток добавления элементов
void* adder_thread(void* arg) {
    while (1) {
        Node new_node;

        new_node.x = rand() % 100;
        new_node.y = rand() % 100;
        new_node.z = rand() % 100;

        pthread_rwlock_wrlock(&list_rwlock);

        nodes.push_front(new_node);
        list_length++;
        pthread_rwlock_unlock(&list_rwlock);

        if (list_length >= 1) {

            pthread_cond_broadcast(&cond_non_empty);
        }
        if (list_length >= 2) {
            pthread_cond_broadcast(&cond_min_two);
        }
        if (list_length >= 3) {
            pthread_cond_broadcast(&cond_min_three);
        }
        std::cout<<"created new node\n";

        //sleep(1);
    }
    return NULL;
}

// Поток удаления элементов
void* remover_thread(void* arg) {
    while (1) {
        if(!nodes.empty()){
            auto current = nodes.begin();
            auto end = nodes.end();
            int sub_iter = 0;
            while(sub_iter<list_length-3){
                sub_iter++;
                if(current!=end)
                current++;
            }
            pthread_rwlock_wrlock(&list_rwlock);
            if(current!=end)
            nodes.erase_after(current,end);
            list_length--;
            std::cout<<"remowed nodes\n";
            pthread_rwlock_unlock(&list_rwlock);
        }     
        
        sleep(1);
    }
    return NULL;
}

// Поток вычисления скалярного произведения
void* scalar_product_thread(void* arg) {
    while (1) {

        pthread_mutex_lock(&cond_mutex);
        while (list_length < 2) {
            pthread_cond_wait(&cond_min_two, &cond_mutex);
        }
        pthread_mutex_unlock(&cond_mutex);
        int idx1 = rand() % list_length;
        int idx2 = rand() % list_length;
        while (idx2 == idx1) idx2 = rand() % list_length;
        auto current = nodes.begin();
        auto end = nodes.end();
        int i = 0;
        Node node1, node2;

        while(current!=end){
            if(i==idx1){
                node1 = *current;
            }
            else if(i==idx2){
                node2 = *current;
            }
            current++;
            i++;
        }

        double product = node1.x * node2.x + node1.y * node2.y + node1.z * node2.z;
        printf("Scalar product: %f\n", product);
        
        pthread_rwlock_unlock(&list_rwlock);

        //sleep(1);
    }
    return NULL;
}

// Поток вычисления векторного произведения
void* vector_product_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&cond_mutex);
        while (list_length < 2) {
            pthread_cond_wait(&cond_min_two, &cond_mutex);
        }
        pthread_mutex_unlock(&cond_mutex);
        pthread_rwlock_rdlock(&list_rwlock);

        int idx1 = rand() % list_length;
        int idx2 = rand() % list_length;
        while (idx2 == idx1) idx2 = rand() % list_length;
        auto current = nodes.begin();
        auto end = nodes.end();
        int i = 0;
        Node node1, node2;
        
        while(current!=end){
            if(i==idx1){
                node1 = *current;
            }
            else if(i==idx2){
                node2 = *current;
            }
            current++;
            i++;
        }

        double cross_x = node1.y * node2.z - node1.z * node2.y;
        double cross_y = node1.z * node2.x - node1.x * node2.z;
        double cross_z = node1.x * node2.y - node1.y * node2.x;
        printf("Vector product: (%d, %d, %d)\n", cross_x, cross_y, cross_z);
        pthread_rwlock_unlock(&list_rwlock);
        //sleep(1);
    }
    return NULL;
}

// Поток поиска компланарных пар (коллинеарных)
void* coplanar_pairs_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&cond_mutex);
        while (list_length < 2) {
            pthread_cond_wait(&cond_min_two, &cond_mutex);
        }
        pthread_mutex_unlock(&cond_mutex);
        pthread_rwlock_rdlock(&list_rwlock);
        auto curr = nodes.begin();
        auto end = nodes.end();
        bool endFlag=false;
        while (curr!=end && !endFlag) {
            Node node1 = *curr;
            auto curr2 = curr;
            while (curr2!=end && !endFlag) {
                Node node2 = *curr2;
                double cross_x = node1.y * node2.z - node1.z * node2.y;
                double cross_y = node1.z * node2.x - node1.x * node2.z;
                double cross_z = node1.x * node2.y - node1.y * node2.x;
                if (cross_x == 0.0 && cross_y == 0.0 && cross_z == 0.0) {
                    printf("Collinear pair found: (%d,%d,%d) and (%d,%d,%d)\n",
                        node1.x, node1.y, node1.z, node2.x, node2.y, node2.z);
                        endFlag = true;
                        break;
                }
                curr2++;
            }
            curr++;
        }
        pthread_rwlock_unlock(&list_rwlock);

        //sleep(1);
    }
    return NULL;
}

// Поток поиска компланарных троек
void* coplanar_triples_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&cond_mutex);
        while (list_length < 3) {
            pthread_cond_wait(&cond_min_three, &cond_mutex);
        }
        pthread_mutex_unlock(&cond_mutex);

        pthread_rwlock_rdlock(&list_rwlock);
        auto curr = nodes.begin();
        auto end = nodes.end();
        bool endFlag=false;
        while (curr!=end && !endFlag) {
            Node a = *curr;
            auto curr2 = curr;
            while (curr2!=end && !endFlag) {
                Node b = *curr2;
                auto curr3 = curr;
                while (curr3!=end && !endFlag) {
                    Node c = *curr3;
                    double cross_x = b.y * c.z - b.z * c.y;
                    double cross_y = b.z * c.x - b.x * c.z;
                    double cross_z = b.x * c.y - b.y * c.x;
                    double dot = a.x * cross_x + a.y * cross_y + a.z * cross_z;
                    if (dot == 0.0) {
                        printf("Coplanar triple found: (%d,%d,%d), (%d,%d,%d), (%d,%d,%d)\n",
                               a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z);
                    }
                    curr3++;
                }
                curr2++;
            }
            curr++;
        }
        pthread_rwlock_unlock(&list_rwlock);

        //sleep(1);
    }
    return NULL;
}

int main() {
    srand(time(NULL));

    pthread_t adder, remover, scalar, vector, coplanar_pairs, coplanar_triples;

    pthread_create(&adder, NULL, adder_thread, NULL);
    pthread_create(&remover, NULL, remover_thread, NULL);
    pthread_create(&scalar, NULL, scalar_product_thread, NULL);
    pthread_create(&vector, NULL, vector_product_thread, NULL);
    pthread_create(&coplanar_pairs, NULL, coplanar_pairs_thread, NULL);
    pthread_create(&coplanar_triples, NULL, coplanar_triples_thread, NULL);

    pthread_join(adder, NULL);
    pthread_join(remover, NULL);
    pthread_join(scalar, NULL);
    pthread_join(vector, NULL);
    pthread_join(coplanar_pairs, NULL);
    pthread_join(coplanar_triples, NULL);

    pthread_rwlock_destroy(&list_rwlock);
    pthread_mutex_destroy(&cond_mutex);
    pthread_cond_destroy(&cond_non_empty);
    pthread_cond_destroy(&cond_min_two);
    pthread_cond_destroy(&cond_min_three);

    return 0;
}