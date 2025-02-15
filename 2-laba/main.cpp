#include <iostream>
#include <vector>
#include <pthread.h>
#include <chrono>
struct ThreadData {
    int id;               
    int m;                
    int n;       
    int t_count;          
    std::vector<std::vector<double>> &A; 
    std::vector<std::vector<double>> &B; 
    double e;            
	ThreadData(std::vector<std::vector<double>> &AA, std::vector<std::vector<double>> &BB) : A(AA), B(BB) {

	}
};

void* iterate_matrix(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int id = data->id;
    int m = data->m;
    int n = data->n;
    double e = data->e;
    //диапазон 
    int start = id * m / data->t_count;
    int end = (id + 1) *m / data->t_count;
    std::cout << start << " " << end << "\n";
    bool done = false;
while(done==false){
    for (int i = start; i < end; i++) {
        for (int j = 0; j < n; j++) {
            double new_value;
            double locA = data->A[i][j];
            double locB = data->B[i][j];
            double locB1 = 0.0;
            if (i < m - 1) {
                locB1 = data->A[i+1][j];
                // new_value = (data->A[i][j]+data->A[i][j] + data->B[i][j]) / 3.0;
                new_value = (data->A[i][j] + data->B[i][j] + data->B[i][j]) / 3.0;
                // рассмотрим предпоследнюю строку матрицы, когда последняя строка сошлась в своим значениями в матрице B
                // эту ситуацию можно записать как a_i = (a_i-1 +b +c)/3, где b,c для нас константы, это значение из матрицы B
                // и значение из последней строки преобразуемой матрицы. Такая последовательность очевидно сходится к (b+c)/2.
                // Невозможно сделать матрица А похожей на Б с любой заданной точностью, если не окажется, что (b+c)/2 уложится в эту точность.
            } else {
                new_value = (data->A[i][j] + data->B[i][j]) / 2.0;
            }
            data->A[i][j] = new_value;
        }
    }
    done = true;
    for (int i = start; i < end; i++) {
        for (int j = 0; j < n; j++) {
            double locA = data->A[i][j];
            double locB = data->B[i][j];
            double error = std::abs(data->A[i][j] - data->B[i][j]);
            if(error>e){
                done = false;
            }
        }
    }
    // if(done==true){
    //     break;
    // }
}
    return NULL;
}

int main() {
    srand(time(NULL));
    const int num_threads = 6;
    int m = 4096; 
    int n = 4096; 
    double e = 0.1; 

    std::vector<std::vector<double>> A(m, std::vector<double>(n, 0.0));
    std::vector<std::vector<double>> B(m, std::vector<double>(n, 0.0));

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            A[i][j] = static_cast<double>(random()%100);
            B[i][j] = static_cast<double>(random()%100);
        }
    }

    std::cout << "Initial matrix A:" << std::endl;
    // for (const auto& row : A) {
    //     for (double val : row) {
    //         std::cout << val << " ";
    //     }
    //     std::cout << std::endl;
    // }

    std::cout << "Initial matrix B:" << std::endl;
    // for (const auto& row : B) {
    //     for (double val : row) {
    //         std::cout << val << " ";
    //     }
    //     std::cout << std::endl;
    // }

    auto start = std::chrono::high_resolution_clock::now();
    pthread_t threads[num_threads];
    std::vector<ThreadData*> thread_data;

	for(int i = 0; i<num_threads; i++){
		auto tdata = new ThreadData(A,B);
		thread_data.push_back(tdata);
	}

        for (int i = 0; i < num_threads; i++) {
            thread_data[i]->id = i;
            thread_data[i]->m = m;
            thread_data[i]->n = n;
            thread_data[i]->e = e;
            thread_data[i]->t_count = num_threads;
            pthread_create(&threads[i], nullptr, iterate_matrix, thread_data[i]);
        }

        // Ожидаем завершения всех потоков
        for (int i = 0; i < num_threads; ++i) {
            pthread_join(threads[i], nullptr);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // Вывод итоговой матрицы A
    std::cout << "Final matrix A:" << std::endl;
    std::cout << "Время выполнения: " << duration.count() << " ms." << std::endl;
    // for (const auto& row : A) {
    //     for (double val : row) {
    //         std::cout << val << "|";
    //     }
    //     std::cout << std::endl;
    // }

    return 0;
}
