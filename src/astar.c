/**
 * astar.c - A* Pathfinding Algorithm Implementation
 * 
 * This is an efficient implementation using a binary min-heap as the priority queue.
 * The heuristic function guides the search, making A* optimal and complete when
 * the heuristic is admissible (never overestimates the true cost).
 */

#include "astar.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

// Priority queue node for A*
typedef struct {
    int nodeId;
    float fScore;  // f = g + h
} PQNode;

// Min-heap priority queue
typedef struct {
    PQNode* nodes;
    int size;
    int capacity;
} PriorityQueue;

// Timer for performance measurement
static double get_time_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
#endif
}

// Priority Queue operations
static PriorityQueue* pq_create(int capacity) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    if (!pq) return NULL;
    
    pq->nodes = (PQNode*)malloc(capacity * sizeof(PQNode));
    if (!pq->nodes) {
        free(pq);
        return NULL;
    }
    
    pq->size = 0;
    pq->capacity = capacity;
    return pq;
}

static void pq_free(PriorityQueue* pq) {
    if (pq) {
        free(pq->nodes);
        free(pq);
    }
}

static void pq_swap(PQNode* a, PQNode* b) {
    PQNode temp = *a;
    *a = *b;
    *b = temp;
}

static void pq_heapify_up(PriorityQueue* pq, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (pq->nodes[idx].fScore < pq->nodes[parent].fScore) {
            pq_swap(&pq->nodes[idx], &pq->nodes[parent]);
            idx = parent;
        } else {
            break;
        }
    }
}

static void pq_heapify_down(PriorityQueue* pq, int idx) {
    while (true) {
        int smallest = idx;
        int left = 2 * idx + 1;
        int right = 2 * idx + 2;
        
        if (left < pq->size && pq->nodes[left].fScore < pq->nodes[smallest].fScore) {
            smallest = left;
        }
        if (right < pq->size && pq->nodes[right].fScore < pq->nodes[smallest].fScore) {
            smallest = right;
        }
        
        if (smallest != idx) {
            pq_swap(&pq->nodes[idx], &pq->nodes[smallest]);
            idx = smallest;
        } else {
            break;
        }
    }
}

static bool pq_push(PriorityQueue* pq, int nodeId, float fScore) {
    if (pq->size >= pq->capacity) return false;
    
    pq->nodes[pq->size].nodeId = nodeId;
    pq->nodes[pq->size].fScore = fScore;
    pq_heapify_up(pq, pq->size);
    pq->size++;
    return true;
}

static bool pq_pop(PriorityQueue* pq, int* nodeId, float* fScore) {
    if (pq->size == 0) return false;
    
    *nodeId = pq->nodes[0].nodeId;
    *fScore = pq->nodes[0].fScore;
    
    pq->size--;
    if (pq->size > 0) {
        pq->nodes[0] = pq->nodes[pq->size];
        pq_heapify_down(pq, 0);
    }
    return true;
}

static bool pq_empty(const PriorityQueue* pq) {
    return pq->size == 0;
}

// Update priority of existing node or add if not present
static void pq_decrease_priority(PriorityQueue* pq, int nodeId, float newFScore) {
    // Find the node
    for (int i = 0; i < pq->size; i++) {
        if (pq->nodes[i].nodeId == nodeId) {
            if (newFScore < pq->nodes[i].fScore) {
                pq->nodes[i].fScore = newFScore;
                pq_heapify_up(pq, i);
            }
            return;
        }
    }
    // Node not found, add it
    pq_push(pq, nodeId, newFScore);
}

// Heuristic functions
float astar_heuristic(const Node* a, const Node* b, HeuristicType type) {
    if (!a || !b) return 0.0f;
    
    float dx = fabsf(b->x - a->x);
    float dy = fabsf(b->y - a->y);
    
    switch (type) {
        case HEURISTIC_EUCLIDEAN:
            return sqrtf(dx * dx + dy * dy);
            
        case HEURISTIC_MANHATTAN:
            return dx + dy;
            
        case HEURISTIC_CHEBYSHEV:
            return fmaxf(dx, dy);
            
        case HEURISTIC_ZERO:
        default:
            return 0.0f;  // Dijkstra's algorithm
    }
}

// Default configuration
AStarConfig astar_default_config(void) {
    AStarConfig config;
    config.heuristic = HEURISTIC_EUCLIDEAN;
    config.heuristicWeight = 1.0f;
    config.allowDiagonal = true;
    return config;
}

// Reconstruct path from came_from array
static PathResult reconstruct_path(
    const int* cameFrom,
    const float* gScore,
    int startId,
    int goalId,
    int nodeCount
) {
    PathResult result = path_result_create();
    
    // Count path length
    int length = 0;
    int current = goalId;
    while (current != startId && current != -1 && length < nodeCount) {
        length++;
        current = cameFrom[current];
    }
    if (current == startId) length++;  // Include start node
    
    if (current != startId) {
        return result;  // No valid path
    }
    
    // Allocate path array
    result.nodes = (int*)malloc(length * sizeof(int));
    if (!result.nodes) return result;
    
    result.length = length;
    result.found = true;
    result.totalCost = gScore[goalId];
    
    // Fill path in reverse order
    current = goalId;
    for (int i = length - 1; i >= 0; i--) {
        result.nodes[i] = current;
        current = cameFrom[current];
    }
    
    return result;
}

// Main A* algorithm
PathResult astar_find_path(
    const Graph* graph,
    int startId,
    int goalId,
    const AStarConfig* config,
    AStarStats* stats
) {
    PathResult result = path_result_create();
    
    if (!graph || startId < 0 || goalId < 0 || 
        startId >= graph->nodeCount || goalId >= graph->nodeCount) {
        return result;
    }
    
    // Use default config if none provided
    AStarConfig cfg = config ? *config : astar_default_config();
    
    // Initialize stats
    AStarStats localStats = {0};
    double startTime = get_time_ms();
    
    int nodeCount = graph->nodeCount;
    
    // Allocate working arrays
    float* gScore = (float*)malloc(nodeCount * sizeof(float));
    float* fScore = (float*)malloc(nodeCount * sizeof(float));
    int* cameFrom = (int*)malloc(nodeCount * sizeof(int));
    bool* inClosedSet = (bool*)calloc(nodeCount, sizeof(bool));
    bool* inOpenSet = (bool*)calloc(nodeCount, sizeof(bool));
    
    if (!gScore || !fScore || !cameFrom || !inClosedSet || !inOpenSet) {
        free(gScore);
        free(fScore);
        free(cameFrom);
        free(inClosedSet);
        free(inOpenSet);
        return result;
    }
    
    // Initialize scores to infinity
    for (int i = 0; i < nodeCount; i++) {
        gScore[i] = FLT_MAX;
        fScore[i] = FLT_MAX;
        cameFrom[i] = -1;
    }
    
    // Get start and goal nodes
    const Node* startNode = &graph->nodes[startId];
    const Node* goalNode = &graph->nodes[goalId];
    
    // Initialize start node
    gScore[startId] = 0.0f;
    float h = astar_heuristic(startNode, goalNode, cfg.heuristic) * cfg.heuristicWeight;
    fScore[startId] = h;
    
    // Create priority queue (open set)
    PriorityQueue* openSet = pq_create(nodeCount);
    if (!openSet) {
        free(gScore);
        free(fScore);
        free(cameFrom);
        free(inClosedSet);
        free(inOpenSet);
        return result;
    }
    
    pq_push(openSet, startId, fScore[startId]);
    inOpenSet[startId] = true;
    localStats.maxOpenSetSize = 1;
    
    // Main A* loop
    while (!pq_empty(openSet)) {
        int currentId;
        float currentFScore;
        pq_pop(openSet, &currentId, &currentFScore);
        inOpenSet[currentId] = false;
        
        localStats.nodesExplored++;
        
        // Check if we reached the goal
        if (currentId == goalId) {
            result = reconstruct_path(cameFrom, gScore, startId, goalId, nodeCount);
            localStats.nodesInOpenSet = openSet->size;
            break;
        }
        
        // Skip if already in closed set (can happen due to lazy deletion)
        if (inClosedSet[currentId]) continue;
        inClosedSet[currentId] = true;
        
        const Node* currentNode = &graph->nodes[currentId];
        
        // Explore neighbors
        for (int i = 0; i < graph->edgeCounts[currentId]; i++) {
            const Edge* edge = &graph->edges[currentId][i];
            if (!edge->active) continue;
            
            int neighborId = edge->to;
            if (!graph->nodes[neighborId].active) continue;
            if (inClosedSet[neighborId]) continue;
            
            // Calculate tentative g score
            float tentativeG = gScore[currentId] + edge->weight;
            
            if (tentativeG < gScore[neighborId]) {
                // This is a better path
                const Node* neighborNode = &graph->nodes[neighborId];
                
                cameFrom[neighborId] = currentId;
                gScore[neighborId] = tentativeG;
                h = astar_heuristic(neighborNode, goalNode, cfg.heuristic) * cfg.heuristicWeight;
                fScore[neighborId] = tentativeG + h;
                
                // Add to open set if not already there
                if (!inOpenSet[neighborId]) {
                    pq_push(openSet, neighborId, fScore[neighborId]);
                    inOpenSet[neighborId] = true;
                    
                    if (openSet->size > localStats.maxOpenSetSize) {
                        localStats.maxOpenSetSize = openSet->size;
                    }
                } else {
                    // Update priority
                    pq_decrease_priority(openSet, neighborId, fScore[neighborId]);
                }
            }
        }
    }
    
    // Record timing
    localStats.searchTimeMs = (float)(get_time_ms() - startTime);
    
    if (stats) {
        *stats = localStats;
    }
    
    // Cleanup
    pq_free(openSet);
    free(gScore);
    free(fScore);
    free(cameFrom);
    free(inClosedSet);
    free(inOpenSet);
    
    return result;
}

// Get exploration order for visualization
int astar_get_exploration_order(
    const Graph* graph,
    int startId,
    int goalId,
    int* explored,
    int maxNodes
) {
    if (!graph || !explored || startId < 0 || goalId < 0) return 0;
    
    int nodeCount = graph->nodeCount;
    float* gScore = (float*)malloc(nodeCount * sizeof(float));
    bool* inClosedSet = (bool*)calloc(nodeCount, sizeof(bool));
    bool* inOpenSet = (bool*)calloc(nodeCount, sizeof(bool));
    
    if (!gScore || !inClosedSet || !inOpenSet) {
        free(gScore);
        free(inClosedSet);
        free(inOpenSet);
        return 0;
    }
    
    for (int i = 0; i < nodeCount; i++) {
        gScore[i] = FLT_MAX;
    }
    gScore[startId] = 0.0f;
    
    const Node* goalNode = &graph->nodes[goalId];
    
    PriorityQueue* openSet = pq_create(nodeCount);
    float h = astar_heuristic(&graph->nodes[startId], goalNode, HEURISTIC_EUCLIDEAN);
    pq_push(openSet, startId, h);
    inOpenSet[startId] = true;
    
    int exploredCount = 0;
    
    while (!pq_empty(openSet) && exploredCount < maxNodes) {
        int currentId;
        float currentFScore;
        pq_pop(openSet, &currentId, &currentFScore);
        inOpenSet[currentId] = false;
        
        if (inClosedSet[currentId]) continue;
        inClosedSet[currentId] = true;
        
        explored[exploredCount++] = currentId;
        
        if (currentId == goalId) break;
        
        for (int i = 0; i < graph->edgeCounts[currentId]; i++) {
            const Edge* edge = &graph->edges[currentId][i];
            if (!edge->active) continue;
            
            int neighborId = edge->to;
            if (!graph->nodes[neighborId].active) continue;
            if (inClosedSet[neighborId]) continue;
            
            float tentativeG = gScore[currentId] + edge->weight;
            
            if (tentativeG < gScore[neighborId]) {
                gScore[neighborId] = tentativeG;
                h = astar_heuristic(&graph->nodes[neighborId], goalNode, HEURISTIC_EUCLIDEAN);
                float fScore = tentativeG + h;
                
                if (!inOpenSet[neighborId]) {
                    pq_push(openSet, neighborId, fScore);
                    inOpenSet[neighborId] = true;
                }
            }
        }
    }
    
    pq_free(openSet);
    free(gScore);
    free(inClosedSet);
    free(inOpenSet);
    
    return exploredCount;
}
