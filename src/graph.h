/**
 * graph.h - Graph data structure for map representation
 * 
 * This module provides a graph-based map representation where:
 * - Nodes represent locations/addresses
 * - Edges represent roads/paths between locations
 * - Weights represent distances
 */

#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum limits
#define MAX_NODES 1000
#define MAX_EDGES_PER_NODE 20
#define MAX_NAME_LENGTH 128

// Node structure representing a location
typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    float x;  // Screen/map x coordinate
    float y;  // Screen/map y coordinate
    bool active;
} Node;

// Edge structure representing a connection
typedef struct {
    int from;
    int to;
    float weight;  // Distance/cost
    bool active;
} Edge;

// Graph structure
typedef struct {
    Node nodes[MAX_NODES];
    int nodeCount;
    
    // Adjacency list representation
    Edge edges[MAX_NODES][MAX_EDGES_PER_NODE];
    int edgeCounts[MAX_NODES];
} Graph;

// Path result from A* algorithm
typedef struct {
    int* nodes;         // Array of node IDs in the path
    int length;         // Number of nodes in path
    float totalCost;    // Total distance
    bool found;         // Whether a path was found
} PathResult;

// Graph lifecycle
void graph_init(Graph* graph);
void graph_free(Graph* graph);

// Node operations
int graph_add_node(Graph* graph, const char* name, float x, float y);
bool graph_remove_node(Graph* graph, int nodeId);
Node* graph_get_node(Graph* graph, int nodeId);
int graph_find_node_by_name(const Graph* graph, const char* name);
int graph_find_node_at_position(const Graph* graph, float x, float y, float radius);

// Edge operations
bool graph_add_edge(Graph* graph, int from, int to, float weight);
bool graph_add_edge_bidirectional(Graph* graph, int from, int to, float weight);
bool graph_remove_edge(Graph* graph, int from, int to);
float graph_get_edge_weight(const Graph* graph, int from, int to);
bool graph_has_edge(const Graph* graph, int from, int to);

// Serialization
bool graph_save(const Graph* graph, const char* filename);
bool graph_load(Graph* graph, const char* filename);

// Utility
float graph_calculate_distance(const Node* a, const Node* b);
int graph_get_neighbors(const Graph* graph, int nodeId, int* neighbors, int maxNeighbors);

// Path result management
PathResult path_result_create(void);
void path_result_free(PathResult* result);

#ifdef __cplusplus
}
#endif

#endif // GRAPH_H
