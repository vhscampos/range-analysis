#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct List
{
    void* ptr;
    int minValue;
    int maxValue;
    struct List* next;
};

typedef struct List List;

typedef struct 
{
    void* ptr;
    int minValue;
    int maxValue;
    List* list;
} Hashitem;

#define RA_HASH_SIZE 65536

Hashitem hash[RA_HASH_SIZE];

void initHash()
{
    memset(hash, 0, sizeof(hash));
}

void init()
{
    initHash();
}

List* getFromList(List* list, void* ptr)
{
    while (list != 0){
        if (list->ptr == ptr) return list;
        list = list->next;
    }

    return 0;
}

List* setCurrentMinInList(List* list, void* ptr, int minValue)
{
    List *tmp;
    tmp = getFromList(list, ptr);
    if (tmp != 0){
        tmp->minValue = minValue;
        return list;
    } else {
        tmp = malloc(sizeof(List));
        tmp->minValue = minValue;
        tmp->maxValue = minValue;  
        tmp->next = list;
        return tmp;
    }
}


List* setCurrentMaxInList(List* list, void* ptr, int maxValue)
{
    List *tmp;
    tmp = getFromList(list, ptr);
    if (tmp != 0){
        tmp->maxValue = maxValue;
        return list;
    } else {
        tmp = malloc(sizeof(List));
        tmp->minValue = maxValue;
        tmp->maxValue = maxValue;  
        tmp->next = list;
        return tmp;
    }
}


int getCurrentMin(void* ptr)
{
    List *tmp;
    int key = (int) ptr & 0xffff;
    if (hash[key].ptr == ptr)
    {
        return hash[key].minValue;
    }
    else
    {
        tmp = getFromList(hash[key].list, ptr);
        if (tmp != 0){
            return tmp->minValue;
        } else {
            return INT_MAX;
        }
    }
}

int getCurrentMax(void* ptr)
{
    List *tmp;
    int key = (int) ptr & 0xffff;
    if (hash[key].ptr == ptr)
    {
        return hash[key].maxValue;
    }
    else
    {
        tmp = getFromList(hash[key].list, ptr);
        if (tmp != 0){
            return tmp->maxValue;
        } else {
            return INT_MIN;
        }
    }
}


void setCurrentMin(void* ptr, int minValue)
{
    int key = (int) ptr & 0xffff;
    
    if (hash[key].ptr == 0)
    {
        hash[key].ptr = ptr;
        hash[key].minValue = minValue;
        hash[key].maxValue = minValue;        
    }
    else if (hash[key].ptr == ptr)
    {
        hash[key].minValue = minValue;
    }
    else
    {
        hash[key].list = setCurrentMinInList(hash[key].list, ptr, minValue);    
    }
}




void setCurrentMax(void* ptr, int maxValue)
{
    int key = (int) ptr & 0xffff;
    if (hash[key].ptr == 0)
    {
        hash[key].ptr = ptr;
        hash[key].minValue = maxValue;
        hash[key].maxValue = maxValue;        
    }
    else if (hash[key].ptr == ptr)
    {
        hash[key].maxValue = maxValue;
    }
    else
    {
        hash[key].list = setCurrentMaxInList(hash[key].list, ptr, maxValue);    
    }
}

void setCurrentMinMax(void* ptr, int Value){
    
    if (Value < getCurrentMin(ptr))
        setCurrentMin(ptr, Value);
        
    if (Value > getCurrentMax(ptr))
        setCurrentMax(ptr, Value);
}

char* getFileName(const char* prefix, char* moduleName, const char* suffix){

	char* result = (char*)calloc(strlen(prefix) + strlen(moduleName) + strlen(suffix), sizeof(char));
	result[0] = '\0';
	strcat(result, prefix);
	strcat(result, moduleName);
	strcat(result, suffix);
	return result;

}

void printHash(char* moduleName)
{
    int i;
    char* FileName = getFileName("/tmp/RAHashValues.", moduleName, ".txt");

    FILE* file = fopen (FileName,"w");
    List* list;
    
    for(i = 0; i<RA_HASH_SIZE; i++){
        if (hash[i].ptr != 0) {
            fprintf(file, "%d %d %d\n", (int)hash[i].ptr, hash[i].minValue, hash[i].maxValue);
            list =  hash[i].list;
            
            while(list!=0){
                fprintf(file, "%d %d %d\n", (int)list->ptr, list->minValue, list->maxValue);
                list = list->next;
            }       
        }
    } 
    fclose(file);   
}
