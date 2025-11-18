#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>
#include <time.h>

struct HashMap;

struct List;
struct Node;

typedef struct FileCache {
    char* key;
    char* path;
    char* mime_type;
    char* headers;
    size_t size;
    time_t mtime;
    time_t loaded_at;
    time_t expires_at;
    struct Node* lru_node;
} FileCache;

typedef struct Cache {
    struct HashMap* cache;
    struct List* cache_lru;
    int max_entries;
    int ttl;
} Cache;

Cache* cache_create(int max_entries, int ttl);
void cache_set(Cache* cache, const char* key, const char* path, const char* mime_type, const char* headers, size_t size, time_t mtime);
FileCache* cache_get(Cache* cache, const char* key);
void cache_destroy(Cache* cache);

#endif // CACHE_H
