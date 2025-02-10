#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <iostream>
#include <cstring>

typedef struct {
    const char *text;
    int start;        
    int end;         
    int vowel_count; 
} ThreadData;

bool is_vowel(char c) {
    c = tolower(c);
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u');
}
void *count_vowels(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    data->vowel_count = 0;

    for (int i = data->start; i < data->end; i++) {
        if (is_vowel(data->text[i])) {
            data->vowel_count++;
        }
    }

    return NULL;
}
int main(){
	int threadsCount = 6;
	pthread_t threads[threadsCount];
	ThreadData thread_data[threadsCount];
	
	std::string str;
	std::cout << "type an english string\n";
	std::getline(std::cin, str);
	int textLength = str.size();
	int chunkSize = textLength / threadsCount;
	char text[textLength+1];
	std::strcpy(text,str.c_str());
		
	for(int i = 0; i<threadsCount; i++){
		thread_data[i].text = text;
		thread_data[i].start = i * chunkSize;
		thread_data[i].end = (i == threadsCount - 1) ? textLength+1 : (i + 1) * chunkSize;
		pthread_create(&threads[i],NULL,count_vowels,&thread_data[i]);
	}
	for (int i = 0; i < threadsCount; i++) {
	        pthread_join(threads[i], NULL);
	    }
	
	    // Суммируем результаты
	    int total_vowels = 0;
	    for (int i = 0; i < threadsCount; i++) {
	        total_vowels += thread_data[i].vowel_count;
	    }
	
	    std::cout << "Total vowels in the text: %d\n" << total_vowels << "\n";
	
	    return 0;
}
