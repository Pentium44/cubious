#include <stdlib.h>
#include "map.h"
#include "db.h"
#include "util.h" // random_number
#include "noise.h"

#define CHUNK_SIZE 32

int hash_int(int key) {
    key = ~key + (key << 15);
    key = key ^ (key >> 12);
    key = key + (key << 2);
    key = key ^ (key >> 4);
    key = key * 2057;
    key = key ^ (key >> 16);
    return key;
}

int hash(int x, int y, int z) {
    x = hash_int(x);
    y = hash_int(y);
    z = hash_int(z);
    return x ^ y ^ z;
}

void map_alloc(Map *map) {
    map->mask = 0xfff;
    map->size = 0;
    map->data = (Entry *)calloc(map->mask + 1, sizeof(Entry));
}

void map_free(Map *map) {
    free(map->data);
}

void map_grow(Map *map);

void map_set(Map *map, int x, int y, int z, int w) {
    unsigned int index = hash(x, y, z) & map->mask;
    Entry *entry = map->data + index;
    int overwrite = 0;
    while (!EMPTY_ENTRY(entry)) {
        if (entry->x == x && entry->y == y && entry->z == z) {
            overwrite = 1;
            break;
        }
        index = (index + 1) & map->mask;
        entry = map->data + index;
    }
    if (overwrite) {
        entry->w = w;
    }
    else if (w) {
        entry->x = x;
        entry->y = y;
        entry->z = z;
        entry->w = w;
        map->size++;
        if (map->size * 2 > map->mask) {
            map_grow(map);
        }
    }
}

int map_get(Map *map, int x, int y, int z) {
    unsigned int index = hash(x, y, z) & map->mask;
    Entry *entry = map->data + index;
    while (!EMPTY_ENTRY(entry)) {
        if (entry->x == x && entry->y == y && entry->z == z) {
            return entry->w;
        }
        index = (index + 1) & map->mask;
        entry = map->data + index;
    }
    return 0;
}

void map_grow(Map *map) {
    Map new_map;
    new_map.mask = (map->mask << 1) | 1;
    new_map.size = 0;
    new_map.data = (Entry *)calloc(new_map.mask + 1, sizeof(Entry));
    for (unsigned int index = 0; index <= map->mask; index++) {
        Entry *entry = map->data + index;
        if (!EMPTY_ENTRY(entry)) {
            map_set(&new_map, entry->x, entry->y, entry->z, entry->w);
        }
    }
    free(map->data);
    map->mask = new_map.mask;
    map->size = new_map.size;
    map->data = new_map.data;
}

// Generate map on spawn - Generate chunks
void make_world(Map *map, int p, int q) {
    int pad = 1;
    for (int dx = -pad; dx < CHUNK_SIZE + pad; dx++) {
        for (int dz = -pad; dz < CHUNK_SIZE + pad; dz++) {
            int x = p * CHUNK_SIZE + dx; // X axis
            int z = q * CHUNK_SIZE + dz; // Z axis
            float f = simplex2(x * 0.01, z * 0.01, 4, 0.5, 2);
            float g = simplex2(-x * 0.01, -z * 0.01, 2, 0.9, 2);
            int mh = g * 32 + 16;
            int h = f * mh;
            int w = 1;
            int t = 12;
            if (h <= t) {
                h = t;
                w = 2;
            }
            if (dx < 0 || dz < 0 || dx >= CHUNK_SIZE || dz >= CHUNK_SIZE) {
                w = -1;
            }
            
            // grass gen
            for (int y = 0; y < h; y++) {
                map_set(map, x, y, z, w);
            }
            
            // Complete trees!
            int open_confirm = 1;
            if(dx - 3 < 0 || dz - 3 < 0 || dx + 4 >= CHUNK_SIZE || dz + 4 >= CHUNK_SIZE) {
				open_confirm = 0;
			}
			
			if(open_confirm && simplex2(x, z, 6, 0.5, 2) > 0.84) {
				for (int y = h + 3; y < h + 8; y++) {
					for (int ox = -3; ox <= 3; ox++) {
						for (int oz = -3; oz <= 3; oz++) {
						int d = (ox * ox) + (oz * oz) + (y - (h + 4)) * (y - (h + 4));
							if (d < 11) {
								map_set(map, x + ox, y, z + oz, 7);
							}
						}
					}
				}
				for (int y = h; y < h + 7; y++) {
					map_set(map, x, y, z, 5);
				}
			}
			
			// Gen plants
			if(w == 1 && simplex2(x * 0.1, z * 0.15, 4, 0.5, 2) > 0.76) {
				map_set(map, x, h, z, 17); // write plants to map
			}
			
			// Gen long grass
			if(w == 1 && simplex2(x * 0.2, z * 0.3, 4, 0.5, 2) > 0.68) {
				map_set(map, x, h, z, 18); // write long grass to map
			}
			
			// NEW - Tall grass biomes
			// Gen tall grass biomes
			if(w == 1 && simplex2(x * 0.014, z * 0.025, 6, 0.5, 2) > 0.78) {
				map_set(map, x, h, z, 18);
				map_set(map, x, h+1, z, 18);
				
				// Get the grass to generate at different heights
				int check = random_number(0,1);
				if(check) {
					map_set(map, x, h+2, z, 18);
				}
			}
			
			
			// Gen clouds
			for(int y = 70; y < 78; y++) {
				if(simplex3(x * 0.014, y * 0.1, z * 0.014, 6, 0.5, 2) > 0.70) {
					map_set(map, x, y, z, 32); // write clouds to map
				}
			}
        }
    }
    db_update_chunk(map, p, q);
}
