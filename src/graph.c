/**
 * graph.c - Graph implementation
 */

#include "graph.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Initialize graph
void graph_init(Graph* graph) {
    if (!graph) return;
    
    graph->nodeCount = 0;
    memset(graph->nodes, 0, sizeof(graph->nodes));
    memset(graph->edges, 0, sizeof(graph->edges));
    memset(graph->edgeCounts, 0, sizeof(graph->edgeCounts));
}

// Free graph resources
void graph_free(Graph* graph) {
    if (!graph) return;
    graph_init(graph);  // Reset to initial state
}

// Add a node to the graph
int graph_add_node(Graph* graph, const char* name, float x, float y) {
    if (!graph || graph->nodeCount >= MAX_NODES) return -1;
    
    int id = graph->nodeCount;
    Node* node = &graph->nodes[id];
    
    node->id = id;
    strncpy(node->name, name, MAX_NAME_LENGTH - 1);
    node->name[MAX_NAME_LENGTH - 1] = '\0';
    node->x = x;
    node->y = y;
    node->active = true;
    
    graph->nodeCount++;
    return id;
}

// Remove a node (mark as inactive)
bool graph_remove_node(Graph* graph, int nodeId) {
    if (!graph || nodeId < 0 || nodeId >= graph->nodeCount) return false;
    
    graph->nodes[nodeId].active = false;
    
    // Remove all edges to/from this node
    graph->edgeCounts[nodeId] = 0;
    
    // Remove edges from other nodes to this one
    for (int i = 0; i < graph->nodeCount; i++) {
        if (i == nodeId) continue;
        for (int j = 0; j < graph->edgeCounts[i]; j++) {
            if (graph->edges[i][j].to == nodeId) {
                graph->edges[i][j].active = false;
            }
        }
    }
    
    return true;
}

// Get a node by ID
Node* graph_get_node(Graph* graph, int nodeId) {
    if (!graph || nodeId < 0 || nodeId >= graph->nodeCount) return NULL;
    if (!graph->nodes[nodeId].active) return NULL;
    return &graph->nodes[nodeId];
}

// Find node by name (case-insensitive partial match)
int graph_find_node_by_name(const Graph* graph, const char* name) {
    if (!graph || !name) return -1;
    
    // First try exact match
    for (int i = 0; i < graph->nodeCount; i++) {
        if (graph->nodes[i].active) {
            #ifdef _WIN32
            if (_stricmp(graph->nodes[i].name, name) == 0) return i;
            #else
            if (strcasecmp(graph->nodes[i].name, name) == 0) return i;
            #endif
        }
    }
    
    // Then try partial match
    for (int i = 0; i < graph->nodeCount; i++) {
        if (graph->nodes[i].active) {
            #ifdef _WIN32
            if (strstr(graph->nodes[i].name, name) != NULL) return i;
            #else
            // Case-insensitive substring search
            const char* haystack = graph->nodes[i].name;
            size_t nameLen = strlen(name);
            size_t hayLen = strlen(haystack);
            for (size_t j = 0; j <= hayLen - nameLen; j++) {
                bool match = true;
                for (size_t k = 0; k < nameLen; k++) {
                    char c1 = haystack[j + k];
                    char c2 = name[k];
                    if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
                    if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
                    if (c1 != c2) { match = false; break; }
                }
                if (match) return i;
            }
            #endif
        }
    }
    
    return -1;
}

// Find node at screen position
int graph_find_node_at_position(const Graph* graph, float x, float y, float radius) {
    if (!graph) return -1;
    
    float radiusSq = radius * radius;
    int closest = -1;
    float closestDist = radiusSq;
    
    for (int i = 0; i < graph->nodeCount; i++) {
        if (!graph->nodes[i].active) continue;
        
        float dx = graph->nodes[i].x - x;
        float dy = graph->nodes[i].y - y;
        float distSq = dx * dx + dy * dy;
        
        if (distSq < closestDist) {
            closestDist = distSq;
            closest = i;
        }
    }
    
    return closest;
}

// Add a directed edge
bool graph_add_edge(Graph* graph, int from, int to, float weight) {
    if (!graph) return false;
    if (from < 0 || from >= graph->nodeCount) return false;
    if (to < 0 || to >= graph->nodeCount) return false;
    if (graph->edgeCounts[from] >= MAX_EDGES_PER_NODE) return false;
    
    // Check if edge already exists
    if (graph_has_edge(graph, from, to)) return false;
    
    int idx = graph->edgeCounts[from];
    graph->edges[from][idx].from = from;
    graph->edges[from][idx].to = to;
    graph->edges[from][idx].weight = weight;
    graph->edges[from][idx].active = true;
    graph->edgeCounts[from]++;
    
    return true;
}

// Add bidirectional edge
bool graph_add_edge_bidirectional(Graph* graph, int from, int to, float weight) {
    bool a = graph_add_edge(graph, from, to, weight);
    bool b = graph_add_edge(graph, to, from, weight);
    return a || b;  // Return true if at least one was added
}

// Remove an edge
bool graph_remove_edge(Graph* graph, int from, int to) {
    if (!graph || from < 0 || from >= graph->nodeCount) return false;
    
    for (int i = 0; i < graph->edgeCounts[from]; i++) {
        if (graph->edges[from][i].to == to && graph->edges[from][i].active) {
            graph->edges[from][i].active = false;
            return true;
        }
    }
    
    return false;
}

// Get edge weight
float graph_get_edge_weight(const Graph* graph, int from, int to) {
    if (!graph || from < 0 || from >= graph->nodeCount) return -1.0f;
    
    for (int i = 0; i < graph->edgeCounts[from]; i++) {
        if (graph->edges[from][i].to == to && graph->edges[from][i].active) {
            return graph->edges[from][i].weight;
        }
    }
    
    return -1.0f;  // No edge
}

// Check if edge exists
bool graph_has_edge(const Graph* graph, int from, int to) {
    if (!graph || from < 0 || from >= graph->nodeCount) return false;
    
    for (int i = 0; i < graph->edgeCounts[from]; i++) {
        if (graph->edges[from][i].to == to && graph->edges[from][i].active) {
            return true;
        }
    }
    
    return false;
}

// Calculate Euclidean distance between nodes
float graph_calculate_distance(const Node* a, const Node* b) {
    if (!a || !b) return 0.0f;
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    return sqrtf(dx * dx + dy * dy);
}

// Get all neighbors of a node
int graph_get_neighbors(const Graph* graph, int nodeId, int* neighbors, int maxNeighbors) {
    if (!graph || !neighbors || nodeId < 0 || nodeId >= graph->nodeCount) return 0;
    
    int count = 0;
    for (int i = 0; i < graph->edgeCounts[nodeId] && count < maxNeighbors; i++) {
        if (graph->edges[nodeId][i].active) {
            int to = graph->edges[nodeId][i].to;
            if (graph->nodes[to].active) {
                neighbors[count++] = to;
            }
        }
    }
    
    return count;
}

// Save graph to file
bool graph_save(const Graph* graph, const char* filename) {
    if (!graph || !filename) return false;
    
    FILE* file = fopen(filename, "wb");
    if (!file) return false;
    
    // Write header
    const char magic[] = "RCGRAPH1";
    fwrite(magic, 1, 8, file);
    
    // Write node count
    fwrite(&graph->nodeCount, sizeof(int), 1, file);
    
    // Write nodes
    for (int i = 0; i < graph->nodeCount; i++) {
        const Node* node = &graph->nodes[i];
        fwrite(&node->id, sizeof(int), 1, file);
        fwrite(node->name, sizeof(char), MAX_NAME_LENGTH, file);
        fwrite(&node->x, sizeof(float), 1, file);
        fwrite(&node->y, sizeof(float), 1, file);
        fwrite(&node->active, sizeof(bool), 1, file);
    }
    
    // Write edge counts
    fwrite(graph->edgeCounts, sizeof(int), MAX_NODES, file);
    
    // Write edges
    for (int i = 0; i < graph->nodeCount; i++) {
        for (int j = 0; j < graph->edgeCounts[i]; j++) {
            const Edge* edge = &graph->edges[i][j];
            fwrite(&edge->from, sizeof(int), 1, file);
            fwrite(&edge->to, sizeof(int), 1, file);
            fwrite(&edge->weight, sizeof(float), 1, file);
            fwrite(&edge->active, sizeof(bool), 1, file);
        }
    }
    
    fclose(file);
    return true;
}

// Load graph from file
bool graph_load(Graph* graph, const char* filename) {
    if (!graph || !filename) return false;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return false;
    
    // Read and verify header
    char magic[8];
    if (fread(magic, 1, 8, file) != 8 || memcmp(magic, "RCGRAPH1", 8) != 0) {
        fclose(file);
        return false;
    }
    
    // Initialize graph
    graph_init(graph);
    
    // Read node count
    if (fread(&graph->nodeCount, sizeof(int), 1, file) != 1) {
        fclose(file);
        return false;
    }
    
    // Read nodes
    for (int i = 0; i < graph->nodeCount; i++) {
        Node* node = &graph->nodes[i];
        fread(&node->id, sizeof(int), 1, file);
        fread(node->name, sizeof(char), MAX_NAME_LENGTH, file);
        fread(&node->x, sizeof(float), 1, file);
        fread(&node->y, sizeof(float), 1, file);
        fread(&node->active, sizeof(bool), 1, file);
    }
    
    // Read edge counts
    fread(graph->edgeCounts, sizeof(int), MAX_NODES, file);
    
    // Read edges
    for (int i = 0; i < graph->nodeCount; i++) {
        for (int j = 0; j < graph->edgeCounts[i]; j++) {
            Edge* edge = &graph->edges[i][j];
            fread(&edge->from, sizeof(int), 1, file);
            fread(&edge->to, sizeof(int), 1, file);
            fread(&edge->weight, sizeof(float), 1, file);
            fread(&edge->active, sizeof(bool), 1, file);
        }
    }
    
    fclose(file);
    return true;
}

// Path result management
PathResult path_result_create(void) {
    PathResult result = {0};
    result.nodes = NULL;
    result.length = 0;
    result.totalCost = 0.0f;
    result.found = false;
    return result;
}

void path_result_free(PathResult* result) {
    if (result && result->nodes) {
        free(result->nodes);
        result->nodes = NULL;
        result->length = 0;
    }
}
