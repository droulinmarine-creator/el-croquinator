#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- WIFI CONFIG ---
const char *ssid = "La Parros";
const char *password = "LP48.100+";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// --- ECRAN OLED ---
#define SCREEN_WIDTH 128     // Taille de l'écran OLED, en pixel, au niveau de sa largeur
#define SCREEN_HEIGHT 64     // Taille de l'écran OLED, en pixel, au niveau de sa hauteur
#define OLED_RESET_PIN -1    // Reset de l'OLED partagé avec l'Arduino (d'où la valeur à -1, et non un numéro de pin)
#define OLED_I2C_ADRESS 0x3C // Adresse de "mon" écran OLED sur le bus i2c (généralement égal à 0x3C ou 0x3D)

// --- SERVOMOTEUR ---
#define SERVO_PIN D3
const int ANGLE_OUVERTURE = 180;       // Angle pour ouvrir la valve
const int ANGLE_FERMETURE = 0;         // Angle pour fermer la valve
const unsigned int croquinettes = 111; // temps (ms) ouverture rapide
const unsigned int croquettes = 500;   // temps (ms) ouverture longue

// --- RTC (DS1302) ---
#define DS1302_CLK_PIN D6
#define DS1302_DAT_PIN D7
#define DS1302_RST_PIN D5

// --- PARAMETRES TIMER ---
const unsigned long FEED_DELAY_SEC = 30 * 1000;
unsigned plagehoraire = true; // Activer ou désactiver la plage horaire de nourrissage

// Capteur de présence de croquettes
#define IR_PIN D0  // Pin du capteur ir



#endif
