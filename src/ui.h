/**
 * ui.h - User Interface Components
 * 
 * Beautiful and expressive UI components for the RouteCraft application.
 * Uses Raylib for rendering with a modern, flat design aesthetic.
 */

#ifndef UI_H
#define UI_H

#include "raylib.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Color scheme (Modern dark theme)
#define UI_COLOR_BG             (Color){24, 24, 32, 255}
#define UI_COLOR_BG_LIGHT       (Color){36, 36, 48, 255}
#define UI_COLOR_BG_LIGHTER     (Color){48, 48, 64, 255}
#define UI_COLOR_PRIMARY        (Color){99, 102, 241, 255}    // Indigo
#define UI_COLOR_PRIMARY_HOVER  (Color){129, 132, 255, 255}
#define UI_COLOR_SECONDARY      (Color){16, 185, 129, 255}    // Emerald
#define UI_COLOR_ACCENT         (Color){245, 158, 11, 255}    // Amber
#define UI_COLOR_DANGER         (Color){239, 68, 68, 255}     // Red
#define UI_COLOR_TEXT           (Color){248, 250, 252, 255}
#define UI_COLOR_TEXT_DIM       (Color){148, 163, 184, 255}
#define UI_COLOR_BORDER         (Color){71, 85, 105, 255}
#define UI_COLOR_PATH           (Color){34, 197, 94, 255}     // Green
#define UI_COLOR_NODE           (Color){59, 130, 246, 255}    // Blue
#define UI_COLOR_NODE_SELECTED  (Color){251, 191, 36, 255}    // Yellow
#define UI_COLOR_EDGE           (Color){100, 116, 139, 255}   // Slate
#define UI_COLOR_EXPLORED       (Color){147, 51, 234, 128}    // Purple (transparent)

// UI Constants
#define UI_FONT_SIZE_SMALL      14
#define UI_FONT_SIZE_NORMAL     18
#define UI_FONT_SIZE_LARGE      24
#define UI_FONT_SIZE_TITLE      32
#define UI_PADDING              12
#define UI_BORDER_RADIUS        8
#define UI_NODE_RADIUS          12
#define UI_NODE_RADIUS_HOVER    16
#define UI_ANIMATION_SPEED      0.15f

// Input field state
typedef struct {
    char text[256];
    int cursor;
    bool focused;
    bool active;
    float cursorBlink;
    Rectangle bounds;
    const char* placeholder;
} InputField;

// Button state
typedef struct {
    Rectangle bounds;
    const char* label;
    bool hovered;
    bool pressed;
    bool disabled;
    Color color;
    float hoverAnim;
} Button;

// Panel state
typedef struct {
    Rectangle bounds;
    const char* title;
    bool collapsed;
    float collapseAnim;
} Panel;

// Notification system
typedef enum {
    NOTIFY_INFO,
    NOTIFY_SUCCESS,
    NOTIFY_WARNING,
    NOTIFY_ERROR
} NotifyType;

typedef struct {
    char message[256];
    NotifyType type;
    float timer;
    float alpha;
    bool active;
} Notification;

// Toast notifications queue
#define MAX_NOTIFICATIONS 5
typedef struct {
    Notification items[MAX_NOTIFICATIONS];
    int count;
} NotificationQueue;

// Animation helper
typedef struct {
    float current;
    float target;
    float speed;
} AnimValue;

// Initialize UI
void ui_init(void);
void ui_cleanup(void);

// Input field operations
void ui_input_init(InputField* field, float x, float y, float width, float height, const char* placeholder);
bool ui_input_update(InputField* field);  // Returns true if Enter was pressed
void ui_input_draw(const InputField* field);
void ui_input_clear(InputField* field);
void ui_input_set_text(InputField* field, const char* text);

// Button operations
void ui_button_init(Button* button, float x, float y, float width, float height, const char* label, Color color);
bool ui_button_update(Button* button);  // Returns true if clicked
void ui_button_draw(const Button* button);

// Panel operations
void ui_panel_init(Panel* panel, float x, float y, float width, float height, const char* title);
void ui_panel_update(Panel* panel);
void ui_panel_draw_begin(const Panel* panel);
void ui_panel_draw_end(void);

// Notification system
void ui_notify(const char* message, NotifyType type);
void ui_notifications_update(float deltaTime);
void ui_notifications_draw(void);

// Drawing helpers
void ui_draw_rounded_rect(Rectangle rect, float radius, Color color);
void ui_draw_rounded_rect_outline(Rectangle rect, float radius, float thickness, Color color);
void ui_draw_shadow(Rectangle rect, float radius, float blur, Color color);
void ui_draw_text_centered(const char* text, Rectangle rect, int fontSize, Color color);
void ui_draw_node(float x, float y, float radius, Color color, bool selected, bool hovered);
void ui_draw_edge(float x1, float y1, float x2, float y2, float thickness, Color color);
void ui_draw_path_segment(float x1, float y1, float x2, float y2, float thickness, Color color, float progress);
void ui_draw_arrow(float x1, float y1, float x2, float y2, float size, Color color);

// Animation helpers
void ui_anim_update(AnimValue* anim, float deltaTime);
float ui_ease_out_cubic(float t);
float ui_ease_in_out_cubic(float t);

// Utility
bool ui_point_in_rect(Vector2 point, Rectangle rect);
Color ui_color_lerp(Color a, Color b, float t);

#ifdef __cplusplus
}
#endif

#endif // UI_H
