/**
 * main.c - RouteCraft Application
 * 
 * A beautiful map application demonstrating the A* pathfinding algorithm.
 * This educational tool visualizes how heuristic search algorithms work
 * in real-world routing applications.
 * 
 * Features:
 * - Interactive graph/map editor
 * - Add locations (nodes) by clicking
 * - Connect locations with roads (edges)
 * - Find shortest path using A* algorithm
 * - Visual representation of the search process
 * - Save/load maps from disk
 */

#include "raylib.h"
#include "graph.h"
#include "astar.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Application constants
#define WINDOW_WIDTH    1280
#define WINDOW_HEIGHT   720
#define SIDEBAR_WIDTH   320
#define MAP_FILE        "map.rcg"

// Application modes
typedef enum {
    MODE_VIEW,          // Default viewing mode
    MODE_ADD_NODE,      // Adding new nodes
    MODE_ADD_EDGE,      // Adding edges between nodes
    MODE_DELETE,        // Deleting nodes/edges
    MODE_SEARCH         // Path search mode
} AppMode;

// Application state
typedef struct {
    Graph graph;
    AppMode mode;
    
    // Selection
    int hoveredNode;
    int selectedNode;
    int edgeStartNode;      // For edge creation
    int searchStartNode;    // For pathfinding
    int searchEndNode;
    
    // Path result
    PathResult currentPath;
    AStarStats pathStats;
    bool pathAnimating;
    float pathAnimProgress;
    
    // Exploration visualization
    int* exploredNodes;
    int exploredCount;
    float explorationAnimProgress;
    bool showExploration;
    
    // Camera/pan
    Vector2 offset;
    float zoom;
    bool panning;
    Vector2 panStart;
    Vector2 offsetStart;
    
    // UI elements
    InputField nodeNameInput;
    InputField searchFromInput;
    InputField searchToInput;
    Button addNodeBtn;
    Button addEdgeBtn;
    Button deleteBtn;
    Button searchBtn;
    Button clearPathBtn;
    Button saveBtn;
    Button loadBtn;
    Button generateSampleBtn;
    
    // Sidebar scroll
    float sidebarScroll;
} AppState;

// Global state
static AppState app = {0};

// Forward declarations
void app_init(void);
void app_cleanup(void);
void app_update(void);
void app_draw(void);
void app_draw_sidebar(void);
void app_draw_map(void);
void app_handle_map_input(void);
void app_perform_search(void);
void app_clear_path(void);
void app_generate_sample_map(void);
Vector2 world_to_screen(float x, float y);
Vector2 screen_to_world(float x, float y);

int main(void) {
    // Initialize window
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "RouteCraft - A* Pathfinding Visualizer");
    SetTargetFPS(60);
    
    // Initialize application
    app_init();
    
    // Main loop
    while (!WindowShouldClose()) {
        app_update();
        
        BeginDrawing();
        ClearBackground(UI_COLOR_BG);
        app_draw();
        EndDrawing();
    }
    
    // Cleanup
    app_cleanup();
    CloseWindow();
    
    return 0;
}

void app_init(void) {
    // Initialize graph
    graph_init(&app.graph);
    
    // Try to load existing map
    if (!graph_load(&app.graph, MAP_FILE)) {
        // Generate sample map if no saved map exists
        app_generate_sample_map();
    }
    
    // Initialize state
    app.mode = MODE_VIEW;
    app.hoveredNode = -1;
    app.selectedNode = -1;
    app.edgeStartNode = -1;
    app.searchStartNode = -1;
    app.searchEndNode = -1;
    app.currentPath = path_result_create();
    app.exploredNodes = (int*)malloc(MAX_NODES * sizeof(int));
    app.exploredCount = 0;
    
    // Camera
    app.offset = (Vector2){0, 0};
    app.zoom = 1.0f;
    app.panning = false;
    
    // Initialize UI
    ui_init();
    
    float y = 60;
    float btnWidth = SIDEBAR_WIDTH - 40;
    
    // Input fields
    ui_input_init(&app.nodeNameInput, 20, y, btnWidth, 40, "Location name...");
    y += 50;
    
    // Mode buttons
    ui_button_init(&app.addNodeBtn, 20, y, btnWidth, 40, "➕ Add Location", UI_COLOR_PRIMARY);
    y += 50;
    ui_button_init(&app.addEdgeBtn, 20, y, btnWidth, 40, "🔗 Connect Locations", UI_COLOR_PRIMARY);
    y += 50;
    ui_button_init(&app.deleteBtn, 20, y, btnWidth, 40, "🗑️ Delete", UI_COLOR_DANGER);
    y += 70;
    
    // Search section
    ui_input_init(&app.searchFromInput, 20, y, btnWidth, 40, "From location...");
    y += 50;
    ui_input_init(&app.searchToInput, 20, y, btnWidth, 40, "To location...");
    y += 50;
    ui_button_init(&app.searchBtn, 20, y, btnWidth, 40, "🔍 Find Route", UI_COLOR_SECONDARY);
    y += 50;
    ui_button_init(&app.clearPathBtn, 20, y, btnWidth, 40, "Clear Path", UI_COLOR_BG_LIGHTER);
    y += 70;
    
    // File operations
    ui_button_init(&app.saveBtn, 20, y, (btnWidth - 10) / 2, 40, "💾 Save", UI_COLOR_BG_LIGHTER);
    ui_button_init(&app.loadBtn, 20 + (btnWidth + 10) / 2, y, (btnWidth - 10) / 2, 40, "📂 Load", UI_COLOR_BG_LIGHTER);
    y += 50;
    
    ui_button_init(&app.generateSampleBtn, 20, y, btnWidth, 40, "🗺️ Generate Sample", UI_COLOR_ACCENT);
}

void app_cleanup(void) {
    path_result_free(&app.currentPath);
    free(app.exploredNodes);
    graph_free(&app.graph);
    ui_cleanup();
}

void app_update(void) {
    float dt = GetFrameTime();
    
    // Update notifications
    ui_notifications_update(dt);
    
    // Update path animation
    if (app.pathAnimating) {
        app.pathAnimProgress += dt * 2.0f;
        if (app.pathAnimProgress >= (float)app.currentPath.length) {
            app.pathAnimating = false;
            app.pathAnimProgress = (float)app.currentPath.length;
        }
    }
    
    // Update exploration animation
    if (app.showExploration && app.explorationAnimProgress < (float)app.exploredCount) {
        app.explorationAnimProgress += dt * 30.0f;  // Show 30 nodes per second
    }
    
    // Update buttons
    if (ui_button_update(&app.addNodeBtn)) {
        app.mode = (app.mode == MODE_ADD_NODE) ? MODE_VIEW : MODE_ADD_NODE;
        app.edgeStartNode = -1;
        ui_notify(app.mode == MODE_ADD_NODE ? "Click on the map to add a location" : "Returned to view mode", NOTIFY_INFO);
    }
    
    if (ui_button_update(&app.addEdgeBtn)) {
        app.mode = (app.mode == MODE_ADD_EDGE) ? MODE_VIEW : MODE_ADD_EDGE;
        app.edgeStartNode = -1;
        ui_notify(app.mode == MODE_ADD_EDGE ? "Click two locations to connect them" : "Returned to view mode", NOTIFY_INFO);
    }
    
    if (ui_button_update(&app.deleteBtn)) {
        app.mode = (app.mode == MODE_DELETE) ? MODE_VIEW : MODE_DELETE;
        ui_notify(app.mode == MODE_DELETE ? "Click a location or edge to delete it" : "Returned to view mode", NOTIFY_INFO);
    }
    
    if (ui_button_update(&app.searchBtn)) {
        app_perform_search();
    }
    
    if (ui_button_update(&app.clearPathBtn)) {
        app_clear_path();
    }
    
    if (ui_button_update(&app.saveBtn)) {
        if (graph_save(&app.graph, MAP_FILE)) {
            ui_notify("Map saved successfully!", NOTIFY_SUCCESS);
        } else {
            ui_notify("Failed to save map", NOTIFY_ERROR);
        }
    }
    
    if (ui_button_update(&app.loadBtn)) {
        if (graph_load(&app.graph, MAP_FILE)) {
            app_clear_path();
            ui_notify("Map loaded successfully!", NOTIFY_SUCCESS);
        } else {
            ui_notify("No saved map found", NOTIFY_WARNING);
        }
    }
    
    if (ui_button_update(&app.generateSampleBtn)) {
        app_generate_sample_map();
        app_clear_path();
        ui_notify("Sample map generated!", NOTIFY_SUCCESS);
    }
    
    // Update input fields
    if (ui_input_update(&app.nodeNameInput)) {
        // Enter pressed in name field - could add node
    }
    ui_input_update(&app.searchFromInput);
    ui_input_update(&app.searchToInput);
    
    // Handle map input
    app_handle_map_input();
}

void app_handle_map_input(void) {
    Vector2 mouse = GetMousePosition();
    
    // Only handle map input if mouse is in map area
    if (mouse.x < SIDEBAR_WIDTH) return;
    
    Vector2 worldPos = screen_to_world(mouse.x, mouse.y);
    
    // Update hovered node
    app.hoveredNode = graph_find_node_at_position(&app.graph, worldPos.x, worldPos.y, 
                                                   UI_NODE_RADIUS * 2.0f / app.zoom);
    
    // Zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        float oldZoom = app.zoom;
        app.zoom += wheel * 0.1f;
        if (app.zoom < 0.2f) app.zoom = 0.2f;
        if (app.zoom > 3.0f) app.zoom = 3.0f;
        
        // Zoom towards mouse position
        Vector2 mouseWorld = screen_to_world(mouse.x, mouse.y);
        app.offset.x += (mouseWorld.x - worldPos.x) * (app.zoom - oldZoom);
        app.offset.y += (mouseWorld.y - worldPos.y) * (app.zoom - oldZoom);
    }
    
    // Panning with right mouse button
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        app.panning = true;
        app.panStart = mouse;
        app.offsetStart = app.offset;
    }
    if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
        app.panning = false;
    }
    if (app.panning) {
        app.offset.x = app.offsetStart.x + (mouse.x - app.panStart.x) / app.zoom;
        app.offset.y = app.offsetStart.y + (mouse.y - app.panStart.y) / app.zoom;
    }
    
    // Left click actions
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        switch (app.mode) {
            case MODE_VIEW:
                app.selectedNode = app.hoveredNode;
                if (app.selectedNode >= 0) {
                    Node* node = graph_get_node(&app.graph, app.selectedNode);
                    if (node) {
                        ui_input_set_text(&app.nodeNameInput, node->name);
                    }
                }
                break;
                
            case MODE_ADD_NODE: {
                const char* name = strlen(app.nodeNameInput.text) > 0 ? 
                                   app.nodeNameInput.text : "Location";
                char nodeName[MAX_NAME_LENGTH];
                snprintf(nodeName, sizeof(nodeName), "%s %d", name, app.graph.nodeCount + 1);
                
                int id = graph_add_node(&app.graph, nodeName, worldPos.x, worldPos.y);
                if (id >= 0) {
                    ui_notify("Location added!", NOTIFY_SUCCESS);
                    ui_input_clear(&app.nodeNameInput);
                }
                break;
            }
            
            case MODE_ADD_EDGE:
                if (app.hoveredNode >= 0) {
                    if (app.edgeStartNode < 0) {
                        app.edgeStartNode = app.hoveredNode;
                        ui_notify("Now click the destination location", NOTIFY_INFO);
                    } else if (app.edgeStartNode != app.hoveredNode) {
                        // Calculate distance as weight
                        Node* from = graph_get_node(&app.graph, app.edgeStartNode);
                        Node* to = graph_get_node(&app.graph, app.hoveredNode);
                        float distance = graph_calculate_distance(from, to);
                        
                        if (graph_add_edge_bidirectional(&app.graph, app.edgeStartNode, app.hoveredNode, distance)) {
                            ui_notify("Road created!", NOTIFY_SUCCESS);
                        } else {
                            ui_notify("Road already exists", NOTIFY_WARNING);
                        }
                        app.edgeStartNode = -1;
                    }
                }
                break;
                
            case MODE_DELETE:
                if (app.hoveredNode >= 0) {
                    graph_remove_node(&app.graph, app.hoveredNode);
                    ui_notify("Location deleted", NOTIFY_INFO);
                    app_clear_path();
                }
                break;
                
            case MODE_SEARCH:
                // Select start/end for search
                break;
        }
    }
    
    // ESC to cancel current mode
    if (IsKeyPressed(KEY_ESCAPE)) {
        app.mode = MODE_VIEW;
        app.edgeStartNode = -1;
        app.selectedNode = -1;
    }
}

void app_perform_search(void) {
    const char* fromName = app.searchFromInput.text;
    const char* toName = app.searchToInput.text;
    
    if (strlen(fromName) == 0 || strlen(toName) == 0) {
        ui_notify("Please enter both From and To locations", NOTIFY_WARNING);
        return;
    }
    
    int fromId = graph_find_node_by_name(&app.graph, fromName);
    int toId = graph_find_node_by_name(&app.graph, toName);
    
    if (fromId < 0) {
        ui_notify("Origin location not found", NOTIFY_ERROR);
        return;
    }
    if (toId < 0) {
        ui_notify("Destination location not found", NOTIFY_ERROR);
        return;
    }
    
    // Clear previous path
    app_clear_path();
    
    // Get exploration order for visualization
    app.exploredCount = astar_get_exploration_order(&app.graph, fromId, toId, 
                                                     app.exploredNodes, MAX_NODES);
    app.explorationAnimProgress = 0.0f;
    app.showExploration = true;
    
    // Find path
    AStarConfig config = astar_default_config();
    app.currentPath = astar_find_path(&app.graph, fromId, toId, &config, &app.pathStats);
    
    if (app.currentPath.found) {
        app.searchStartNode = fromId;
        app.searchEndNode = toId;
        app.pathAnimating = true;
        app.pathAnimProgress = 0.0f;
        
        char msg[128];
        snprintf(msg, sizeof(msg), "Route found! Distance: %.1f, Nodes explored: %d", 
                 app.currentPath.totalCost, app.pathStats.nodesExplored);
        ui_notify(msg, NOTIFY_SUCCESS);
    } else {
        ui_notify("No route found between these locations", NOTIFY_ERROR);
    }
}

void app_clear_path(void) {
    path_result_free(&app.currentPath);
    app.currentPath = path_result_create();
    app.searchStartNode = -1;
    app.searchEndNode = -1;
    app.pathAnimating = false;
    app.pathAnimProgress = 0.0f;
    app.exploredCount = 0;
    app.explorationAnimProgress = 0.0f;
    app.showExploration = false;
}

void app_generate_sample_map(void) {
    graph_init(&app.graph);
    
    // Create a sample city-like map
    // Central area
    int downtown = graph_add_node(&app.graph, "Downtown", 600, 360);
    int centralPark = graph_add_node(&app.graph, "Central Park", 700, 300);
    int mainStation = graph_add_node(&app.graph, "Main Station", 550, 420);
    int cityHall = graph_add_node(&app.graph, "City Hall", 650, 380);
    
    // North area
    int northGate = graph_add_node(&app.graph, "North Gate", 620, 180);
    int university = graph_add_node(&app.graph, "University", 720, 200);
    int museum = graph_add_node(&app.graph, "Museum", 550, 220);
    
    // South area
    int southMall = graph_add_node(&app.graph, "South Mall", 600, 520);
    int airport = graph_add_node(&app.graph, "Airport", 750, 550);
    int harbor = graph_add_node(&app.graph, "Harbor", 480, 550);
    
    // East area  
    int eastTech = graph_add_node(&app.graph, "Tech Park", 850, 350);
    int stadium = graph_add_node(&app.graph, "Stadium", 880, 450);
    int beach = graph_add_node(&app.graph, "Beach", 920, 300);
    
    // West area
    int westGardens = graph_add_node(&app.graph, "West Gardens", 400, 350);
    int hospital = graph_add_node(&app.graph, "Hospital", 380, 280);
    int industrial = graph_add_node(&app.graph, "Industrial Zone", 350, 450);
    
    // Helper macro for bidirectional edges
    #define CONNECT(a, b) { \
        Node* na = graph_get_node(&app.graph, a); \
        Node* nb = graph_get_node(&app.graph, b); \
        float d = graph_calculate_distance(na, nb); \
        graph_add_edge_bidirectional(&app.graph, a, b, d); \
    }
    
    // Central connections
    CONNECT(downtown, centralPark);
    CONNECT(downtown, mainStation);
    CONNECT(downtown, cityHall);
    CONNECT(centralPark, cityHall);
    CONNECT(mainStation, cityHall);
    
    // North connections
    CONNECT(centralPark, northGate);
    CONNECT(centralPark, university);
    CONNECT(northGate, museum);
    CONNECT(northGate, university);
    CONNECT(museum, hospital);
    
    // South connections
    CONNECT(mainStation, southMall);
    CONNECT(southMall, airport);
    CONNECT(southMall, harbor);
    CONNECT(airport, stadium);
    CONNECT(harbor, industrial);
    
    // East connections
    CONNECT(centralPark, eastTech);
    CONNECT(eastTech, beach);
    CONNECT(eastTech, stadium);
    CONNECT(university, beach);
    
    // West connections
    CONNECT(downtown, westGardens);
    CONNECT(westGardens, hospital);
    CONNECT(westGardens, industrial);
    CONNECT(mainStation, industrial);
    
    // Cross connections for more routing options
    CONNECT(museum, downtown);
    CONNECT(cityHall, southMall);
    CONNECT(harbor, mainStation);
    
    #undef CONNECT
}

Vector2 world_to_screen(float x, float y) {
    return (Vector2){
        SIDEBAR_WIDTH + (x + app.offset.x) * app.zoom,
        (y + app.offset.y) * app.zoom
    };
}

Vector2 screen_to_world(float x, float y) {
    return (Vector2){
        (x - SIDEBAR_WIDTH) / app.zoom - app.offset.x,
        y / app.zoom - app.offset.y
    };
}

void app_draw(void) {
    // Draw map first
    app_draw_map();
    
    // Draw sidebar
    app_draw_sidebar();
    
    // Draw notifications on top
    ui_notifications_draw();
    
    // Draw mode indicator
    const char* modeText = "";
    Color modeColor = UI_COLOR_TEXT_DIM;
    switch (app.mode) {
        case MODE_ADD_NODE: modeText = "MODE: Add Location"; modeColor = UI_COLOR_PRIMARY; break;
        case MODE_ADD_EDGE: modeText = "MODE: Connect Locations"; modeColor = UI_COLOR_PRIMARY; break;
        case MODE_DELETE: modeText = "MODE: Delete"; modeColor = UI_COLOR_DANGER; break;
        default: modeText = "MODE: View"; break;
    }
    DrawText(modeText, SIDEBAR_WIDTH + 20, WINDOW_HEIGHT - 30, UI_FONT_SIZE_SMALL, modeColor);
    
    // Draw zoom level
    char zoomText[32];
    snprintf(zoomText, sizeof(zoomText), "Zoom: %.0f%%", app.zoom * 100);
    DrawText(zoomText, WINDOW_WIDTH - 100, WINDOW_HEIGHT - 30, UI_FONT_SIZE_SMALL, UI_COLOR_TEXT_DIM);
}

void app_draw_sidebar(void) {
    // Sidebar background
    DrawRectangle(0, 0, SIDEBAR_WIDTH, WINDOW_HEIGHT, UI_COLOR_BG_LIGHT);
    DrawLineEx((Vector2){SIDEBAR_WIDTH, 0}, (Vector2){SIDEBAR_WIDTH, WINDOW_HEIGHT}, 2, UI_COLOR_BORDER);
    
    // Title
    DrawText("🗺️ RouteCraft", 20, 15, UI_FONT_SIZE_TITLE, UI_COLOR_TEXT);
    
    // Locations section
    DrawText("📍 Add Location", 20, 50, UI_FONT_SIZE_SMALL, UI_COLOR_TEXT_DIM);
    ui_input_draw(&app.nodeNameInput);
    ui_button_draw(&app.addNodeBtn);
    ui_button_draw(&app.addEdgeBtn);
    ui_button_draw(&app.deleteBtn);
    
    // Search section
    DrawText("🔍 Find Route", 20, 210, UI_FONT_SIZE_SMALL, UI_COLOR_TEXT_DIM);
    ui_input_draw(&app.searchFromInput);
    ui_input_draw(&app.searchToInput);
    ui_button_draw(&app.searchBtn);
    ui_button_draw(&app.clearPathBtn);
    
    // Path info
    if (app.currentPath.found) {
        int y = 400;
        DrawText("Route Info", 20, y, UI_FONT_SIZE_SMALL, UI_COLOR_TEXT_DIM);
        y += 25;
        
        char info[128];
        snprintf(info, sizeof(info), "Distance: %.1f units", app.currentPath.totalCost);
        DrawText(info, 20, y, UI_FONT_SIZE_SMALL, UI_COLOR_SECONDARY);
        y += 20;
        
        snprintf(info, sizeof(info), "Stops: %d", app.currentPath.length);
        DrawText(info, 20, y, UI_FONT_SIZE_SMALL, UI_COLOR_TEXT);
        y += 20;
        
        snprintf(info, sizeof(info), "Nodes explored: %d", app.pathStats.nodesExplored);
        DrawText(info, 20, y, UI_FONT_SIZE_SMALL, UI_COLOR_TEXT);
        y += 20;
        
        snprintf(info, sizeof(info), "Search time: %.2f ms", app.pathStats.searchTimeMs);
        DrawText(info, 20, y, UI_FONT_SIZE_SMALL, UI_COLOR_TEXT);
    }
    
    // File operations
    DrawText("💾 Save/Load", 20, 510, UI_FONT_SIZE_SMALL, UI_COLOR_TEXT_DIM);
    ui_button_draw(&app.saveBtn);
    ui_button_draw(&app.loadBtn);
    ui_button_draw(&app.generateSampleBtn);
    
    // Stats at bottom
    char statsText[64];
    snprintf(statsText, sizeof(statsText), "Locations: %d", app.graph.nodeCount);
    DrawText(statsText, 20, WINDOW_HEIGHT - 60, UI_FONT_SIZE_SMALL, UI_COLOR_TEXT_DIM);
    
    // Instructions
    DrawText("RMB: Pan | Scroll: Zoom", 20, WINDOW_HEIGHT - 35, UI_FONT_SIZE_SMALL, UI_COLOR_TEXT_DIM);
}

void app_draw_map(void) {
    // Set scissor to map area
    BeginScissorMode(SIDEBAR_WIDTH, 0, WINDOW_WIDTH - SIDEBAR_WIDTH, WINDOW_HEIGHT);
    
    // Draw grid
    Color gridColor = (Color){40, 40, 55, 255};
    float gridSize = 50 * app.zoom;
    float offsetX = fmodf(app.offset.x * app.zoom, gridSize);
    float offsetY = fmodf(app.offset.y * app.zoom, gridSize);
    
    for (float x = SIDEBAR_WIDTH + offsetX; x < WINDOW_WIDTH; x += gridSize) {
        DrawLineV((Vector2){x, 0}, (Vector2){x, WINDOW_HEIGHT}, gridColor);
    }
    for (float y = offsetY; y < WINDOW_HEIGHT; y += gridSize) {
        DrawLineV((Vector2){SIDEBAR_WIDTH, y}, (Vector2){WINDOW_WIDTH, y}, gridColor);
    }
    
    // Draw explored nodes (A* visualization)
    if (app.showExploration && app.exploredCount > 0) {
        int nodesToShow = (int)app.explorationAnimProgress;
        if (nodesToShow > app.exploredCount) nodesToShow = app.exploredCount;
        
        for (int i = 0; i < nodesToShow; i++) {
            Node* node = graph_get_node(&app.graph, app.exploredNodes[i]);
            if (!node) continue;
            
            Vector2 pos = world_to_screen(node->x, node->y);
            float alpha = 0.3f - (float)i / (float)app.exploredCount * 0.2f;
            Color c = UI_COLOR_EXPLORED;
            c.a = (unsigned char)(255 * alpha);
            DrawCircle((int)pos.x, (int)pos.y, UI_NODE_RADIUS * 2.5f * app.zoom, c);
        }
    }
    
    // Draw edges
    for (int i = 0; i < app.graph.nodeCount; i++) {
        Node* from = graph_get_node(&app.graph, i);
        if (!from) continue;
        
        for (int j = 0; j < app.graph.edgeCounts[i]; j++) {
            Edge* edge = &app.graph.edges[i][j];
            if (!edge->active) continue;
            
            Node* to = graph_get_node(&app.graph, edge->to);
            if (!to) continue;
            
            // Only draw edge once (when from < to for bidirectional)
            if (edge->to < i && graph_has_edge(&app.graph, edge->to, i)) continue;
            
            Vector2 p1 = world_to_screen(from->x, from->y);
            Vector2 p2 = world_to_screen(to->x, to->y);
            
            float thickness = 2.0f * app.zoom;
            Color color = UI_COLOR_EDGE;
            
            // Highlight edge if part of path
            if (app.currentPath.found) {
                for (int k = 0; k < app.currentPath.length - 1; k++) {
                    if ((app.currentPath.nodes[k] == i && app.currentPath.nodes[k+1] == edge->to) ||
                        (app.currentPath.nodes[k] == edge->to && app.currentPath.nodes[k+1] == i)) {
                        // Skip, will be drawn as path
                        break;
                    }
                }
            }
            
            ui_draw_edge(p1.x, p1.y, p2.x, p2.y, thickness, color);
            
            // Draw distance label
            if (app.zoom > 0.6f) {
                char distLabel[16];
                snprintf(distLabel, sizeof(distLabel), "%.0f", edge->weight);
                Vector2 mid = {(p1.x + p2.x) / 2, (p1.y + p2.y) / 2};
                int textW = MeasureText(distLabel, UI_FONT_SIZE_SMALL);
                DrawRectangle((int)(mid.x - textW/2 - 4), (int)(mid.y - 8), textW + 8, 16, UI_COLOR_BG);
                DrawText(distLabel, (int)(mid.x - textW/2), (int)(mid.y - 6), UI_FONT_SIZE_SMALL, UI_COLOR_TEXT_DIM);
            }
        }
    }
    
    // Draw path
    if (app.currentPath.found && app.currentPath.length > 1) {
        for (int i = 0; i < app.currentPath.length - 1; i++) {
            float segmentProgress = app.pathAnimProgress - (float)i;
            if (segmentProgress <= 0.0f) break;
            if (segmentProgress > 1.0f) segmentProgress = 1.0f;
            
            Node* from = graph_get_node(&app.graph, app.currentPath.nodes[i]);
            Node* to = graph_get_node(&app.graph, app.currentPath.nodes[i + 1]);
            if (!from || !to) continue;
            
            Vector2 p1 = world_to_screen(from->x, from->y);
            Vector2 p2 = world_to_screen(to->x, to->y);
            
            ui_draw_path_segment(p1.x, p1.y, p2.x, p2.y, 4.0f * app.zoom, UI_COLOR_PATH, segmentProgress);
        }
    }
    
    // Draw edge being created
    if (app.mode == MODE_ADD_EDGE && app.edgeStartNode >= 0) {
        Node* from = graph_get_node(&app.graph, app.edgeStartNode);
        if (from) {
            Vector2 p1 = world_to_screen(from->x, from->y);
            Vector2 mouse = GetMousePosition();
            DrawLineEx(p1, mouse, 2.0f, UI_COLOR_PRIMARY);
        }
    }
    
    // Draw nodes
    for (int i = 0; i < app.graph.nodeCount; i++) {
        Node* node = graph_get_node(&app.graph, i);
        if (!node) continue;
        
        Vector2 pos = world_to_screen(node->x, node->y);
        
        bool isHovered = (i == app.hoveredNode);
        bool isSelected = (i == app.selectedNode);
        bool isPathNode = false;
        bool isStart = (i == app.searchStartNode);
        bool isEnd = (i == app.searchEndNode);
        
        // Check if node is in path
        if (app.currentPath.found) {
            for (int j = 0; j < app.currentPath.length; j++) {
                if (app.currentPath.nodes[j] == i) {
                    isPathNode = true;
                    break;
                }
            }
        }
        
        Color nodeColor = UI_COLOR_NODE;
        if (isStart) nodeColor = UI_COLOR_SECONDARY;
        else if (isEnd) nodeColor = UI_COLOR_DANGER;
        else if (isPathNode) nodeColor = UI_COLOR_PATH;
        else if (isSelected) nodeColor = UI_COLOR_NODE_SELECTED;
        
        float radius = UI_NODE_RADIUS * app.zoom;
        ui_draw_node(pos.x, pos.y, radius, nodeColor, isSelected || isPathNode, isHovered);
        
        // Draw node name
        if (app.zoom > 0.5f) {
            int textW = MeasureText(node->name, UI_FONT_SIZE_SMALL);
            float textX = pos.x - textW / 2;
            float textY = pos.y + radius + 8;
            
            // Background for text
            DrawRectangle((int)(textX - 4), (int)(textY - 2), textW + 8, UI_FONT_SIZE_SMALL + 4, 
                         (Color){24, 24, 32, 200});
            DrawText(node->name, (int)textX, (int)textY, UI_FONT_SIZE_SMALL, UI_COLOR_TEXT);
        }
    }
    
    EndScissorMode();
}
