#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Простейший односвязный список
class Node {
    public:
    int x, y, z;
    Node* next;
};

// Глобальные переменные списка
Node* list_head = NULL;
Node* list_bottom = NULL;
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
        new_node.next = list_head;
        //смещаем голову
        list_head = &new_node;
        int new_length = ++list_length;
        pthread_rwlock_unlock(&list_rwlock);

        if (new_length == 1) {

            list_bottom = list_head;
            pthread_cond_broadcast(&cond_non_empty);
        }
        if (new_length == 2) {
            pthread_cond_broadcast(&cond_min_two);
        }
        if (new_length == 3) {
            pthread_cond_broadcast(&cond_min_three);
        }

        sleep(1);
    }
    return NULL;
}

// Поток удаления элементов
void* remover_thread(void* arg) {
    while (1) {
        pthread_rwlock_wrlock(&list_rwlock);
        while (list_head == NULL) {
            pthread_rwlock_unlock(&list_rwlock);
            pthread_mutex_lock(&cond_mutex);
            pthread_cond_wait(&cond_non_empty, &cond_mutex);
            pthread_mutex_unlock(&cond_mutex);
            pthread_rwlock_wrlock(&list_rwlock);
        }
        Node* to_remove = list_bottom;
        list_bottom = list_bottom->next;
        free(to_remove);
        list_length--;
        pthread_rwlock_unlock(&list_rwlock);

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

        pthread_rwlock_rdlock(&list_rwlock);
        if (list_length < 2) {
            pthread_rwlock_unlock(&list_rwlock);
            continue;
        }
        int idx1 = rand() % list_length;
        int idx2 = rand() % list_length;
        while (idx2 == idx1) idx2 = rand() % list_length;

        Node *node1 = list_head, *node2 = list_head;
        for (int i = 0; i < idx1; i++) node1 = node1->next;
        for (int i = 0; i < idx2; i++) node2 = node2->next;

        int product = node1->x * node2->x + node1->y * node2->y + node1->z * node2->z;
        printf("Scalar product: %d\n", product);
        pthread_rwlock_unlock(&list_rwlock);

        sleep(1);
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
        if (list_length < 2) {
            pthread_rwlock_unlock(&list_rwlock);
            continue;
        }
        int idx1 = rand() % list_length;
        int idx2 = rand() % list_length;
        while (idx2 == idx1) idx2 = rand() % list_length;

        Node *node1 = list_head, *node2 = list_head;
        for (int i = 0; i < idx1; i++) node1 = node1->next;
        for (int i = 0; i < idx2; i++) node2 = node2->next;

        int cross_x = node1->y * node2->z - node1->z * node2->y;
        int cross_y = node1->z * node2->x - node1->x * node2->z;
        int cross_z = node1->x * node2->y - node1->y * node2->x;
        printf("Vector product: (%d, %d, %d)\n", cross_x, cross_y, cross_z);
        pthread_rwlock_unlock(&list_rwlock);

        sleep(1);
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
        Node *curr = list_head;
        while (curr) {
            Node *next = curr->next;
            while (next) {
                int cross_x = curr->y * next->z - curr->z * next->y;
                int cross_y = curr->z * next->x - curr->x * next->z;
                int cross_z = curr->x * next->y - curr->y * next->x;
                if (cross_x == 0 && cross_y == 0 && cross_z == 0) {
                    printf("Collinear pair found: (%d,%d,%d) and (%d,%d,%d)\n",
                           curr->x, curr->y, curr->z, next->x, next->y, next->z);
                }
                next = next->next;
            }
            curr = curr->next;
        }
        pthread_rwlock_unlock(&list_rwlock);

        sleep(1);
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
        Node *a = list_head;
        while (a) {
            Node *b = a->next;
            while (b) {
                Node *c = b->next;
                while (c) {
                    int cross_x = b->y * c->z - b->z * c->y;
                    int cross_y = b->z * c->x - b->x * c->z;
                    int cross_z = b->x * c->y - b->y * c->x;
                    int dot = a->x * cross_x + a->y * cross_y + a->z * cross_z;
                    if (dot == 0) {
                        printf("Coplanar triple found: (%d,%d,%d), (%d,%d,%d), (%d,%d,%d)\n",
                               a->x, a->y, a->z, b->x, b->y, b->z, c->x, c->y, c->z);
                    }
                    c = c->next;
                }
                b = b->next;
            }
            a = a->next;
        }
        pthread_rwlock_unlock(&list_rwlock);

        sleep(1);
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