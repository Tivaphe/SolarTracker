// Sketch: LDR_OLED_Display.ino
// Description: Reads 4 LDR values and displays them on an SH1107 OLED screen.

// --- Necessary libraries ---
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// --- LDR Pin Definitions ---
#define LDR_GAUCHE 1
#define LDR_DROITE 2
#define LDR_HAUT 4
#define LDR_BAS 3

// --- I2C Pin Definitions ---
#define I2C_SDA 8
#define I2C_SCL 9

// --- OLED Display Definitions ---
#define OLED_PHYSICAL_WIDTH 64   // Physical width of the display
#define OLED_PHYSICAL_HEIGHT 128 // Physical height of the display
#define OLED_RESET -1            // Reset pin # (or -1 if sharing Arduino reset pin)

// Initialize display object
Adafruit_SH1107 display = Adafruit_SH1107(OLED_PHYSICAL_WIDTH, OLED_PHYSICAL_HEIGHT, &Wire, OLED_RESET);

void setup() {
  // Initialize Serial communication (for display init debug messages)
  Serial.begin(115200);

  // Initialize I2C communication
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialize OLED display
  if(!display.begin(0x3C, true)) { // Address 0x3C, true indicates I2C bus is already initialized
    Serial.println(F("Erreur: Ecran SH1107 non détecté"));
    while(1); // Halt execution if display is not found
  }

  display.setRotation(1); // Rotate to landscape (128 logical width, 64 logical height)
  display.setTextSize(1);       // Set text size to 1
  display.setTextColor(SH110X_WHITE); // Set text color to white

  display.clearDisplay();       // Clear the display buffer
  display.setCursor(0,0);       // Set cursor to top-left
  display.println("LDR RAW VALUES");  // Print title
  display.display();            // Show initial screen
  delay(1000);                  // Pause for a second
}

void loop() {
  // Read raw LDR values
  int ldrG = analogRead(LDR_GAUCHE);
  int ldrD = analogRead(LDR_DROITE);
  int ldrH = analogRead(LDR_HAUT);
  int ldrB = analogRead(LDR_BAS);

  // Clear the display content area (below the title)
  // Title "LDR RAW VALUES" with textSize 1 takes about 8 pixels height.
  // Start clearing from y=10. Logical height is 64.
  display.fillRect(0, 10, 128, 54, SH110X_BLACK);

  // Set cursor and print LDR values
  // Logical screen is 128x64 after rotation. Font height is approx 8px.

  display.setCursor(0, 10); // First data line
  display.print("G: ");
  display.print(ldrG);

  display.setCursor(64, 10); // First data line, second column
  display.print("D: ");
  display.print(ldrD);

  display.setCursor(0, 25);  // Second data line
  display.print("H: ");
  display.print(ldrH);

  display.setCursor(64, 25); // Second data line, second column
  display.print("B: ");
  display.print(ldrB);

  // Update the display
  display.display();

  // Wait before next update
  delay(250);
}
