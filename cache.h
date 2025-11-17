#ifndef CACHE
#define CACHE

#include <stddef.h>
#include <time.h>

#include "list.h"
#include "hashmap.h"

typedef struct FileCache {
    char* key;
    char* path;
    char* mime_type;
    size_t size;
    time_t mtime;
    time_t loaded_at;
    time_t expires_at;
    Node* lru_node;
} FileCache;

typedef struct Cache {
    HashMap* cache;
    List* cache_lru;
    int max_entries;
    int ttl;
} Cache;

Cache* cache_create(int max_entries, int ttl);
void cache_set(Cache* cache, const char* key, const char* path, const char* mime_type, size_t size, time_t mtime);
FileCache* cache_get(Cache* cache, const char* key);
void cache_destroy(Cache* cache);

#endif // CACHE
