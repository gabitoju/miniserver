#include "cache.h"
#include "hashmap.h"
#include "list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

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


void cache_set(Cache* cache, const char* key, const char* path, const char* mime_type, const char* headers, size_t size, time_t mtime) {

    FileCache* existing_item = hashmap_get(cache->cache, key);
    if (existing_item != NULL) {

        existing_item->size = size;
        existing_item->expires_at = time(NULL) + cache->ttl;
        if (strcmp(existing_item->path, path) != 0) {
            free(existing_item->path);
            existing_item->path = strdup(path);
        }
        if (mime_type != NULL && strcmp(existing_item->mime_type, mime_type) != 0) {
            free(existing_item->mime_type);
            existing_item->mime_type = mime_type ? strdup(mime_type) : NULL;
        }
        if (headers != NULL && strcmp(existing_item->headers, headers) != 0) {
            free(existing_item->headers);
            existing_item->headers = strdup(headers);
        }
        existing_item->mtime = mtime;

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
    file_cache->size = size;
    file_cache->path = strdup(path);
    file_cache->key = strdup(key);
    file_cache->mime_type = mime_type ? strdup(mime_type) : NULL;
    file_cache->headers = strdup(headers);
    file_cache->mtime = mtime;
  
    file_cache->loaded_at = time(NULL);
    file_cache->expires_at = time(NULL) + cache->ttl;

    hashmap_set(cache->cache, key, file_cache);
    list_push(cache->cache_lru, file_cache);
    file_cache->lru_node = cache->cache_lru->tail;
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

    if (cache->cache_lru->size > 0) {
        Node* item = cache->cache_lru->head;

        while (item != NULL) {
            FileCache* file_cache = (FileCache*)item->data;
            item = item->next;
            free_cache_item(file_cache);
        }
    }

    list_destroy(cache->cache_lru);
    hashmap_destroy(cache->cache);
    free(cache);
}

static void free_cache_item(FileCache* item) {
    
    if (item->key) {
        free(item->key);
    }

    if (item->path) {
        free(item->path);
    }

    if (item->mime_type) {
        free(item->mime_type);
    }

    if (item->headers) {
        free(item->headers);
    }

    free(item);
}
