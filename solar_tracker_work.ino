// =================================================================
// ===           TRACKER SOLAIRE 2 AXES AVEC ESP32-C3            ===
// ===       LDRs + Moteurs DC + OLED SH1107 + INA219            ===
// =================================================================

// --- Bibliothèques nécessaires ---
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_INA219.h>

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
const int zoneMorte = 10;       // Hystérésis pour éviter les oscillations (à ajuster)
const int delaiMouvement = 30; // Durée d'activation des moteurs en ms pour chaque ajustement
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

// === Variables pour la temporisation ===
unsigned long dernierScan = 0;
const int intervalleScan = 200; // Lire les capteurs et ajuster toutes les 200ms

void setup() {
  Serial.begin(115200);

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
  pinMode(MOTEUR_H_IN1, OUTPUT);
  pinMode(MOTEUR_H_IN2, OUTPUT);
  pinMode(MOTEUR_V_IN1, OUTPUT);
  pinMode(MOTEUR_V_IN2, OUTPUT);
  stopMoteurs(); // S'assurer que les moteurs sont à l'arrêt au démarrage

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

void lireCapteurs() {
  // Soustrait l'ancienne valeur et ajoute la nouvelle
  totalGauche = totalGauche - lecturesGauche[indiceMoyenne] + analogRead(LDR_GAUCHE);
  totalDroite = totalDroite - lecturesDroite[indiceMoyenne] + analogRead(LDR_DROITE);
  totalHaut   = totalHaut   - lecturesHaut[indiceMoyenne]   + analogRead(LDR_HAUT);
  totalBas    = totalBas    - lecturesBas[indiceMoyenne]    + analogRead(LDR_BAS);

  // Met à jour le tableau avec la nouvelle valeur
  lecturesGauche[indiceMoyenne] = analogRead(LDR_GAUCHE);
  lecturesDroite[indiceMoyenne] = analogRead(LDR_DROITE);
  lecturesHaut[indiceMoyenne]   = analogRead(LDR_HAUT);
  lecturesBas[indiceMoyenne]    = analogRead(LDR_BAS);

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
  // --- Contrôle Horizontal ---
  if (abs(moyGauche - moyDroite) > zoneMorte) {
    if (moyGauche > moyDroite) {
      tournerGauche();
      etatHorizontal = "Gauche";
    } else {
      tournerDroite();
      etatHorizontal = "Droite";
    }
    delay(delaiMouvement); // Active le moteur pour une courte durée
  }
  stopMoteurHorizontal(); // Arrête toujours le moteur après l'impulsion
  etatHorizontal = "Stop";

  // --- Contrôle Vertical ---
  if (abs(moyHaut - moyBas) > zoneMorte) {
    if (moyHaut > moyBas) {
      monter();
      etatVertical = "Haut";
    } else {
      descendre();
      etatVertical = "Bas";
    }
    delay(delaiMouvement);
  }
  stopMoteurVertical();
  etatVertical = "Stop";
}

void mettreAJourOLED() {
  display.clearDisplay();

  // --- Colonne de gauche : Capteurs LDR ---
  display.setCursor(0, 0);
  display.print("LDR_MATRIX");
  display.setCursor(0, 12);
  display.printf("G:%-4d", moyGauche);
  display.setCursor(0, 22);
  display.printf("D:%-4d", moyDroite);
  display.setCursor(0, 32);
  display.printf("H:%-4d", moyHaut);
  display.setCursor(0, 42);
  display.printf("B:%-4d", moyBas);

  // Ligne de séparation verticale
  display.drawFastVLine(63, 0, 52, SH110X_WHITE);

  // --- Colonne de droite : Puissance INA219 ---
  display.setCursor(68, 0);
  display.print("Power");
  display.setCursor(68, 12);
  display.printf("%.2f V", busVoltage);
  display.setCursor(68, 22);
  display.printf("%.1f mA", current_mA);
  display.setCursor(68, 32);
  display.printf("%.1f mW", power_mW);

  // Ligne de séparation horizontale en bas
  display.drawFastHLine(0, 53, 128, SH110X_WHITE);
  
  // --- Ligne du bas : Statut du système ---
  display.setCursor(3, 56);
  display.print("STATUS: TRACKING...");
  
  display.display();
}


// === Fonctions de contrôle des moteurs ===
void tournerGauche()  { digitalWrite(MOTEUR_H_IN1, HIGH); digitalWrite(MOTEUR_H_IN2, LOW); }
void tournerDroite()  { digitalWrite(MOTEUR_H_IN1, LOW);  digitalWrite(MOTEUR_H_IN2, HIGH); }
void stopMoteurHorizontal() { digitalWrite(MOTEUR_H_IN1, LOW);  digitalWrite(MOTEUR_H_IN2, LOW); }
void monter()         { digitalWrite(MOTEUR_V_IN1, HIGH); digitalWrite(MOTEUR_V_IN2, LOW); }
void descendre()      { digitalWrite(MOTEUR_V_IN1, LOW);  digitalWrite(MOTEUR_V_IN2, HIGH); }
void stopMoteurVertical()   { digitalWrite(MOTEUR_V_IN1, LOW);  digitalWrite(MOTEUR_V_IN2, LOW); }
void stopMoteurs()    { stopMoteurHorizontal(); stopMoteurVertical(); }