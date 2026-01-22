/**
 * unity.c - Unity Build File for RouteCraft
 * 
 * This file includes all source files for single-unit compilation.
 * Unity builds provide faster compilation and better optimization.
 * 
 * Usage: 
 *   make unity
 * 
 * Or manually:
 *   gcc -O2 -I../src src/unity_build.c -o routecraft -lraylib [platform libs]
 */

// Core modules
#include "graph.c"
#include "astar.c"

// UI components  
#include "ui.c"

// Main application (must be last as it contains main())
#include "main.c"
