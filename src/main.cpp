#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "bsp_cst816.h"

#define EXAMPLE_PIN_NUM_LCD_SCLK 39
#define EXAMPLE_PIN_NUM_LCD_MOSI 38
#define EXAMPLE_PIN_NUM_LCD_MISO 40
#define EXAMPLE_PIN_NUM_LCD_DC 42
#define EXAMPLE_PIN_NUM_LCD_RST -1
#define EXAMPLE_PIN_NUM_LCD_CS 45
#define EXAMPLE_PIN_NUM_LCD_BL 1
#define EXAMPLE_PIN_NUM_TP_SDA 48
#define EXAMPLE_PIN_NUM_TP_SCL 47

#define EXAMPLE_LCD_ROTATION 1  // 90 degrees for landscape
#define EXAMPLE_LCD_H_RES 240  // Physical width
#define EXAMPLE_LCD_V_RES 320  // Physical height

#define BLACK 0x0000

/* Arduino_GFX setup */
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  EXAMPLE_PIN_NUM_LCD_DC /* DC */, EXAMPLE_PIN_NUM_LCD_CS /* CS */,
  EXAMPLE_PIN_NUM_LCD_SCLK /* SCK */, EXAMPLE_PIN_NUM_LCD_MOSI /* MOSI */, EXAMPLE_PIN_NUM_LCD_MISO /* MISO */);

Arduino_GFX *gfx = new Arduino_ST7789(
  bus, EXAMPLE_PIN_NUM_LCD_RST /* RST */, EXAMPLE_LCD_ROTATION /* rotation */, true /* IPS */,
  EXAMPLE_LCD_H_RES /* width */, EXAMPLE_LCD_V_RES /* height */);

/* LVGL display buffer */
uint32_t screenWidth;
uint32_t screenHeight;
uint32_t bufSize;
lv_disp_draw_buf_t draw_buf;
lv_color_t *disp_draw_buf;
lv_disp_drv_t disp_drv;

/* Screen objects */
lv_obj_t *screen_main_menu;
lv_obj_t *screen_status;
lv_obj_t *screen_battery;
lv_obj_t *screen_diagnostics;
lv_obj_t *screen_tests;

/* LVGL tick callback */
static uint32_t my_tick_get_cb(void) {
    return millis();
}

/* Display flush - Arduino_GFX style */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
  lv_disp_flush_ready(disp_drv);
}

/* Touch input read - matches working examples exactly */
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    uint16_t touchpad_x;
    uint16_t touchpad_y;
    bsp_touch_read();
    if (bsp_touch_get_coordinates(&touchpad_x, &touchpad_y)) {
        data->point.x = touchpad_x;
        data->point.y = touchpad_y;
        data->state = LV_INDEV_STATE_PRESSED;
        Serial.printf("TOUCH! x=%d, y=%d\n", touchpad_x, touchpad_y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* Navigation functions */
void navigate_to(lv_obj_t *screen) {
    Serial.printf("Navigating to screen: %p\n", screen);
    lv_scr_load(screen);  // Instant load - no animation
    lv_obj_invalidate(lv_scr_act());  // Force full screen redraw
    lv_refr_now(NULL);    // Force immediate refresh
    Serial.println("Screen loaded successfully");
}

void go_to_main_menu(lv_event_t *e) {
    navigate_to(screen_main_menu);
}

void go_to_status(lv_event_t *e) {
    navigate_to(screen_status);
}

void go_to_battery(lv_event_t *e) {
    navigate_to(screen_battery);
}

void go_to_diagnostics(lv_event_t *e) {
    navigate_to(screen_diagnostics);
}

void go_to_tests(lv_event_t *e) {
    navigate_to(screen_tests);
}

/* Helper to create a back button */
lv_obj_t* create_back_button(lv_obj_t *parent) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 80, 40);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(btn, go_to_main_menu, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(label);
    
    return btn;
}

/* Helper to create a menu button */
lv_obj_t* create_menu_button(lv_obj_t *parent, const char *text, lv_event_cb_t callback) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 140, 60);
    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    return btn;
}

/* Create all application screens */
void create_screens() {
    // ===== Main Menu Screen =====
    screen_main_menu = lv_obj_create(NULL);
    lv_obj_clear_flag(screen_main_menu, LV_OBJ_FLAG_CLICKABLE); // Make container click-through
    
    lv_obj_t *title = lv_label_create(screen_main_menu);
    lv_label_set_text(title, "Robot HMI");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create 2x2 button grid
    lv_obj_t *btn_status = create_menu_button(screen_main_menu, "Status", go_to_status);
    lv_obj_align(btn_status, LV_ALIGN_CENTER, -75, -40);
    
    lv_obj_t *btn_battery = create_menu_button(screen_main_menu, "Battery", go_to_battery);
    lv_obj_align(btn_battery, LV_ALIGN_CENTER, 75, -40);
    
    lv_obj_t *btn_diagnostics = create_menu_button(screen_main_menu, "Diagnostics", go_to_diagnostics);
    lv_obj_align(btn_diagnostics, LV_ALIGN_CENTER, -75, 40);
    
    lv_obj_t *btn_tests = create_menu_button(screen_main_menu, "Tests", go_to_tests);
    lv_obj_align(btn_tests, LV_ALIGN_CENTER, 75, 40);
    
    // ===== Status Screen =====
    screen_status = lv_obj_create(NULL);
    lv_obj_clear_flag(screen_status, LV_OBJ_FLAG_CLICKABLE);
    create_back_button(screen_status);
    
    lv_obj_t *status_title = lv_label_create(screen_status);
    lv_label_set_text(status_title, "Robot Status");
    lv_obj_align(status_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *status_content = lv_label_create(screen_status);
    lv_label_set_text(status_content, 
        "System: OK\n"
        "Mode: Autonomous\n"
        "Uptime: 1234s\n"
        "Speed: 1.2 m/s"
    );
    lv_obj_align(status_content, LV_ALIGN_CENTER, 0, 0);
    
    // ===== Battery Screen =====
    screen_battery = lv_obj_create(NULL);
    lv_obj_clear_flag(screen_battery, LV_OBJ_FLAG_CLICKABLE);
    create_back_button(screen_battery);
    
    lv_obj_t *battery_title = lv_label_create(screen_battery);
    lv_label_set_text(battery_title, "Battery Monitor");
    lv_obj_align(battery_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *battery_content = lv_label_create(screen_battery);
    lv_label_set_text(battery_content, 
        "Voltage: 12.4V\n"
        "Current: 2.5A\n"
        "Capacity: 85%\n"
        "Time Remaining: 3.2h"
    );
    lv_obj_align(battery_content, LV_ALIGN_CENTER, 0, 0);
    
    // ===== Diagnostics Screen =====
    screen_diagnostics = lv_obj_create(NULL);
    lv_obj_clear_flag(screen_diagnostics, LV_OBJ_FLAG_CLICKABLE);
    create_back_button(screen_diagnostics);
    
    lv_obj_t *diag_title = lv_label_create(screen_diagnostics);
    lv_label_set_text(diag_title, "System Diagnostics");
    lv_obj_align(diag_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *diag_content = lv_label_create(screen_diagnostics);
    lv_label_set_text(diag_content, 
        "CPU Temp: 45C\n"
        "Memory: 64KB/320KB\n"
        "Errors: 0\n"
        "Warnings: 0"
    );
    lv_obj_align(diag_content, LV_ALIGN_CENTER, 0, 0);
    
    // ===== Tests Screen =====
    screen_tests = lv_obj_create(NULL);
    lv_obj_clear_flag(screen_tests, LV_OBJ_FLAG_CLICKABLE);
    create_back_button(screen_tests);
    
    lv_obj_t *tests_title = lv_label_create(screen_tests);
    lv_label_set_text(tests_title, "Self Tests");
    lv_obj_align(tests_title, LV_ALIGN_TOP_MID, 0, 10);
    
    lv_obj_t *tests_content = lv_label_create(screen_tests);
    lv_label_set_text(tests_content, 
        "Motor Test: PASS\n"
        "Sensor Test: PASS\n"
        "Comm Test: PASS\n"
        "All Systems: GO"
    );
    lv_obj_align(tests_content, LV_ALIGN_CENTER, 0, 0);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== Arduino_GFX LVGL Example ===");
    String LVGL_Arduino = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
    Serial.println(LVGL_Arduino);

    // Init Display
    if (!gfx->begin()) {
        Serial.println("gfx->begin() failed!");
    }
    gfx->fillScreen(BLACK);
    Serial.println("Display initialized");

    // Init backlight
    pinMode(EXAMPLE_PIN_NUM_LCD_BL, OUTPUT);
    digitalWrite(EXAMPLE_PIN_NUM_LCD_BL, HIGH);
    Serial.println("Backlight enabled");
    
    // Init touch device - matches working examples
    Wire.begin(EXAMPLE_PIN_NUM_TP_SDA, EXAMPLE_PIN_NUM_TP_SCL);
    bsp_touch_init(&Wire, gfx->getRotation(), gfx->width(), gfx->height());

    // Init LVGL
    lv_init();
    Serial.println("LVGL initialized");

    screenWidth = gfx->width();
    screenHeight = gfx->height();
    bufSize = screenWidth * screenHeight;

    // Allocate LVGL draw buffer
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) {
        // Try without MALLOC_CAP_INTERNAL
        disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
    }

    if (!disp_draw_buf) {
        Serial.println("LVGL disp_draw_buf allocate failed!");
    } else {
        Serial.println("LVGL buffer allocated");
        lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, bufSize);

        // Initialize the display driver
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res = screenWidth;
        disp_drv.ver_res = screenHeight;
        disp_drv.flush_cb = my_disp_flush;
        disp_drv.draw_buf = &draw_buf;
        disp_drv.direct_mode = true;
        lv_disp_drv_register(&disp_drv);
        
        Serial.print("Screen dimensions: ");
        Serial.print(screenWidth);
        Serial.print("x");
        Serial.println(screenHeight);

        Serial.println("Display driver registered");

        // Initialize the input device driver
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = my_touchpad_read;
        lv_indev_t *my_indev = lv_indev_drv_register(&indev_drv);
        
        if (my_indev != NULL) {
            Serial.printf("Touch input registered at %p\n", my_indev);
        } else {
            Serial.println("ERROR: Touch input registration failed!");
        }

        // Create all application screens
        create_screens();
        
        // Load the main menu as the starting screen
        lv_scr_load(screen_main_menu);
        
        // Force LVGL to calculate layout for all objects
        lv_obj_update_layout(screen_main_menu);
        lv_obj_update_layout(screen_status);
        lv_obj_update_layout(screen_battery);
        lv_obj_update_layout(screen_diagnostics);
        lv_obj_update_layout(screen_tests);
        
        Serial.println("Robot HMI screens created and laid out");
    }

    Serial.println("Setup done");
}

void loop()
{
    static unsigned long last_debug = 0;
    static int loop_count = 0;
    
    // Manually read touch and inject into LVGL (bypassing broken callback)
    static bool was_pressed = false;
    static unsigned long last_click_time = 0;
    uint16_t x, y;
    bsp_touch_read();
    bool is_pressed = bsp_touch_get_coordinates(&x, &y);
    
    // Debounce and ignore touches right after screen navigation
    if (is_pressed && !was_pressed && (millis() - last_click_time > 300)) {
        // Touch just pressed (with 300ms debounce)
        Serial.printf("TOUCH PRESS: x=%d, y=%d\n", x, y);
        
        // Debug: list all children on screen
        lv_obj_t *screen = lv_scr_act();
        Serial.printf("  Active screen: %p, child count: %d\n", screen, lv_obj_get_child_cnt(screen));
        
        lv_point_t point = {(lv_coord_t)x, (lv_coord_t)y};
        lv_obj_t *obj = lv_indev_search_obj(screen, &point);
        Serial.printf("  Search result: %p\n", obj);
        
        // Try direct child iteration
        uint32_t child_cnt = lv_obj_get_child_cnt(screen);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t *child = lv_obj_get_child(screen, i);
            lv_area_t coords;
            lv_obj_get_coords(child, &coords);
            Serial.printf("    Child %d: %p, pos(%d,%d)-(%d,%d), clickable=%d\n", 
                         i, child, coords.x1, coords.y1, coords.x2, coords.y2,
                         lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE));
            
            // Check if touch is within this child
            if (x >= coords.x1 && x <= coords.x2 && y >= coords.y1 && y <= coords.y2) {
                Serial.printf("      Touch is inside this child!\n");
                if (lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)) {
                    Serial.printf("      Sending click to this child\n");
                    lv_event_send(child, LV_EVENT_CLICKED, NULL);
                    last_click_time = millis();
                    delay(50);
                    break;
                }
            }
        }
    }
    was_pressed = is_pressed;
    
    lv_task_handler(); /* LVGL 8.3 - handles timers and tasks */
    
    // Debug: Print loop status every 5 seconds
    loop_count++;
    if (millis() - last_debug > 5000) {
        Serial.printf("Loop running: %d iterations in 5s\n", loop_count);
        loop_count = 0;
        last_debug = millis();
    }
    
    // Push LVGL buffer to display
#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#else
    gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#endif
    
    delay(5);
}
