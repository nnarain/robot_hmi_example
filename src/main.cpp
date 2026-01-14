#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>

#define EXAMPLE_PIN_NUM_LCD_SCLK 39
#define EXAMPLE_PIN_NUM_LCD_MOSI 38
#define EXAMPLE_PIN_NUM_LCD_MISO 40
#define EXAMPLE_PIN_NUM_LCD_DC 42
#define EXAMPLE_PIN_NUM_LCD_RST -1
#define EXAMPLE_PIN_NUM_LCD_CS 45
#define EXAMPLE_PIN_NUM_LCD_BL 1

#define EXAMPLE_LCD_ROTATION 0
#define EXAMPLE_LCD_H_RES 240
#define EXAMPLE_LCD_V_RES 320

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

/* Display flush - Arduino_GFX style */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
  lv_disp_flush_ready(disp_drv);
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
    Serial.println("Backlight ON");

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
        disp_drv.direct_mode = true;  // Arduino_GFX uses direct mode
        lv_disp_drv_register(&disp_drv);

        Serial.println("Display driver registered");

        // Create "Hello, LVGL" label
        lv_obj_t *label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "Hello, LVGL!\nPlatformIO + Arduino_GFX");
        lv_obj_center(label);
        
        Serial.println("UI created");
    }

    Serial.println("Setup done");
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    
    // Push LVGL buffer to display - THIS IS THE KEY!
#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#else
    gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#endif
    
    delay(5);
}
