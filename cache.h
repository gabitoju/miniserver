#ifndef CACHE
#define CACHE

#include "hashmap.h"
#include "list.h"
#include <stddef.h>
#include <time.h>

typedef struct FileCache {
    char* key;
    char* path;
    char* data;
    char* mime_type;
    size_t size;
    time_t mtime;
    time_t loaded_at;
    time_t expires_at;
    int fd;
    Node* lru_node;
} FileCache;

typedef struct Cache {
    HashMap* cache;
    List* cache_lru;
    int max_entries;
    int ttl;
} Cache;

Cache* cache_create(int max_entries, int ttl);
void cache_set(Cache* cache, const char* key, const char* path, char* data, const char* mime_type, int fd, size_t size, time_t mtime);
FileCache* cache_get(Cache* cache, const char* key);
void cache_destroy(Cache* cache);

#endif // CACHE
