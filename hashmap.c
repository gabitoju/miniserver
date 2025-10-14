#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"


struct Bucket {
    const char* key;
    void* value;
    struct Bucket* next;

};

struct HashMap {
    int capacity;
    int size;
    Bucket** buckets;
};

static unsigned long _get_bucket_key(HashMap* hash_map, const char* key) {

    unsigned long hash = 5381;
    int c;

    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % hash_map->capacity;
}

HashMap* hashmap_create(int capacity) {

    if (capacity <= 0) {
        return NULL;
    }

    HashMap* hash_map = malloc(sizeof(HashMap));

    if (hash_map == NULL) {
        perror("error allocation hash memory");
        return NULL;
    }

    hash_map->capacity = capacity;
    hash_map->size = 0;
    hash_map->buckets = calloc(capacity, sizeof(Bucket*));
    if (hash_map->buckets == NULL) {
        perror("error allocating buckets memory");
        free(hash_map);
        return NULL;
    }

    return hash_map;
}

void hashmap_destroy(HashMap *hash_map) {

    if (hash_map == NULL) {
        return;
    }

    for (int i = 0; i < hash_map->capacity; i++) {
        if (hash_map->buckets[i] != NULL) {
            Bucket* bucket = hash_map->buckets[i];
            while (bucket != NULL) {
                Bucket* next = bucket->next;
                free((void*)bucket->key);
                free(bucket->value);
                free(bucket);
                bucket = next;
            }
        }
    }
    free(hash_map->buckets);
    free(hash_map);
}

void hashmap_set(HashMap *hash_map, const char *key, void *value) {
    unsigned long bucket_key = _get_bucket_key(hash_map, key);

    Bucket* bucket = hash_map->buckets[bucket_key];

    while (bucket != NULL) {
        if (strcmp(key, bucket->key) == 0) {
            bucket->value = value;
            return;
        } else {
            bucket = bucket->next;
        }
    }

    Bucket* new_bucket = malloc(sizeof(Bucket));
    if (new_bucket == NULL) {
        perror("error allocation bucket memory");
        return;
    }

    new_bucket->key = strdup(key);
    new_bucket->value = value;
    new_bucket->next = NULL;


    new_bucket->next = hash_map->buckets[bucket_key];
    hash_map->buckets[bucket_key] = new_bucket;
    hash_map->size++;
}

void* hashmap_get(HashMap *hash_map, const char *key) {
    unsigned long bucket_key = _get_bucket_key(hash_map, key);

    Bucket* bucket = hash_map->buckets[bucket_key];

    while (bucket != NULL) {
        if (strcmp(key, bucket->key) == 0) {
            return bucket->value;
        }
        bucket = bucket->next;
    }

    return NULL;
}

void hashmap_remove(HashMap *hash_map, const char *key) {
    int bucket_key = _get_bucket_key(hash_map, key);

    Bucket* bucket = hash_map->buckets[bucket_key];
    Bucket* previous = NULL;
    while (bucket != NULL) {
        if (strcmp(key, bucket->key) == 0) {        

            if (previous != NULL) {
                previous->next = bucket->next;
            } else {
                hash_map->buckets[bucket_key] = bucket->next;
            }

            free((void*)bucket->key);
            free(bucket);
            bucket = NULL;
            hash_map->size--;
            return;

        }
        previous = bucket;
        bucket = bucket->next;
    }
}

int hashmap_size(HashMap *hash_map) {
    return hash_map->size;
}
