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

// Capteur distance HC-SR04
#define ECHO_PIN 7
#define TRIGGER_PIN 8

// Servomoteur
#include <Servo.h>
#define SERVO_PIN 3
Servo monServomoteur;                       // Crée un objet Servo
const int ANGLE_OUVERTURE = 180;     // Angle pour ouvrir la valve
const int ANGLE_FERMETURE = 0;       // Angle pour fermer la valve
const unsigned int croquinettes = 111; // temps (ms) ouverture rapide
const unsigned int croquettes = 500;  // temps (ms) ouverture longue

// RTC Time
#include <RtcDS1302.h>
#define DS1302_CLK_PIN 6
#define DS1302_DAT_PIN 7
#define DS1302_RST_PIN 5
ThreeWire myWire(DS1302_DAT_PIN,DS1302_CLK_PIN,DS1302_RST_PIN); // IO, SCLK, CE
RtcDS1302<ThreeWire> myRTC(myWire);

/* Local time */
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// Timer variables
const unsigned long FEED_DELAY_SEC = 10 ;
RtcDateTime lastFeedtime;
unsigned long heureactuelle;


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
RtcDateTime getRtcTime();                                                          // Imprime le temps RTC dans la console
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
  preferences.begin("croquinator-settings", false);
  setupSPIFFS();
  // Configuration du WiFi
  setupWiFi();
  // Récupération de l'heure depuis le WiFi
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // NTP server config

  // Configuration de l'horloge interne  
 myRTC.Begin();
 

  // Configuration du capteur de distance
  pinMode(TRIGGER_PIN, OUTPUT); // Broche Trigger en sortie //
  pinMode(ECHO_PIN, INPUT);     // Broche Echo en entree //

  // Configuration du Servomoteur
  monServomoteur.attach(SERVO_PIN);      // Attache l'objet Servo à la broche D9 de l'Arduino
  monServomoteur.write(ANGLE_FERMETURE); // S'assure que la valve est fermée au démarrage

  // Just to know which program is running
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\n"));
}
// -------------------                INITIALISATION (fin)                ------------------- /

// -------------------                BOUCLE LOOP (début)                ------------------- /
void loop()
{
  Serial.println("Début de la Boucle principale");

  // Vérifier l'heure
  RtcDateTime maintenant = getRtcTime();
  // Si le chat est affamé : Verifier le délai depuis le dernier
  unsigned long deltaSecondes = maintenant.TotalSeconds() - lastFeedtime.TotalSeconds();
  
  if (deltaSecondes >= FEED_DELAY_SEC) // interval de temps
    { 
      feedCat(croquettes);
    }
  

      // Vérifier la distance
      // Si le chat est trop insistant
      //feedCat(); // Bonus: diminuer le temps d'ouverture pour un petit feed

      // Vérifier les boutons
      // Si un humain veut nourrir le chat
      // Verifier le délai depuis le dernier
      // Si OK
    
      //feedCat();
      //SI NON
      //printMessage("Attente...", "Prochain repas dans 2h", 5);

      delay(1000);
}
// -------------------                BOUCLE LOOP (fin)                ------------------- /

void feedCat(int timeOpen)
{
  Serial.println("Nourrir le chat !");
  // Bonus: Prévenir le chat avec un son ou une led

    // Ouvrir la valve pour laisser tomber les croquettes
  openValve(timeOpen);
  // Enregistrer l'heure de feed
  lastFeedtime = getRtcTime();
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

RtcDateTime getRtcTime()
{ 
RtcDateTime dt = myRTC.GetDateTime();
char datestring[26];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.println(datestring);
    
    return dt;
}

// Get time stamp
char* getWiFiTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
  }
  // Convert to string
  char timeStr[19];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
  // strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  return timeStr;
}

int calculateDistance()
{
  // Debut de la mesure avec un signal de 10 μS applique sur TRIG //
  digitalWrite(TRIGGER_PIN, LOW); // On efface l'etat logique de TRIG //
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH); // On met la broche TRIG a "1" pendant 10μS //
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW); // On remet la broche TRIG a "0" //
  // On mesure combien de temps le niveau logique haut est actif sur ECHO //
  const int duration = pulseIn(ECHO_PIN, HIGH);
  // Calcul de la distance grace au temps mesure //
  const int distance = duration * 0.034 / 2;

  // Affichage dans le moniteur serie de la distance mesuree //
  Serial.print("Distance mesuree : ");
  Serial.print(distance);
  Serial.println(" cm");

  return distance;
}
