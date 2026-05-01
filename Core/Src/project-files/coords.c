#include "main.h"
#include "project.h"
#include "math.h"

#include <float.h>
#include <stdio.h>
#include <string.h>

#define EARTH_RADIUS_MILES 3958.8f
#define DEG_TO_RAD (M_PI / 180.0f)

const char* format_distance(double miles) {
    static char buf[16];

    int whole   = (int)miles;
    int decimal = (int)((miles - whole) * 100);

    if (miles >= 100.0) {
        snprintf(buf, sizeof(buf), "99.99mi+");
    } else {
        snprintf(buf, sizeof(buf), "%d.%02dmi", whole, decimal);
    }

    return buf;
}

const char* get_bearing(double lat1, double lon1, double lat2, double lon2) {
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    // Determine NS component
    const char *ns = NULL;
    const char *ew = NULL;

    if (dlat > 0.01)       ns = "North";
    else if (dlat < -0.01) ns = "South";

    if (dlon > 0.01)       ew = "East";
    else if (dlon < -0.01) ew = "West";

    // Return combined or cardinal
    if (ns && ew) {
        if (strcmp(ns, "North") == 0 && strcmp(ew, "East") == 0) return "Northeast";
        if (strcmp(ns, "North") == 0 && strcmp(ew, "West") == 0) return "Northwest";
        if (strcmp(ns, "South") == 0 && strcmp(ew, "East") == 0) return "Southeast";
        if (strcmp(ns, "South") == 0 && strcmp(ew, "West") == 0) return "Southwest";
    }
    if (ns) return ns;
    if (ew) return ew;
    return "Here";
}

double distance(double lat1, double lon1, double lat2, double lon2) {

    double dlat = lat1 - lat2;
    double dlon = lon1 - lon2;

    return sqrt(dlat * dlat + dlon * dlon);
}

double distance_miles(double lat1, double lon1,
                     double lat2, double lon2) {
    double dlat = (lat2 - lat1) * DEG_TO_RAD;
    double dlon = (lon2 - lon1) * DEG_TO_RAD;

    double a =
        sinf(dlat / 2.0f) * sinf(dlat / 2.0f) +
        cosf(lat1 * DEG_TO_RAD) *
        cosf(lat2 * DEG_TO_RAD) *
        sinf(dlon / 2.0f) * sinf(dlon / 2.0f);

    double c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));

    return EARTH_RADIUS_MILES * c;
}

typedef struct {
    size_t node_idx;
    int    depth;
} KDStackFrame;

#define KD_MAX_DEPTH 32  // log2(KD_SIZE) rounded up, adjust if tree is larger

void kd_search(size_t root_idx, double lat, double lon,
               size_t *best_idx, double *best_dist)
{
    // Manual stack allocated statically — no heap, no recursive stack frames
    static KDStackFrame stack[KD_MAX_DEPTH * 2];
    int stack_top = 0;

    stack[stack_top++] = (KDStackFrame){ root_idx, 0 };

    while (stack_top > 0)
    {
        KDStackFrame frame = stack[--stack_top];
        size_t node_idx   = frame.node_idx;
        int    depth      = frame.depth;

        if (node_idx >= KD_SIZE) continue;

        // Check this node
        double d = distance(lat, lon,
                            kd_points[node_idx].lat,
                            kd_points[node_idx].lon);
        if (d < *best_dist) {
            *best_dist = d;
            *best_idx  = node_idx;
        }

        int    axis      = depth % 2;
        double delta     = (axis == 0)
                         ? (lat - kd_points[node_idx].lat)
                         : (lon - kd_points[node_idx].lon);

        size_t near_side = (delta <= 0) ? (2 * node_idx + 1) : (2 * node_idx + 2);
        size_t far_side  = (delta <= 0) ? (2 * node_idx + 2) : (2 * node_idx + 1);

        // Push far side first so near side is processed first (LIFO)
        // Only push far side if it could contain a closer point
        if (fabsf((float)delta) < (float)*best_dist)
            stack[stack_top++] = (KDStackFrame){ far_side, depth + 1 };

        stack[stack_top++] = (KDStackFrame){ near_side, depth + 1 };
    }
}


/*

void kd_search(size_t node_idx, double lat, double lon,
                      int depth, size_t* best_idx, double* best_dist) {

    if (node_idx > KD_SIZE) 
      return;

    double d = distance(lat, lon, kd_points[node_idx].lat, kd_points[node_idx].lon);

    if (d < *best_dist) {

        *best_dist = d;
        *best_idx = node_idx;
    }

    int axis = depth % 2;
    double delta = (axis == 0) ? (lat - kd_points[node_idx].lat) : (lon - kd_points[node_idx].lon);

    size_t near_side = (delta <= 0) ? (2 * node_idx + 1)  : (2 * node_idx + 2);
    size_t far_side = (delta <= 0) ? (2 * node_idx + 2) : (2 * node_idx + 1);

    kd_search(near_side, lat, lon, depth + 1, best_idx, best_dist);

    if (fabs(delta) < *best_dist)
        kd_search(far_side, lat, lon, depth + 1, best_idx, best_dist);

}

*/

location_t* find_nearest(double lat, double lon) {

    size_t best = 0;
    double best_dist = DBL_MAX;

    kd_search(0, lat, lon, &best, &best_dist);

    return &kd_points[best];
}

