#include <Arduino.h>

#include "time.h"

// Persistent memory
#include <Preferences.h>
Preferences preferences;
#ifdef ESP32
#include <SPIFFS.h>
#else
#include <Hash.h>
#include <FS.h>
#endif

// Wifi
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
const char *ssid = "La Parros";
const char *password = "LP48.100+";

// Ecran OLED
#include <Adafruit_SSD1306.h>
#define SCREEN_WIdtH 128     // Taille de l'écran OLED, en pixel, au niveau de sa largeur
#define SCREEN_HEIGHT 64     // Taille de l'écran OLED, en pixel, au niveau de sa hauteur
#define OLED_RESET_PIN -1    // Reset de l'OLED partagé avec l'Arduino (d'où la valeur à -1, et non un numéro de pin)
#define OLED_I2C_ADRESS 0x3C // Adresse de "mon" écran OLED sur le bus i2c (généralement égal à 0x3C ou 0x3D)
Adafruit_SSD1306 oled(SCREEN_WIdtH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

// Servomoteur
#include <Servo.h>
#define SERVO_PIN D3
Servo monServomoteur;                       // Crée un objet Servo
const int ANGLE_OUVERTURE = 180;     // Angle pour ouvrir la valve
const int ANGLE_FERMETURE = 0;       // Angle pour fermer la valve
const unsigned int croquinettes = 111; // temps (ms) ouverture rapide
const unsigned int croquettes = 500;  // temps (ms) ouverture longue

// RTC Time
#include "virtuabotixRTC.h"
#define DS1302_CLK_PIN D6
#define DS1302_DAT_PIN D7
#define DS1302_RST_PIN D5
virtuabotixRTC myRTC(DS1302_CLK_PIN, DS1302_DAT_PIN, DS1302_RST_PIN);

/* Local time */
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// Timer variables
const unsigned long FEED_DELAY_CROQUETTES_SEC = 2 * 60 * 60; // Délai minimum entre deux nourrissages (2 heures)
const unsigned long FEED_DELAY_CROQUINETTES_SEC = 30 * 60; // Délai minimum entre deux nourrissages rapides (30 minutes) 
unsigned long lastFeedtimeCroquettes = 0; // Dernier temps (en secondes depuis minuit) où le chat a été nourri avec des croquettes
unsigned long lastFeedtimecroquinettes = 0; // Dernier temps (en secondes depuis minuit) où le chat a été nourri avec quelques croquinettes
unsigned long maintenantSec = 0;

// Bouton poussoir
const int BOUTON = 4; // Pin du bouton poussoir
const int BoutonSec = 50; // Durée (ms) pour considérer une pression comme valide
int etatBouton;           // État actuel du bouton  

// Led
const int led = 13;      // Pin de la LED 
int etatled = LOW; // État actuel de la LED


// Logo (chat)
const int IMAGE_HEIGHT = 40;
const int IMAGE_WIdtH = 60;
const unsigned char IMAGE_CHAT[] PROGMEM = {
    // 'silhouette-cute-cat-peeking-vector-600nw-2593333797, 60x40px
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x1c, 0x00, 0x00,
    0x00, 0x00, 0xf8, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0xfc, 0x00, 0x00,
    0x00, 0x01, 0xfe, 0x00, 0x01, 0xfc, 0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x03, 0xfc, 0x00, 0x00,
    0x00, 0x01, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00,
    0x00, 0x01, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00,
    0x00, 0x01, 0xf9, 0x3f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x01, 0xf9, 0xbf, 0xf4, 0xfc, 0x00, 0x00,
    0x00, 0x03, 0xf7, 0xdf, 0xf6, 0x7c, 0x00, 0x00, 0x00, 0x03, 0xf7, 0xdf, 0xff, 0x7c, 0x00, 0x00,
    0x00, 0x07, 0xfb, 0xbf, 0xff, 0x7e, 0x00, 0x00, 0x00, 0x07, 0xf8, 0x39, 0xee, 0x7e, 0x00, 0x00,
    0x00, 0x0f, 0xff, 0xff, 0xf0, 0xfe, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
    0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
    0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
    0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
    0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80,
    0x1f, 0xfc, 0x00, 0x00, 0x00, 0x03, 0xff, 0x00, 0x0f, 0xf8, 0x00, 0x00, 0x00, 0x01, 0xfe, 0x00,
    0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// -------------------           DECLARATION DES FONCTIONS (début)           ------------------- /
void setupSPIFFS();                                                         // (setup) Connecte la mémoire persistante
void setupWiFi();                                                           // (setup) Connecte le wifi
void getSavedSettings();                                                    // (setup) Récupère la data de la mémoire persistante
void setupScreen();                                                         // (setup) Connecte l'écran OLED
void printMessage(char *title, char *message, unsigned int displayTimeSec); // Affiche un message sur l'écran OLED
void printImage(unsigned int displayTimeSec);                               // Affichage d'une image au centre de l'écran
void openValve(unsigned int timeOpen);                                      // Donne la nourriture
void getRtcTime();         
unsigned long getRtcSecondsFromMidnight();                                                 // Imprime le temps RTC dans la console
char* getWiFiTime();                                                       // Récupère la date depuis le wifi
int calculateDistance();                                                    // Mesure la distance du capteur
void feedCat(int timeOpen);
// -------------------           DECLARATION DES FONCTIONS (fin)           ------------------- /

// -------------------                INITIALISATION (début)                ------------------- /
void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ; // wait until Arduino Serial Monitor opens

  // Initialize persistent storage space
  //preferences.begin("croquinator-settings", false);
  //setupSPIFFS();
  // Configuration du WiFi
  //setupWiFi();
  // Récupération de l'heure depuis le WiFi
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // NTP server config

  // Configuration de l'horloge interne  
  myRTC.setDS1302Time(0, 58, 17, 4, 17, 12, 2020); // sec min h, j semaine, j mois, mois, année
 

  // Configuration du Servomoteur
  monServomoteur.attach(SERVO_PIN);      // Attache l'objet Servo à la broche D9 de l'Arduino
  monServomoteur.write(ANGLE_FERMETURE); // S'assure que la valve est fermée au démarrage

  // Configuration du bouton poussoir
  pinMode(BOUTON, INPUT);   // le bouton est une entrée
   etatBouton = HIGH; // on initialise l'état du bouton comme "relaché"

  // Configuration de la LED
    pinMode(led, OUTPUT); // la led est une sortie

  // Just to know which program is running
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\n"));
}
// -------------------                INITIALISATION (fin)                ------------------- /

// -------------------                BOUCLE LOOP (début)                ------------------- /
void loop()
{
  Serial.println("Début de la Boucle principale");
// fonction principale
  // Vérifier l'heure
  maintenantSec = getRtcSecondsFromMidnight();
  // Si le chat est affamé : Verifier le délai depuis le dernier
  unsigned long deltaSecondes = maintenantSec - lastFeedtimeCroquettes;  // interval de temps
  
  if (deltaSecondes >= FEED_DELAY_CROQUETTES_SEC) // Si le délai de 2H est écoulé
    { 
      // Vérifier la distance
      feedCat(croquettes);      // Nourrir le chat avec une portion complète
      lastFeedtimeCroquettes= maintenantSec; // Met à jour le dernier temps de nourrissage
    }
  

      // Vérifier les boutons
 etatBouton = digitalRead(BOUTON); // Lecture de l'état du bouton

 if (etatBouton == HIGH) 
 {
Serial.println("Le bouton est appuyé");
digitalWrite(led, HIGH); // Allume la LED

  unsigned long deltaSecondes = maintenantSec - lastFeedtimecroquinettes;  // interval de temps
  
  if (deltaSecondes >= FEED_DELAY_CROQUINETTES_SEC) // Si le délai de 30 min est écoulé
    { 
      feedCat(croquinettes);      // Nourrir le chat avec quelques croquettes
      lastFeedtimecroquinettes= maintenantSec; // Met à jour le dernier temps de gourmandise
    }
delay(BoutonSec); // Anti-rebond
} 
else 
{
Serial.println("Le bouton n'est pas appuyé");

digitalWrite(led, LOW); // Éteint la LED}

char message[56];
const unsigned int deltaMinutes = deltaSecondes / 60;
sprintf(message, "Dernières Croquinettes il y a %d min", deltaMinutes);

      delay(1000);
}
}
// -------------------                BOUCLE LOOP (fin)                ------------------- /

void feedCat(int timeOpen)
{
  Serial.println("Nourrir le chat !");
  // Bonus: Prévenir le chat avec un son ou une led

    // Ouvrir la valve pour laisser tomber les croquettes
  openValve(timeOpen);
  // Enregistrer l'heure de feed
  // preferences.putULong("feedtime", value);
  // Bonus: Afficher un message sur ecran
};

// -------------------                FONCTIONS                ------------------- /
// Initialize SPIFFS (mémoire persistante)
void setupSPIFFS()
{
  if (!SPIFFS.begin())
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else
  {
    Serial.println("SPIFFS mounted successfully");
  }
}
void getSavedSettings()
{
  //lastFeedtime = preferences.getUShort("feedtime", 0); // 0 par défaut
}
// Initialize WiFi
void setupWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
}

// Initialisation de l'écran OLED
void setupScreen()
{
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADRESS))
  {
    Serial.println(F("Erreur de communication avec le chipset SSD1306… arrêt du programme."));
    while (1)
      ; // Arrêt du programme (boucle infinie)
  }
  else
  {
    Serial.println(F("Initialisation du contrôleur SSD1306 réussi."));
  }
  printImage(1);
}
void printMessage(char *title, char *message, unsigned int displayTimeSec)
{
  oled.clearDisplay();      // Vidange du buffer de l'écran OLED
  oled.setTextColor(WHITE); // Couleur "blanche" (ou colorée, si votre afficheur monochrome est bleu, jaune, ou bleu/jaune)

  oled.setCursor(0, 1); // Positionnement du curseur dans l'angle haut-gauche, avec décalage de 1 pixel vers le bas
  oled.setTextSize(2);  // Sélection de l'échelle 1:1
  oled.println(title);
  oled.println();
  oled.setTextSize(1); // Sélection de l'échelle 1:1
  oled.println(message);

  oled.display();
  delay(displayTimeSec * 1000);
  oled.clearDisplay();
}
// Affichage d'une image au centre de l'écran
void printImage(unsigned int displayTimeSec)
{
  oled.clearDisplay();
  oled.drawBitmap(
      (oled.width() - IMAGE_WIdtH) / 2,   // Position de l'extrême "gauche" de l'image (pour centrage écran, ici)
      (oled.height() - IMAGE_HEIGHT) / 2, // Position de l'extrême "haute" de l'image (pour centrage écran, ici)
      IMAGE_CHAT,
      IMAGE_WIdtH,
      IMAGE_HEIGHT,
      WHITE); // "couleur" de l'image

  oled.display();

  delay(displayTimeSec * 1000);
  oled.clearDisplay();
}

void openValve(unsigned int timeOpen)
{
  Serial.print("Ouverture de la valve ");
  Serial.print(timeOpen);
  Serial.println(" ms).");

  monServomoteur.write(ANGLE_OUVERTURE); // Ouvre
  delay(timeOpen);                // Attend 0,5 seconde pour laisser tomber la portion
  monServomoteur.write(ANGLE_FERMETURE); // Ferme
}

void getRtcTime()
{ 
  myRTC.updateTime();

  Serial.print("Date / Heure: ");
  Serial.print(myRTC.dayofmonth);
  Serial.print("/");
  Serial.print(myRTC.month);
  Serial.print("/");
  Serial.print(myRTC.year);
  Serial.print(" ");
  Serial.print(myRTC.hours);
  Serial.print(":");
  Serial.print(myRTC.minutes);
  Serial.print(":");
  Serial.println(myRTC.seconds);
}
unsigned long getRtcSecondsFromMidnight()
{
  myRTC.updateTime();
  unsigned long totalSeconds = myRTC.hours * 3600 + myRTC.minutes * 60 + myRTC.seconds;
  return totalSeconds;
}

// // Get time stamp
// char* getWiFiTime()
// {
//   struct tm timeinfo;
//   if (!getLocalTime(&timeinfo))
//   {
//     Serial.println("Failed to obtain time");
//   }
//   // Convert to string
//   char timeStr[19];
//   strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
//   // strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
//   return 'z';
// }