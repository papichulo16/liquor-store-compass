#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define NAME_MAX_BYTES 32

typedef struct {
    double lat;
    double lon;
    char name[NAME_MAX_BYTES];
} location_t;

typedef struct kd_node_t {
    location_t location;
    struct kd_node_t* left;
    struct kd_node_t* right;
} kd_node_t;

kd_node_t* read_node(FILE *f) {

    double lat, lon;
    char name[NAME_MAX_BYTES];
    uint8_t has_left, has_right;

    if (fread(&lat, sizeof(double), 1, f) != 1) 
      return NULL;

    if (fread(&lon, sizeof(double), 1, f) != 1) 
      return NULL;

    if (fread(name, 1, NAME_MAX_BYTES, f) != NAME_MAX_BYTES) 
      return NULL;

    if (fread(&has_left, sizeof(uint8_t),1, f) != 1) 
      return NULL;

    if (fread(&has_right, sizeof(uint8_t),1, f) != 1) 
      return NULL;

    kd_node_t *node = malloc(sizeof(kd_node_t));

    node->location.lat = lat;
    node->location.lon = lon;

    memcpy(node->location.name, name, NAME_MAX_BYTES);

    node->left = has_left  ? read_node(f) : NULL;
    node->right = has_right ? read_node(f) : NULL;

    return node;
}

kd_node_t* load_kd_tree(const char *filename, uint32_t *out_count) {

    FILE* f = fopen(filename, "rb");

    if (!f) {

        fprintf(stderr, "Failed to open %s\n", filename);
        return NULL;
    }

    fread(out_count, sizeof(uint32_t), 1, f);
    kd_node_t *root = read_node(f);
    fclose(f);

    return root;
}

double distance(double lat1, double lon1, double lat2, double lon2) {

    double dlat = lat1 - lat2;
    double dlon = lon1 - lon2;

    return sqrt(dlat * dlat + dlon * dlon);
}

void kd_search(kd_node_t* node, double lat, double lon,
                      int depth, kd_node_t** best, double* best_dist) {

    if (node == NULL) 
      return;

    double d = distance(lat, lon, node->location.lat, node->location.lon);

    if (d < *best_dist) {

        *best_dist = d;
        *best = node;
    }

    int axis = depth % 2;
    double delta = (axis == 0) ? (lat - node->location.lat) : (lon - node->location.lon);

    kd_node_t* near_side = (delta <= 0) ? node->left  : node->right;
    kd_node_t* far_side = (delta <= 0) ? node->right : node->left;

    kd_search(near_side, lat, lon, depth + 1, best, best_dist);

    if (fabs(delta) < *best_dist)
        kd_search(far_side, lat, lon, depth + 1, best, best_dist);

}

kd_node_t* find_nearest(kd_node_t *root, double lat, double lon) {

    kd_node_t *best = NULL;
    double best_dist = DBL_MAX;

    kd_search(root, lat, lon, 0, &best, &best_dist);

    return best;
}

void free_kd_tree(kd_node_t* node) {

    if (!node) 
      return;

    free_kd_tree(node->left);
    free_kd_tree(node->right);

    free(node);
}

int main() {

    uint32_t count = 0;
    kd_node_t *root = load_kd_tree("liquor_stores.kd", &count);

    if (!root) 
      return 1;

    free_kd_tree(root);

    return 0;
}

