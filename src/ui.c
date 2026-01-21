/**
 * ui.c - User Interface Implementation
 */

#include "ui.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// Global notification queue
static NotificationQueue notificationQueue = {0};

// Global font (if using custom font)
static Font uiFont = {0};
static bool fontLoaded = false;

void ui_init(void) {
    memset(&notificationQueue, 0, sizeof(notificationQueue));
    // Could load custom font here
    // uiFont = LoadFontEx("assets/font.ttf", 32, NULL, 0);
    // fontLoaded = true;
}

void ui_cleanup(void) {
    if (fontLoaded) {
        UnloadFont(uiFont);
        fontLoaded = false;
    }
}

// ============ Input Field ============

void ui_input_init(InputField* field, float x, float y, float width, float height, const char* placeholder) {
    if (!field) return;
    memset(field->text, 0, sizeof(field->text));
    field->cursor = 0;
    field->focused = false;
    field->active = true;
    field->cursorBlink = 0.0f;
    field->bounds = (Rectangle){x, y, width, height};
    field->placeholder = placeholder;
}

bool ui_input_update(InputField* field) {
    if (!field || !field->active) return false;
    
    Vector2 mouse = GetMousePosition();
    bool mouseOver = ui_point_in_rect(mouse, field->bounds);
    
    // Handle focus
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        field->focused = mouseOver;
    }
    
    if (!field->focused) return false;
    
    // Cursor blink
    field->cursorBlink += GetFrameTime();
    if (field->cursorBlink > 1.0f) field->cursorBlink = 0.0f;
    
    // Handle text input
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 126 && field->cursor < 255) {
            // Shift characters right
            for (int i = strlen(field->text); i >= field->cursor; i--) {
                field->text[i + 1] = field->text[i];
            }
            field->text[field->cursor] = (char)key;
            field->cursor++;
        }
        key = GetCharPressed();
    }
    
    // Handle backspace
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        if (field->cursor > 0) {
            for (int i = field->cursor - 1; i < (int)strlen(field->text); i++) {
                field->text[i] = field->text[i + 1];
            }
            field->cursor--;
        }
    }
    
    // Handle delete
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) {
        int len = strlen(field->text);
        if (field->cursor < len) {
            for (int i = field->cursor; i < len; i++) {
                field->text[i] = field->text[i + 1];
            }
        }
    }
    
    // Handle arrow keys
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        if (field->cursor > 0) field->cursor--;
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
        if (field->cursor < (int)strlen(field->text)) field->cursor++;
    }
    
    // Handle home/end
    if (IsKeyPressed(KEY_HOME)) field->cursor = 0;
    if (IsKeyPressed(KEY_END)) field->cursor = strlen(field->text);
    
    // Check for enter
    return IsKeyPressed(KEY_ENTER);
}

void ui_input_draw(const InputField* field) {
    if (!field) return;
    
    Color bgColor = field->focused ? UI_COLOR_BG_LIGHTER : UI_COLOR_BG_LIGHT;
    Color borderColor = field->focused ? UI_COLOR_PRIMARY : UI_COLOR_BORDER;
    
    // Draw background
    ui_draw_rounded_rect(field->bounds, UI_BORDER_RADIUS, bgColor);
    ui_draw_rounded_rect_outline(field->bounds, UI_BORDER_RADIUS, 2.0f, borderColor);
    
    // Draw text or placeholder
    Rectangle textArea = {
        field->bounds.x + UI_PADDING,
        field->bounds.y,
        field->bounds.width - UI_PADDING * 2,
        field->bounds.height
    };
    
    if (strlen(field->text) > 0) {
        // Draw text
        Vector2 textPos = {textArea.x, textArea.y + (textArea.height - UI_FONT_SIZE_NORMAL) / 2};
        DrawText(field->text, (int)textPos.x, (int)textPos.y, UI_FONT_SIZE_NORMAL, UI_COLOR_TEXT);
        
        // Draw cursor
        if (field->focused && field->cursorBlink < 0.5f) {
            int cursorX = textPos.x + MeasureText(field->text, UI_FONT_SIZE_NORMAL);
            if (field->cursor < (int)strlen(field->text)) {
                char temp[256];
                strncpy(temp, field->text, field->cursor);
                temp[field->cursor] = '\0';
                cursorX = textPos.x + MeasureText(temp, UI_FONT_SIZE_NORMAL);
            }
            DrawRectangle(cursorX, textPos.y, 2, UI_FONT_SIZE_NORMAL, UI_COLOR_PRIMARY);
        }
    } else if (field->placeholder) {
        // Draw placeholder
        Vector2 textPos = {textArea.x, textArea.y + (textArea.height - UI_FONT_SIZE_NORMAL) / 2};
        DrawText(field->placeholder, (int)textPos.x, (int)textPos.y, UI_FONT_SIZE_NORMAL, UI_COLOR_TEXT_DIM);
        
        // Draw cursor if focused
        if (field->focused && field->cursorBlink < 0.5f) {
            DrawRectangle((int)textPos.x, (int)textPos.y, 2, UI_FONT_SIZE_NORMAL, UI_COLOR_PRIMARY);
        }
    }
}

void ui_input_clear(InputField* field) {
    if (!field) return;
    memset(field->text, 0, sizeof(field->text));
    field->cursor = 0;
}

void ui_input_set_text(InputField* field, const char* text) {
    if (!field || !text) return;
    strncpy(field->text, text, sizeof(field->text) - 1);
    field->cursor = strlen(field->text);
}

// ============ Button ============

void ui_button_init(Button* button, float x, float y, float width, float height, const char* label, Color color) {
    if (!button) return;
    button->bounds = (Rectangle){x, y, width, height};
    button->label = label;
    button->hovered = false;
    button->pressed = false;
    button->disabled = false;
    button->color = color;
    button->hoverAnim = 0.0f;
}

bool ui_button_update(Button* button) {
    if (!button || button->disabled) return false;
    
    Vector2 mouse = GetMousePosition();
    button->hovered = ui_point_in_rect(mouse, button->bounds);
    
    // Animate hover
    float target = button->hovered ? 1.0f : 0.0f;
    button->hoverAnim += (target - button->hoverAnim) * UI_ANIMATION_SPEED * 2.0f;
    
    button->pressed = button->hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    
    return button->hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void ui_button_draw(const Button* button) {
    if (!button) return;
    
    // Calculate colors
    Color bgColor = button->color;
    if (button->disabled) {
        bgColor = UI_COLOR_BG_LIGHTER;
    } else if (button->pressed) {
        bgColor = ui_color_lerp(button->color, UI_COLOR_BG, 0.3f);
    } else if (button->hovered) {
        bgColor = ui_color_lerp(button->color, WHITE, 0.15f);
    }
    
    // Draw shadow when hovered
    if (button->hoverAnim > 0.01f && !button->disabled) {
        Rectangle shadowRect = button->bounds;
        shadowRect.y += 4 * button->hoverAnim;
        Color shadowColor = {0, 0, 0, (unsigned char)(40 * button->hoverAnim)};
        ui_draw_rounded_rect(shadowRect, UI_BORDER_RADIUS, shadowColor);
    }
    
    // Slight lift when hovered
    Rectangle drawRect = button->bounds;
    if (!button->disabled) {
        drawRect.y -= 2 * button->hoverAnim;
    }
    
    // Draw button
    ui_draw_rounded_rect(drawRect, UI_BORDER_RADIUS, bgColor);
    
    // Draw label
    Color textColor = button->disabled ? UI_COLOR_TEXT_DIM : UI_COLOR_TEXT;
    ui_draw_text_centered(button->label, drawRect, UI_FONT_SIZE_NORMAL, textColor);
}

// ============ Panel ============

void ui_panel_init(Panel* panel, float x, float y, float width, float height, const char* title) {
    if (!panel) return;
    panel->bounds = (Rectangle){x, y, width, height};
    panel->title = title;
    panel->collapsed = false;
    panel->collapseAnim = 1.0f;
}

void ui_panel_update(Panel* panel) {
    if (!panel) return;
    
    // Animate collapse
    float target = panel->collapsed ? 0.0f : 1.0f;
    panel->collapseAnim += (target - panel->collapseAnim) * UI_ANIMATION_SPEED * 2.0f;
}

void ui_panel_draw_begin(const Panel* panel) {
    if (!panel) return;
    
    // Draw panel background
    float height = 40 + (panel->bounds.height - 40) * panel->collapseAnim;
    Rectangle drawRect = {panel->bounds.x, panel->bounds.y, panel->bounds.width, height};
    
    // Shadow
    Rectangle shadowRect = drawRect;
    shadowRect.x += 4;
    shadowRect.y += 4;
    ui_draw_rounded_rect(shadowRect, UI_BORDER_RADIUS, (Color){0, 0, 0, 30});
    
    // Background
    ui_draw_rounded_rect(drawRect, UI_BORDER_RADIUS, UI_COLOR_BG_LIGHT);
    ui_draw_rounded_rect_outline(drawRect, UI_BORDER_RADIUS, 1.0f, UI_COLOR_BORDER);
    
    // Title bar
    if (panel->title) {
        Rectangle titleRect = {drawRect.x, drawRect.y, drawRect.width, 40};
        DrawText(panel->title, (int)(titleRect.x + UI_PADDING), 
                (int)(titleRect.y + (40 - UI_FONT_SIZE_NORMAL) / 2),
                UI_FONT_SIZE_NORMAL, UI_COLOR_TEXT);
        
        // Separator
        DrawLineEx(
            (Vector2){drawRect.x + UI_PADDING, drawRect.y + 40},
            (Vector2){drawRect.x + drawRect.width - UI_PADDING, drawRect.y + 40},
            1.0f, UI_COLOR_BORDER
        );
    }
    
    // Set scissor for content
    if (panel->collapseAnim > 0.01f) {
        BeginScissorMode(
            (int)drawRect.x, 
            (int)(drawRect.y + 40),
            (int)drawRect.width,
            (int)(height - 40)
        );
    }
}

void ui_panel_draw_end(void) {
    EndScissorMode();
}

// ============ Notifications ============

void ui_notify(const char* message, NotifyType type) {
    if (!message) return;
    
    // Find empty slot or push oldest out
    int slot = -1;
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        if (!notificationQueue.items[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        // Shift notifications
        for (int i = 0; i < MAX_NOTIFICATIONS - 1; i++) {
            notificationQueue.items[i] = notificationQueue.items[i + 1];
        }
        slot = MAX_NOTIFICATIONS - 1;
    }
    
    Notification* n = &notificationQueue.items[slot];
    strncpy(n->message, message, sizeof(n->message) - 1);
    n->type = type;
    n->timer = 3.0f;  // 3 second display
    n->alpha = 0.0f;
    n->active = true;
    notificationQueue.count++;
}

void ui_notifications_update(float deltaTime) {
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        Notification* n = &notificationQueue.items[i];
        if (!n->active) continue;
        
        // Fade in
        if (n->timer > 2.5f) {
            n->alpha += deltaTime * 4.0f;
            if (n->alpha > 1.0f) n->alpha = 1.0f;
        }
        // Fade out
        else if (n->timer < 0.5f) {
            n->alpha -= deltaTime * 2.0f;
            if (n->alpha < 0.0f) n->alpha = 0.0f;
        }
        
        n->timer -= deltaTime;
        if (n->timer <= 0.0f) {
            n->active = false;
            notificationQueue.count--;
        }
    }
}

void ui_notifications_draw(void) {
    int screenWidth = GetScreenWidth();
    float y = 20;
    
    for (int i = 0; i < MAX_NOTIFICATIONS; i++) {
        Notification* n = &notificationQueue.items[i];
        if (!n->active) continue;
        
        Color bgColor;
        switch (n->type) {
            case NOTIFY_SUCCESS: bgColor = UI_COLOR_SECONDARY; break;
            case NOTIFY_WARNING: bgColor = UI_COLOR_ACCENT; break;
            case NOTIFY_ERROR: bgColor = UI_COLOR_DANGER; break;
            default: bgColor = UI_COLOR_PRIMARY; break;
        }
        bgColor.a = (unsigned char)(255 * n->alpha);
        
        int textWidth = MeasureText(n->message, UI_FONT_SIZE_NORMAL);
        float width = textWidth + UI_PADDING * 2;
        float x = screenWidth - width - 20;
        
        Rectangle rect = {x, y, width, 40};
        ui_draw_rounded_rect(rect, UI_BORDER_RADIUS, bgColor);
        
        Color textColor = UI_COLOR_TEXT;
        textColor.a = (unsigned char)(255 * n->alpha);
        ui_draw_text_centered(n->message, rect, UI_FONT_SIZE_NORMAL, textColor);
        
        y += 50;
    }
}

// ============ Drawing Helpers ============

void ui_draw_rounded_rect(Rectangle rect, float radius, Color color) {
    DrawRectangleRounded(rect, radius / rect.height, 8, color);
}

void ui_draw_rounded_rect_outline(Rectangle rect, float radius, float thickness, Color color) {
    DrawRectangleRoundedLinesEx(rect, radius / rect.height, 8, thickness, color);
}

void ui_draw_shadow(Rectangle rect, float radius, float blur, Color color) {
    (void)blur;  // Not fully implemented
    Rectangle shadowRect = rect;
    shadowRect.x += 4;
    shadowRect.y += 4;
    ui_draw_rounded_rect(shadowRect, radius, color);
}

void ui_draw_text_centered(const char* text, Rectangle rect, int fontSize, Color color) {
    if (!text) return;
    int textWidth = MeasureText(text, fontSize);
    float x = rect.x + (rect.width - textWidth) / 2;
    float y = rect.y + (rect.height - fontSize) / 2;
    DrawText(text, (int)x, (int)y, fontSize, color);
}

void ui_draw_node(float x, float y, float radius, Color color, bool selected, bool hovered) {
    float drawRadius = radius;
    if (hovered) drawRadius = radius * 1.3f;
    
    // Outer glow for selected
    if (selected) {
        DrawCircle((int)x, (int)y, drawRadius + 8, (Color){color.r, color.g, color.b, 60});
        DrawCircle((int)x, (int)y, drawRadius + 4, (Color){color.r, color.g, color.b, 120});
    }
    
    // Shadow
    DrawCircle((int)(x + 2), (int)(y + 2), drawRadius, (Color){0, 0, 0, 40});
    
    // Main circle
    DrawCircle((int)x, (int)y, drawRadius, color);
    
    // Inner highlight
    DrawCircle((int)(x - drawRadius * 0.3f), (int)(y - drawRadius * 0.3f), 
               drawRadius * 0.3f, (Color){255, 255, 255, 80});
    
    // Border
    DrawCircleLines((int)x, (int)y, drawRadius, (Color){255, 255, 255, 60});
}

void ui_draw_edge(float x1, float y1, float x2, float y2, float thickness, Color color) {
    DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, thickness, color);
}

void ui_draw_path_segment(float x1, float y1, float x2, float y2, float thickness, Color color, float progress) {
    if (progress <= 0.0f) return;
    
    float px2 = x1 + (x2 - x1) * progress;
    float py2 = y1 + (y2 - y1) * progress;
    
    // Glow effect
    DrawLineEx((Vector2){x1, y1}, (Vector2){px2, py2}, thickness + 4, 
               (Color){color.r, color.g, color.b, 60});
    DrawLineEx((Vector2){x1, y1}, (Vector2){px2, py2}, thickness + 2, 
               (Color){color.r, color.g, color.b, 120});
    
    // Main line
    DrawLineEx((Vector2){x1, y1}, (Vector2){px2, py2}, thickness, color);
}

void ui_draw_arrow(float x1, float y1, float x2, float y2, float size, Color color) {
    DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 2.0f, color);
    
    // Arrow head
    float angle = atan2f(y2 - y1, x2 - x1);
    float angle1 = angle + 2.5f;
    float angle2 = angle - 2.5f;
    
    Vector2 p1 = {x2 - size * cosf(angle1), y2 - size * sinf(angle1)};
    Vector2 p2 = {x2 - size * cosf(angle2), y2 - size * sinf(angle2)};
    
    DrawTriangle((Vector2){x2, y2}, p1, p2, color);
}

// ============ Animation Helpers ============

void ui_anim_update(AnimValue* anim, float deltaTime) {
    if (!anim) return;
    float diff = anim->target - anim->current;
    anim->current += diff * anim->speed * deltaTime * 60.0f;
}

float ui_ease_out_cubic(float t) {
    t = t - 1.0f;
    return t * t * t + 1.0f;
}

float ui_ease_in_out_cubic(float t) {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float p = 2.0f * t - 2.0f;
        return 0.5f * p * p * p + 1.0f;
    }
}

// ============ Utility ============

bool ui_point_in_rect(Vector2 point, Rectangle rect) {
    return point.x >= rect.x && point.x <= rect.x + rect.width &&
           point.y >= rect.y && point.y <= rect.y + rect.height;
}

Color ui_color_lerp(Color a, Color b, float t) {
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}
