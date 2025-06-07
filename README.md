# ☀️ Mini Tracker Solaire ESP32-C3 – Web UI & Données JSON *(WIP)*

Un projet de **suiveur solaire DIY** à deux axes, basé sur un **ESP32-C3 Super Mini**, contrôlé par 4 capteurs LDR, affichage OLED et capteur de puissance INA219.  
Il diffuse les données en temps réel au format JSON et propose une **interface web locale moderne** pour la visualisation, l’historique de production, et le réglage à distance des paramètres.

> ⚡ **Work in Progress** – Documenté au fur et à mesure  
> 💡 Liens affiliés à venir pour soutenir ce projet

---

## 🎯 Objectifs

- Suivi solaire automatique à 2 axes (horizontal + vertical)
- Pilotage des moteurs DC via un driver DRV8833
- Visualisation locale via écran OLED SH1107
- Lecture en direct de la puissance produite (INA219)
- Interface web locale depuis n’importe quel navigateur sur le réseau
- Transmission de données en JSON (pour enregistrement local + dashboard)
- Contrôle des paramètres (zone morte, délai moteur, etc.) depuis l'UI

---

## 🧩 Matériel utilisé *(liens affiliés à venir)*

| Composant | Rôle |
|----------|------|
| **ESP32-C3 Super Mini** | Microcontrôleur avec Wi-Fi intégré |
| **4 × LDR** | Détection directionnelle de la lumière |
| **2 Moteurs DC avec réducteurs** | Rotation axe horizontal (RF-300C) et vertical (N20) |
| **DRV8833** | Module double driver moteur |
| **OLED SH1107 (128x64)** | Affichage embarqué |
| **INA219** | Capteur de tension/courant/power |
| **Batterie Li-Ion 3.7V + panneau solaire 10 W** | Alimentation autonome |
| **Résistances 10kΩ, câblage, connecteurs** | Divers composants passifs |

---

## 🧠 Fonctionnement

1. **ESP32-C3** lit les 4 LDRs (gauche, droite, haut, bas)
2. Calcule des moyennes glissantes + comparaison avec seuil ("zone morte")
3. Active les moteurs pendant une courte durée si une correction est nécessaire
4. Lecture des mesures via **INA219** (volts, ampères, watts)
5. Affiche les données sur **OLED SH1107**
6. Publie les données toutes les X ms via **JSON HTTP**
7. Interface web locale accessible via Wi-Fi (`http://ip-de-l'esp32`)
8. Données récupérées et affichées dynamiquement (et historisées sur le PC)

---

## 🌐 Interface Web Locale

La page HTML `Local_web_tracker_solaire.html` fournit :

- 🌞 Affichage en direct : puissance, tension, courant
- 📊 Graphique dynamique de production sur 24h
- 🔧 Réglages interactifs (zone morte, délai moteur, etc.)
- 🎨 Thèmes visuels personnalisables (dark, vhs, solarpunk…)

💡 *La communication avec l'ESP32 se fait via JSON. La version actuelle simule les données (mode dev), mais peut être reliée à l’ESP32 réel.*

---


---

## 🚧 Prochaines étapes

- [x] Contrôle moteur avec zone morte + moyenne glissante
- [x] Affichage OLED + mesures INA219
- [x] Simulation interface web en mode local
- [ ] Intégration serveur HTTP sur l’ESP32 (Web UI & JSON)
- [ ] Enregistrement historique JSON côté PC
- [ ] Mise en boîte 3D & tests en extérieur
- [ ] Ajout Deep Sleep + économie d’énergie

---

## 📜 Licence

MIT – Utilisation libre, contributions bienvenues.  
Mention de l’auteur appréciée en cas de réutilisation.
