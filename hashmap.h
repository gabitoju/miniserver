#ifndef HASHMAP
#define HASHMAP

struct Bucket;
typedef struct Bucket Bucket;

struct HashMap;
typedef struct HashMap HashMap;

HashMap* hashmap_create(int capacity);
void hashmap_destroy(HashMap* hash_map);
void hashmap_set(HashMap* hash_map, const char* key, void* value);
void* hashmap_get(HashMap* hash_map, const char* key);
void hashmap_remove(HashMap* hash_map, const char* key);
int hashmap_size(HashMap* hash_map);

#endif // HASHMAP
