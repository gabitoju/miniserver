#include "cache.h"
#include "hashmap.h"
#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static void free_cache_item(FileCache* item);

Cache* cache_create(int max_entries, int ttl) {
    HashMap* file_cache = hashmap_create(max_entries);
    List* lru = list_create();

    Cache* cache = malloc(sizeof(Cache));

    cache->cache = file_cache;
    cache->cache_lru = lru;
    cache->ttl = ttl;
    cache->max_entries = max_entries;

    return cache;
}


void cache_set(Cache* cache, const char* key, const char* path, char* data, const char* mime_type, int fd, size_t size, time_t mtime) {

    FileCache* existing_item = hashmap_get(cache->cache, key);
    if (existing_item != NULL) {
        if (existing_item->data) {
            free(existing_item->data);
        }
        existing_item->data = NULL;

        existing_item->fd = fd;
        existing_item->size = size;
        if (data != NULL) {
            existing_item->data = malloc(size);
            memcpy(existing_item->data, data, size);
        }
        existing_item->expires_at = time(NULL) + cache->ttl;

        list_remove(cache->cache_lru, existing_item->lru_node);
        list_push(cache->cache_lru, existing_item);
        existing_item->lru_node = cache->cache_lru->tail;
        return;
    }

    if (cache->cache_lru->size == cache->max_entries) {
        Node* stale_node = cache->cache_lru->head;
        if (stale_node) {
            FileCache* stale = (FileCache*)stale_node->data;
            hashmap_remove(cache->cache, stale->key);
            list_remove(cache->cache_lru, stale_node);
            free_cache_item(stale);
        }
    }

    FileCache* file_cache = malloc(sizeof(FileCache));
    file_cache->fd = fd;
    file_cache->size = size;
    file_cache->path = strdup(path);
    file_cache->key = strdup(key);
    file_cache->mime_type = strdup(mime_type);
    file_cache->mtime = mtime;
  
    if (data != NULL) {
        file_cache->data = malloc(size);
        memcpy(file_cache->data, data, size);
    }

    file_cache->loaded_at = time(NULL);
    file_cache->expires_at = time(NULL) + cache->ttl;

    list_push(cache->cache_lru, file_cache);
    file_cache->lru_node = cache->cache_lru->tail;
    hashmap_set(cache->cache, key, file_cache);
}

FileCache* cache_get(Cache* cache, const char* key) {
    FileCache* file_cache = hashmap_get(cache->cache, key);

    if (file_cache != NULL) {
        time_t now = time(NULL);

        if (file_cache->expires_at < now) {
            hashmap_remove(cache->cache, key);
            list_remove(cache->cache_lru, file_cache->lru_node);
            free_cache_item(file_cache);
            file_cache = NULL;
        } else {
            list_remove(cache->cache_lru, file_cache->lru_node);
            list_push(cache->cache_lru, file_cache);
            file_cache->lru_node = cache->cache_lru->tail;
        }


    }

    return file_cache;
}

void cache_destroy(Cache *cache) {
    hashmap_destroy(cache->cache);

    if (cache->cache_lru->size > 0) {
        Node* item = cache->cache_lru->head;

        while (item != NULL) {
            FileCache* file_cache = (FileCache*)item->data;
            item = item->next;
            free_cache_item(file_cache);
        }
    }

    list_destroy(cache->cache_lru);
    free(cache);
}

static void free_cache_item(FileCache* item) {
    
    if (item->key) {
        free(item->key);
    }

    if (item->data) {
        free(item->data);
    }

    if (item->path) {
        free(item->path);
    }

    if (item->mime_type) {
        free(item->mime_type);
    }

    if (item->fd >= 0) {
        close(item->fd);
    }

    free(item);
}
