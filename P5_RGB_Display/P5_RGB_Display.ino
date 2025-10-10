/*
 * ================================================================
 * Project   : ESP32 + HUB75 RGB LED Matrix (Virtual Panel Mapping)
 * Board     : ESP32
 * Display   : HUB75 LED Panel (P4 / P5 / P10) — 1/4 Scan Type
 * Library   : ESP32-HUB75-VirtualMatrixPanel_T
 * ================================================================
 *
 * Features:
 * ---------
 * ✅ Drives non-standard 1/4 scan HUB75 LED matrices using I2S  
 * ✅ Uses VirtualMatrixPanel_T for custom scan/pixel remapping  
 * ✅ Supports multiple panels chained in rows and columns  
 * ✅ Initializes display, sets brightness, and draws test pattern  
 * ✅ Useful for outdoor or complex panel wiring configurations  
 *
 * Author : Kamlesh Vasoya  
 * Date   : 06/10/2025  
 * ================================================================
 */

#include <Arduino.h>
#include <ESP32-HUB75-VirtualMatrixPanel_T.hpp>

// === OPTIONAL (Not used in this code) ===
int counter = 10;
int red = 0, green = 0, blue = 0;

// === PANEL SIZE CONFIGURATION ===
#define PANEL_RES_X     64   // Width of one physical panel (in pixels)
#define PANEL_RES_Y     32   // Height of one physical panel (in pixels)

#define VDISP_NUM_ROWS  2    // Number of vertical panels
#define VDISP_NUM_COLS  1    // Number of horizontal panels

// Total panel count in chain
#define PANEL_CHAIN_LEN (VDISP_NUM_ROWS * VDISP_NUM_COLS)

// === CHAINING AND SCAN TYPE CONFIGURATION ===
#define PANEL_CHAIN_TYPE CHAIN_TOP_RIGHT_DOWN     // Panel chaining layout
#define PANEL_SCAN_TYPE  FOUR_SCAN_32PX_HIGH      // 1/4 scan mapping (for outdoor panels)

// === DMA DRIVER OBJECT (Required) ===
MatrixPanel_I2S_DMA* dma_display = nullptr;

// === VIRTUAL DISPLAY OBJECT with scan remapping ===
using MyScanTypeMapping = ScanTypeMapping<PANEL_SCAN_TYPE>;
VirtualMatrixPanel_T<PANEL_CHAIN_TYPE, MyScanTypeMapping>* virtualDisp = nullptr;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Allow time for serial monitor to connect

  // === CONFIGURE DMA MATRIX PANEL ===
  // HACK: For 1/4 scan, you MUST double width and half height
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X * 2,     // Double width trick
    PANEL_RES_Y / 2,     // Half height trick
    PANEL_CHAIN_LEN
  );

  // Optional DMA settings (for speed/stability tuning)
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;
  mxconfig.clkphase = false;
  // mxconfig.driver = HUB75_I2S_CFG::FM6126A;  // For special driver chips (if needed)

  // === INITIALIZE DMA DISPLAY DRIVER ===
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();                   // Start DMA engine
  dma_display->setBrightness8(50);        // Set brightness (0–255)
  dma_display->clearScreen();             // Blank screen

  // === INITIALIZE VIRTUAL MATRIX PANEL ===
  virtualDisp = new VirtualMatrixPanel_T<PANEL_CHAIN_TYPE, MyScanTypeMapping>(
    VDISP_NUM_ROWS, VDISP_NUM_COLS, PANEL_RES_X, PANEL_RES_Y
  );

  // Link physical display to virtual display driver
  virtualDisp->setDisplay(*dma_display);

  // === DRAW TEST PATTERN ===
  // Fill entire display with dim red
  // Left column = green, right column = blue
  for (int y = 0; y < virtualDisp->height(); y++) {
    for (int x = 0; x < virtualDisp->width(); x++) {

      uint16_t color = virtualDisp->color565(96, 0, 0);  // Default: red

      if (x == 0) 
        color = virtualDisp->color565(0, 255, 0);        // Left edge: green
      else if (x == virtualDisp->width() - 1)
        color = virtualDisp->color565(0, 0, 255);        // Right edge: blue

      virtualDisp->drawPixel(x, y, color);               // Set pixel color
      delay(1);  // Delay for visual effect
    }
  }

  delay(3000);                  // Hold pattern for 3 seconds
  virtualDisp->clearScreen();   // Clear the display
}

void loop() {
  delay(1000);  // Placeholder loop
  // You can add animations, text, or data display here
}
