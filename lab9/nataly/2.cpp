#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>

using namespace std;

// Разделяет строку на слова
vector<string> split(const string& line) {
    stringstream ss(line);
    vector<string> result;
    string word;
    while (ss >> word) {
        result.push_back(word);
    }
    return result;
}

// Группировка по определенному полю
map<string, vector<string>> groupByField(const vector<vector<string>>& records, int fieldIndex) {
    map<string, vector<string>> result;
    for (const auto& record : records) {
        if (record.size() < 3) continue;
        string key = record[fieldIndex];
        string fullName = record[0] + " " + record[1] + " " + record[2];
        result[key].push_back(fullName);
    }
    return result;
}

// Сериализация записей в строку
string serializeRecords(const vector<vector<string>>& records) {
    stringstream ss;
    for (const auto& record : records) {
        if (record.size() == 3)
            ss << record[0] << " " << record[1] << " " << record[2] << "\n";
    }
    return ss.str();
}

// Десериализация строк в записи
vector<vector<string>> deserializeRecords(const string& data) {
    stringstream ss(data);
    vector<vector<string>> records;
    string line;
    while (getline(ss, line)) {
        records.push_back(split(line));
    }
    return records;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int fieldIndex = rank; // 0: фамилия, 1: имя, 2: отчество

    vector<vector<string>> records;

    if (rank == 0) {
        ifstream inputFile("input.txt");
        if (!inputFile) {
            cerr << "Ошибка открытия файла!" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        string line;
        while (getline(inputFile, line)) {
            vector<string> record = split(line);
            if (record.size() == 3)
                records.push_back(record);
        }

        inputFile.close();
        string serialized = serializeRecords(records);

        // Рассылаем данные другим процессам
        for (int i = 1; i < size && i <= 2; i++) {
            int len = serialized.length();
            MPI_Send(&len, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(serialized.c_str(), len, MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }
    }

    // Остальные процессы получают данные
    if (rank != 0) {
        int len;
        MPI_Recv(&len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        char* buffer = new char[len + 1];
        MPI_Recv(buffer, len, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        buffer[len] = '\0';
        string data(buffer);
        records = deserializeRecords(data);
        delete[] buffer;
    }

    // Все процессы группируют по своему полю
    if (rank <= 2) {
        auto result = groupByField(records, fieldIndex);
        for (const auto& entry : result) {
            ofstream outFile(entry.first + ".txt");
            for (const string& name : entry.second) {
                outFile << name << endl;
            }
        }
    }

    if (rank == 0) {
        cout << "Группировка завершена, файлы созданы." << endl;
    }

    MPI_Finalize();
    return 0;
}