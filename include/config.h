#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- CONFIGURATION DEBUG ---
#define DEBUG_ENABLED true // Mettre à false pour désactiver tout le debug
// Définition des macros
#if DEBUG_ENABLED
#define DEBUG_PRINT(x)   \
    if (Serial)          \
    {                    \
        Serial.print(x); \
    }
#define DEBUG_PRINTLN(x)   \
    if (Serial)            \
    {                      \
        Serial.println(x); \
    }
#define DEBUG_INIT(speed)              \
    Serial.begin(speed);               \
    while (!Serial && millis() < 5000) \
        ; // Wait max 5s
#else
// Si DEBUG est désactivé, ces lignes sont effacées du code compilé
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_INIT(speed)
#endif
// ---------------------------

// --- WIFI CONFIG ---
const char *SSID = "La Parros";
const char *PASSWORD = "LP48.100+";
const char *NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

// --- SERVOMOTEUR ---
#define SERVO_PIN D3
const int ANGLE_OUVERTURE = 180;       // Angle pour ouvrir la valve
const int ANGLE_FERMETURE = 0;         // Angle pour fermer la valve
const unsigned int CROQUINETTES = 111; // temps (ms) ouverture rapide
const unsigned int CROQUETTES = 500;   // temps (ms) ouverture longue

// --- RTC (DS1302) ---
#define DS1302_CLK_PIN D7
#define DS1302_DAT_PIN D6
#define DS1302_RST_PIN D5

// Capteur de présence de croquettes
#define IR_PIN D0 // Pin du capteur ir

// --- ECRAN OLED ---
#define SCREEN_WIDTH 128            // Taille de l'écran OLED, en pixel, au niveau de sa largeur
#define SCREEN_HEIGHT 64            // Taille de l'écran OLED, en pixel, au niveau de sa hauteur
#define OLED_RESET_PIN -1           // Reset de l'OLED partagé avec l'Arduino (d'où la valeur à -1, et non un numéro de pin)
#define OLED_I2C_ADRESS 0x3C        // Adresse de "mon" écran OLED sur le bus i2c (généralement égal à 0x3C ou 0x3D)
const int DISPLAY_TIME_SEC = 5;     // Temps d'affichage en secondes
unsigned long screenTimerStart = 0; // Quand a commencé l'affichage actuel
unsigned long screenDuration = 0;   // Combien de temps ça doit durer (ms)

// Bouton Tactile TTP223
#define BOUTON_PIN D4                    // Pin du bouton
const int SHORT_PRESS_MAX_MS = 500;      // Un appui court doit être relâché avant 500ms
const int LONG_PRESS_MIN_MS = 3000;      // Un appui très long est maintenu pendant au moins 3s
const int DOUBLE_CLICK_TIMEOUT_MS = 300; // Délai maximum entre deux clics pour être considéré comme un double-clic
const boolean BUTTON_DEFAULT_STATE = LOW;
boolean lastButtonState = BUTTON_DEFAULT_STATE; // État précédent du bouton
boolean buttonState = BUTTON_DEFAULT_STATE;     // État actuel du bouton
unsigned long pressStartTime = 0;               // Moment où le bouton a été pressé
unsigned long releaseTime = 0;                  // Moment où le bouton a été relâché
int clickCount = 0;
long lastClickTime = 0;

// --- PARAMETRES TIMER ---
// Définition de la plage horaire
int HEURE_DEBUT_MIAM = 7;
int MINUTE_DEBUT_MIAM = 30;
int HEURE_FIN_MIAM = 23;
int MINUTE_FIN_MIAM = 15;
boolean dansLaPlageHoraire = true;                    // Activer ou désactiver la plage horaire de nourrissage
const unsigned long FEED_DELAY_CROQUETTES_SEC = 2 * 60 * 60; // Délai minimum entre deux nourrissages (2 heures)
const unsigned long FEED_DELAY_CROQUINETTES_SEC = 60; // 30 * 60;   // Délai minimum entre deux nourrissages rapides (30 minutes)
const unsigned long SNOOZE_DELAY_SEC = 30;            // 30 * 60;              // Délai en cas de présence de croquettes (30 minutes)
unsigned long lastFeedTimeCroquettes = 0;             // Dernier temps (en secondes depuis minuit) où le chat a été nourri avec des croquettes
unsigned long lastFeedTimeCroquinettes = 0;           // Dernier temps (en secondes depuis minuit) où le chat a été nourri avec quelques croquinettes
unsigned long maintenantSec = 0;                      // heure actuelle (en secondes depuis minuit)
unsigned int compteurAbsenceChat = 0;                 // Nombre de reports de distributions de croquettes
unsigned int compteurDeCroquettes = 0;                // Nombre de distributions de croquettes pour ce jour
unsigned int compteurDeCroquinettes = 0;              // Nombre de distributions de croquinettes pour ce jour

// --- PARAMETRES FitCat ---
const int RATION_QUOTIDIENNE_G = 75; // Ration quotidienne (en g) idéale pour El Gazou
const int RATION_CROQUETTES_G = 5;   // Ration croquettes (en g) par distribution
const int RATION_CROQUINETTES_G = 1; // Ration croquinettes (en g) par distribution
int masseEngloutieParLeChatEnG = 0;
unsigned long delayDistributionCroquettesSec = FEED_DELAY_CROQUETTES_SEC;
#endif