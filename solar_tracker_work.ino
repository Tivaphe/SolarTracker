// =================================================================
// ===           TRACKER SOLAIRE 2 AXES AVEC ESP32-C3            ===
// ===       LDRs + Moteurs DC + OLED SH1107 + INA219            ===
// =================================================================

// Note: Ensure ESPAsyncWebServer and AsyncTCP libraries are installed in your Arduino IDE.
// --- Bibliothèques nécessaires ---
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_INA219.h>
#include <WiFi.h> // Added WiFi library
#include <ESPAsyncWebServer.h> // Added ESPAsyncWebServer

// === WiFi Credentials ===
char ssid[] = "YOUR_SSID";     // your network SSID (name)
char password[] = "YOUR_PASSWORD"; // your network password

// === Web Server Instance ===
AsyncWebServer server(80);

// === Définition des broches ===
#define LDR_GAUCHE 1
#define LDR_DROITE 2
#define LDR_HAUT 4
#define LDR_BAS 3
#define MOTEUR_H_IN1 5  // Horizontal
#define MOTEUR_H_IN2 6
#define MOTEUR_V_IN1 7  // Vertical
#define MOTEUR_V_IN2 10
#define I2C_SDA 8
#define I2C_SCL 9

// === Paramètres de l'écran OLED ===
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 128
#define OLED_RESET -1 // Pas de broche de reset
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// === Instance du capteur INA219 ===
Adafruit_INA219 ina219;

// === Paramètres de contrôle du tracker ===
int zoneMorte = 10;       // Hystérésis pour éviter les oscillations (à ajuster)
int delaiMouvement = 30; // Durée d'activation des moteurs en ms pour chaque ajustement
const int tailleMoyenne = 3;    // Nombre de lectures pour la moyenne glissante

// === Variables pour la moyenne glissante des LDR ===
int lecturesGauche[tailleMoyenne] = {0};
int lecturesDroite[tailleMoyenne] = {0};
int lecturesHaut[tailleMoyenne] = {0};
int lecturesBas[tailleMoyenne] = {0};
int totalGauche = 0, totalDroite = 0, totalHaut = 0, totalBas = 0;
int indiceMoyenne = 0; // Renommé pour éviter le conflit avec la fonction 'index'

// === Variables pour stocker les états et mesures ===
int moyGauche, moyDroite, moyHaut, moyBas;
float busVoltage = 0, current_mA = 0, power_mW = 0;
String etatHorizontal = "Stop";
String etatVertical = "Stop";
// String systemDisplayStatus = "Work"; // Removed for this subtask

// === Variables globales pour les valeurs LDR brutes actuelles ===
int currentRawLDR_G = 0;
int currentRawLDR_D = 0;
int currentRawLDR_H = 0;
int currentRawLDR_B = 0;

// === Variables pour la temporisation ===
unsigned long dernierScan = 0;
const int intervalleScan = 200; // Lire les capteurs et ajuster toutes les 200ms

void setup() {
  Serial.begin(115200);

  // Initialize motor pins to a safe state (LOW) early to prevent unwanted movement
  pinMode(MOTEUR_H_IN1, OUTPUT);
  pinMode(MOTEUR_H_IN2, OUTPUT);
  pinMode(MOTEUR_V_IN1, OUTPUT);
  pinMode(MOTEUR_V_IN2, OUTPUT);

  digitalWrite(MOTEUR_H_IN1, LOW);
  digitalWrite(MOTEUR_H_IN2, LOW);
  digitalWrite(MOTEUR_V_IN1, LOW);
  digitalWrite(MOTEUR_V_IN2, LOW);

  // --- WiFi Connection ---
  Serial.println(); // Print a newline for better readability
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    // Timeout after 15 seconds (15000 ms)
    if (millis() - startTime > 15000) {
      Serial.println();
      Serial.println("Failed to connect to WiFi!");
      break; // Exit the while loop
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // --- Web Server Setup ---
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      String html = buildHtmlPage(); // Call the function to get HTML
      request->send(200, "text/html", html); // Send HTML response
    });

    // Handler for updating parameters
    server.on("/update_params", HTTP_GET, [](AsyncWebServerRequest *request){
      if (request->hasParam("zoneMorte")) {
        String zm_str = request->getParam("zoneMorte")->value();
        zoneMorte = zm_str.toInt();
        Serial.print("zoneMorte updated to: "); Serial.println(zoneMorte);
      }
      if (request->hasParam("delaiMouvement")) {
        String dm_str = request->getParam("delaiMouvement")->value();
        delaiMouvement = dm_str.toInt();
        Serial.print("delaiMouvement updated to: "); Serial.println(delaiMouvement);
      }
      request->redirect("/"); // Redirect back to the main page
    });

    server.begin();
    Serial.println("HTTP server started");

  } else {
    Serial.println();
    Serial.println("WiFi connection attempt finished."); // Or some other status
  }

  // --- Initialisation I2C ---
  Wire.begin(I2C_SDA, I2C_SCL);

  // --- Initialisation de l'écran OLED ---
  if(!display.begin(0x3C, true)) { // Adresse I2C 0x3C
    Serial.println(F("Erreur: Ecran SH1107 non détecté"));
    while(1);
  }
  display.setRotation(1); // 0=Portrait, 1=Paysage, 2=Portrait inversé, 3=Paysage inversé
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Tracker Solaire");
  display.println("SYSTEM BOOT...");
  display.display();
  delay(2000);

  // --- Initialisation du capteur INA219 ---
  if (!ina219.begin()) {
    ina219.setCalibration_32V_2A();
    Serial.println("Erreur: Capteur INA219 non détecté");
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("FATAL ERROR: INA219");
    display.display();
    while (1);
  }
  
  // --- Configuration des broches moteurs ---
  // pinMode calls are now done earlier for safety.
  // MOTEUR_H_IN1, MOTEUR_H_IN2, MOTEUR_V_IN1, MOTEUR_V_IN2 are already OUTPUT and LOW.
  stopMoteurs(); // S'assurer que les moteurs sont à l'arrêt au démarrage (re-confirms LOW state)

  // --- Initialisation de la moyenne glissante ---
  // On "charge" le tableau avec des valeurs initiales pour une moyenne plus réactive
  for (int i = 0; i < tailleMoyenne; i++) {
    lecturesGauche[i] = analogRead(LDR_GAUCHE);
    lecturesDroite[i] = analogRead(LDR_DROITE);
    lecturesHaut[i] = analogRead(LDR_HAUT);
    lecturesBas[i] = analogRead(LDR_BAS);
    totalGauche += lecturesGauche[i];
    totalDroite += lecturesDroite[i];
    totalHaut += lecturesHaut[i];
    totalBas += lecturesBas[i];
  }
}

void loop() {
  // Exécute le code principal à un intervalle défini pour plus de stabilité
  if (millis() - dernierScan > intervalleScan) {
    dernierScan = millis();

    lireCapteurs();
    lirePuissance();
    controlerMoteurs();
    mettreAJourOLED();
  }
}

// === Fonctions principales ===

String buildHtmlPage() {
  String html = "<!DOCTYPE html><html lang='fr'>";
  html += "<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 Solar Tracker Stats</title></head>";
  html += "<body><h1>ESP32 Solar Tracker Statistics</h1>";

  html += "<h2>Raw LDR Values</h2>";
  html += "<p>Gauche (Raw): " + String(currentRawLDR_G) + "</p>";
  html += "<p>Droite (Raw): " + String(currentRawLDR_D) + "</p>";
  html += "<p>Haut (Raw): " + String(currentRawLDR_H) + "</p>";
  html += "<p>Bas (Raw): " + String(currentRawLDR_B) + "</p>";

  html += "<h2>Averaged LDR Values</h2>";
  html += "<p>Gauche (Avg): " + String(moyGauche) + "</p>";
  html += "<p>Droite (Avg): " + String(moyDroite) + "</p>";
  html += "<p>Haut (Avg): " + String(moyHaut) + "</p>";
  html += "<p>Bas (Avg): " + String(moyBas) + "</p>";

  // New section for Control Parameters
  html += "<h2>Control Parameters</h2>";
  html += "<form action='/update_params' method='GET'>";

  html += "<label for='zoneMorte'>Dead Zone (zoneMorte):</label>";
  html += "<input type='number' id='zoneMorte' name='zoneMorte' value='" + String(zoneMorte) + "'><br><br>";

  html += "<label for='delaiMouvement'>Movement Delay (delaiMouvement):</label>";
  html += "<input type='number' id='delaiMouvement' name='delaiMouvement' value='" + String(delaiMouvement) + "'><br><br>";

  html += "<input type='submit' value='Update Parameters'>";
  html += "</form>";

  html += "</body></html>";
  return html;
}

void lireCapteurs() {
  // Lecture LDR Gauche
  currentRawLDR_G = analogRead(LDR_GAUCHE);
  Serial.print("Raw G: "); Serial.println(currentRawLDR_G);
  totalGauche = totalGauche - lecturesGauche[indiceMoyenne] + currentRawLDR_G;
  lecturesGauche[indiceMoyenne] = currentRawLDR_G;

  // Lecture LDR Droite
  currentRawLDR_D = analogRead(LDR_DROITE);
  Serial.print("Raw D: "); Serial.println(currentRawLDR_D);
  totalDroite = totalDroite - lecturesDroite[indiceMoyenne] + currentRawLDR_D;
  lecturesDroite[indiceMoyenne] = currentRawLDR_D;

  // Lecture LDR Haut
  currentRawLDR_H = analogRead(LDR_HAUT);
  Serial.print("Raw H: "); Serial.println(currentRawLDR_H);
  totalHaut   = totalHaut   - lecturesHaut[indiceMoyenne]   + currentRawLDR_H;
  lecturesHaut[indiceMoyenne]   = currentRawLDR_H;

  // Lecture LDR Bas
  currentRawLDR_B = analogRead(LDR_BAS);
  Serial.print("Raw B: "); Serial.println(currentRawLDR_B);
  totalBas    = totalBas    - lecturesBas[indiceMoyenne]    + currentRawLDR_B;
  lecturesBas[indiceMoyenne]    = currentRawLDR_B;

  // Calcule les nouvelles moyennes
  moyGauche = totalGauche / tailleMoyenne;
  moyDroite = totalDroite / tailleMoyenne;
  moyHaut   = totalHaut   / tailleMoyenne;
  moyBas    = totalBas    / tailleMoyenne;

  // Passe à l'index suivant
  indiceMoyenne = (indiceMoyenne + 1) % tailleMoyenne;
}

void lirePuissance() {
  busVoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW   = ina219.getPower_mW();
}

void controlerMoteurs() {
  int darkLDRCount = 0;
  if (moyGauche > 2000) {
    darkLDRCount++;
  }
  if (moyDroite > 2000) {
    darkLDRCount++;
  }
  if (moyHaut > 2000) {
    darkLDRCount++;
  }
  if (moyBas > 2000) {
    darkLDRCount++;
  }

  if (darkLDRCount >= 3) {
    stopMoteurs();
    etatHorizontal = "Sleep";
    etatVertical = "Sleep";
    // systemDisplayStatus = "Sleep"; // Removed
    Serial.println("Sleep mode: 3+ LDRs > 2000."); // Debug message
    return; // Exit the function
  }

  // If not sleeping, then we are in "Work" mode (either moving or stopped)
  // systemDisplayStatus = "Work"; // Removed

  // --- Contrôle Horizontal ---
  etatHorizontal = "Stop"; // Default to Stop for this cycle
  if (abs(moyGauche - moyDroite) > zoneMorte) {
    if (moyGauche < moyDroite) { // Inverted condition
      tournerGauche();
      etatHorizontal = "Gauche";
    } else {
      tournerDroite();
      etatHorizontal = "Droite";
    }
    delay(delaiMouvement); // Active le moteur pour une courte durée
  }
  stopMoteurHorizontal(); // Always stop motor after the check/pulse

  // --- Contrôle Vertical ---
  etatVertical = "Stop"; // Default to Stop for this cycle
  if (abs(moyHaut - moyBas) > zoneMorte) {
    if (moyHaut < moyBas) { // Inverted condition
      monter();
      etatVertical = "Haut";
    } else {
      descendre();
      etatVertical = "Bas";
    }
    delay(delaiMouvement);
  }
  stopMoteurVertical(); // Always stop motor after the check/pulse
}

void mettreAJourOLED() {
  display.clearDisplay(); // Clear the entire display first

  // --- Power Data Display (Top-Left) ---
  // TextSize 1: Font height is typically 8px. Max 6 lines on 64px height with some spacing.
  // Logical screen width is 128, height is 64.
  
  display.setCursor(0, 0); // Line 0
  display.print("Power Stats:");
  
  display.setCursor(0, 10); // Line 1
  display.printf("V: %.2f V", busVoltage);

  display.setCursor(0, 20); // Line 2
  display.printf("C: %.1f mA", current_mA);

  display.setCursor(0, 30); // Line 3
  display.printf("P: %.1f mW", power_mW);

  // --- IP Address Display (Middle) ---
  display.setCursor(0, 42); // Line 4
  display.print("IP: ");
  if (WiFi.status() == WL_CONNECTED) {
    display.print(WiFi.localIP().toString());
  } else {
    display.print("N/A");
  }

  // Optional: Horizontal line to separate IP from Mode status
  // display.drawFastHLine(0, 52, 128, SH110X_WHITE); // Line just below IP

  // --- System Status Line (Bottom) ---
  // Mode display removed as per subtask requirements.
  // The last line of display (Y=56) is now free.

  display.display(); // Update the display with all the new content
}


// === Fonctions de contrôle des moteurs ===
void tournerGauche()  { digitalWrite(MOTEUR_H_IN1, HIGH); digitalWrite(MOTEUR_H_IN2, LOW); }
void tournerDroite()  { digitalWrite(MOTEUR_H_IN1, LOW);  digitalWrite(MOTEUR_H_IN2, HIGH); }
void stopMoteurHorizontal() { digitalWrite(MOTEUR_H_IN1, LOW);  digitalWrite(MOTEUR_H_IN2, LOW); }
void monter()         { digitalWrite(MOTEUR_V_IN1, HIGH); digitalWrite(MOTEUR_V_IN2, LOW); }
void descendre()      { digitalWrite(MOTEUR_V_IN1, LOW);  digitalWrite(MOTEUR_V_IN2, HIGH); }
void stopMoteurVertical()   { digitalWrite(MOTEUR_V_IN1, LOW);  digitalWrite(MOTEUR_V_IN2, LOW); }
void stopMoteurs()    { stopMoteurHorizontal(); stopMoteurVertical(); }