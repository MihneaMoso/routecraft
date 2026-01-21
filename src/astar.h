/**
 * astar.h - A* Pathfinding Algorithm
 * 
 * Implementation of the A* (A-Star) algorithm for finding the shortest path
 * in a weighted graph. A* uses a heuristic function (typically Euclidean distance)
 * to guide the search towards the goal, making it more efficient than Dijkstra's
 * algorithm for geographic pathfinding.
 * 
 * Time Complexity: O(E log V) where E = edges, V = vertices
 * Space Complexity: O(V)
 * 
 * The algorithm maintains two sets:
 * - Open Set: Nodes to be evaluated (implemented as a min-heap/priority queue)
 * - Closed Set: Nodes already evaluated
 * 
 * For each node, we track:
 * - g(n): Cost from start to current node
 * - h(n): Heuristic estimate from current to goal
 * - f(n) = g(n) + h(n): Total estimated cost
 */

#ifndef ASTAR_H
#define ASTAR_H

#include "graph.h"

#ifdef __cplusplus
extern "C" {
#endif

// Heuristic types
typedef enum {
    HEURISTIC_EUCLIDEAN,     // Standard straight-line distance
    HEURISTIC_MANHATTAN,     // Grid-based distance (|dx| + |dy|)
    HEURISTIC_CHEBYSHEV,     // Diagonal distance (max(|dx|, |dy|))
    HEURISTIC_ZERO           // Dijkstra's algorithm (no heuristic)
} HeuristicType;

// A* search statistics for visualization and analysis
typedef struct {
    int nodesExplored;       // Total nodes visited
    int nodesInOpenSet;      // Nodes still in open set when path found
    int maxOpenSetSize;      // Maximum size of open set during search
    float searchTimeMs;      // Time taken for search
} AStarStats;

// A* algorithm configuration
typedef struct {
    HeuristicType heuristic;
    float heuristicWeight;   // Weight for heuristic (1.0 = standard A*, >1 = greedy)
    bool allowDiagonal;      // Allow diagonal movement (for grid-based maps)
} AStarConfig;

// Default configuration
AStarConfig astar_default_config(void);

/**
 * Find the shortest path between two nodes using A* algorithm
 * 
 * @param graph     The graph to search
 * @param startId   Starting node ID
 * @param goalId    Goal node ID
 * @param config    Algorithm configuration (can be NULL for defaults)
 * @param stats     Output statistics (can be NULL if not needed)
 * @return          PathResult containing the path (call path_result_free when done)
 */
PathResult astar_find_path(
    const Graph* graph,
    int startId,
    int goalId,
    const AStarConfig* config,
    AStarStats* stats
);

/**
 * Calculate heuristic distance between two nodes
 * 
 * @param a          First node
 * @param b          Second node
 * @param type       Type of heuristic to use
 * @return           Estimated distance
 */
float astar_heuristic(const Node* a, const Node* b, HeuristicType type);

/**
 * Visualize the A* search process (for debugging/educational purposes)
 * Returns an array of node IDs in the order they were explored
 * 
 * @param graph     The graph to search
 * @param startId   Starting node ID
 * @param goalId    Goal node ID
 * @param explored  Output array for explored nodes (must be pre-allocated)
 * @param maxNodes  Size of the explored array
 * @return          Number of nodes in explored array
 */
int astar_get_exploration_order(
    const Graph* graph,
    int startId,
    int goalId,
    int* explored,
    int maxNodes
);

#ifdef __cplusplus
}
#endif

#endif // ASTAR_H
