#include <Arduino.h>
#include "time.h"
#include <Adafruit_SSD1306.h> // Ecran OLED
#include <Servo.h>            // Servomoteur
#include "virtuabotixRTC.h"   // RTC module 2
#include <Preferences.h>      // Persistent memory
#ifdef ESP32
#include <WiFi.h>
#include <SPIFFS.h>
#else
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <FS.h>
#endif

// --- INCLUSIONS PERSO ---
#include "config.h" //  pins et constantes
#include "images.h" //  image de chat

// --- OBJETS GLOBAUX ---
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN); // Ecran OLED
virtuabotixRTC myRTC(DS1302_CLK_PIN, DS1302_DAT_PIN, DS1302_RST_PIN);      // RTC module 2
Servo monServomoteur;                                                      // Servomoteur
Preferences preferences;                                                   // Persistent memory

// Timer variables
const unsigned long FEED_DELAY_CROQUETTES_SEC = 2 * 60 * 60; // Délai minimum entre deux nourrissages (2 heures)
const unsigned long FEED_DELAY_CROQUINETTES_SEC = 30 * 60;   // Délai minimum entre deux nourrissages rapides (30 minutes)
unsigned long lastFeedtimeCroquettes = 0;                    // Dernier temps (en secondes depuis minuit) où le chat a été nourri avec des croquettes
unsigned long lastFeedtimecroquinettes = 0;                  // Dernier temps (en secondes depuis minuit) où le chat a été nourri avec quelques croquinettes
unsigned long maintenantSec = 0;
unsigned long duréeappuiBoutonSec = 0;
unsigned long debutappuiBoutonSec  =0;
// Définition de la plage horaire
int startHour   = 7;
int startMinute = 30;

int endHour     = 23;
int endMinute   = 15;

// Bouton poussoir
const int BOUTON = D8;    // Pin du bouton poussoir
const int BoutonSec = 50; // Durée (ms) pour considérer une pression comme valide
int etatBouton;           // État actuel du bouton

// Led
const int led = 13; // Pin de la LED
int etatled = LOW;  // État actuel de la LED

// -------------------           DECLARATION DES FONCTIONS (début)           ------------------- /
void setupSPIFFS();                                                         // (setup) Connecte la mémoire persistante
void setupWiFi();                                                           // (setup) Connecte le wifi
void getSavedSettings();                                                    // (setup) Récupère la data de la mémoire persistante
void setupScreen();                                                         // (setup) Connecte l'écran OLED
void printMessage(char *title, char *message, unsigned int displayTimeSec); // Affiche un message sur l'écran OLED
void printImage(unsigned int displayTimeSec);                               // Affichage d'une image au centre de l'écran
void openValve(unsigned int timeOpen);                                      // Donne la nourriture
void getRtcTime();
unsigned long getRtcSecondsFromMidnight(); // Imprime le temps RTC dans la console
char *getWiFiTime();                       // Récupère la date depuis le wifi
int calculateDistance();                   // Mesure la distance du capteur
void feedCat(int timeOpen);
void plageHoraire (int h, int m);           // Définit la plage horaire de nourrissage
// -------------------           DECLARATION DES FONCTIONS (fin)           ------------------- /

// -------------------                INITIALISATION (début)                ------------------- /
void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ; // wait until Arduino Serial Monitor opens

  // Initialisation de l'écran OLED
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

  // Initialize persistent storage space
  // preferences.begin("croquinator-settings", false);
  // setupSPIFFS();
  // Configuration du WiFi
  // setupWiFi();
  // Récupération de l'heure depuis le WiFi
  // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // NTP server config

  // Configuration de l'horloge interne  
  myRTC.setDS1302Time(0, 58, 17, 4, 17, 12, 2020); // sec min h, j semaine, j mois, mois, année

  // Configuration du Servomoteur
  monServomoteur.attach(SERVO_PIN);      // Attache l'objet Servo à la broche D9 de l'Arduino
  monServomoteur.write(ANGLE_FERMETURE); // S'assure que la valve est fermée au démarrage

  // Configuration du bouton poussoir
  pinMode(BOUTON, INPUT); // le bouton est une entrée
  etatBouton = HIGH;      // on initialise l'état du bouton comme "relaché"

  // Configuration de la LED
  pinMode(led, OUTPUT); // la led est une sortie

  // Confirguration du capteur ir
  pinMode(IR_PIN, INPUT); // le capteur ir est une entrée

  // Just to know which program is running
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\n"));
}
// -------------------                INITIALISATION (fin)                ------------------- /

// -------------------                BOUCLE LOOP (début)                ------------------- /
void loop()
{
  Serial.println("Début de la Boucle principale");

  int h = myRTC.hours;
  int m = myRTC.minutes;
  
  if ((h > startHour || (h == startHour && m >= startMinute)) &&
        (h < endHour || (h == endHour && m <= endMinute))) {
      Serial.println("Dans la plage horaire de nourrissage.");


  // fonction principale
  // Vérifier l'heure
  maintenantSec = getRtcSecondsFromMidnight();
  Serial.println(maintenantSec);
  // Si le chat est affamé : Verifier le délai depuis le dernier
  unsigned long deltaSecondes = maintenantSec - lastFeedtimeCroquettes; // interval de temps
  Serial.println(deltaSecondes);
  if (deltaSecondes >= FEED_DELAY_CROQUETTES_SEC) // Si le délai de 2H est écoulé
  {
    // Vérifier la distance
    boolean PrésencedeCroquettes = digitalRead(IR_PIN); // Lire l'état du capteur IR
    if (PrésencedeCroquettes == HIGH ) // Si des croquettes sont présentes
  { 
     Serial.println("Des croquetttes sont dans la gamelle");
     delay(60*1000*60); //Attendre 1 heure pour vérifier à nouveau si Gazou a mangé avant de lui donner à nouveau
}
else
// Nourrir el Gazou
{
 Serial.println("Pas de croquetttes dans la gamelle");

    feedCat(croquettes);                    // Nourrir le chat avec une portion complète
    lastFeedtimeCroquettes = maintenantSec; // Met à jour le dernier temps de nourrissage
   preferences.putULong("lastFeedtimecroquinettes", lastFeedtimecroquinettes); // Sauvegarder le temps de nourrissage dans la mémoire persistante
   Serial.println("El Gazou a eu sa dose");
}
  }

  // Vérifier les boutons
  boolean etatBouton = digitalRead(BOUTON); // Lecture de l'état du bouton

  if (etatBouton == LOW) // Si le bouton est appuyé
  {
    Serial.println("Le bouton est appuyé");
     boolean led = digitalRead(led);
     Serial.println("Allumage de la led");
    digitalWrite(led, HIGH); // Allume la LED
   debutappuiBoutonSec = maintenantSec; // Enregistre le temps de début de l'appui sur le bouton
   duréeappuiBoutonSec = (maintenantSec - debutappuiBoutonSec); // Calcul du temps d'appui sur le bouton en secondes
    
   if (duréeappuiBoutonSec <= 1) // Si le bouton est appuyé pendant moins de 1 seconde
   {
    Serial.println("appui court détecté");
    printMessage("super affichage S","affichage S a ajouter", 3);    // Afficher les dernières croquettes et croquinettes servies
   }

   if (duréeappuiBoutonSec > 1)   // Si le bouton est appuyé pendant plus de 1 seconde
  {
    Serial.println("appui long détecté");
    unsigned long deltaSecondes = maintenantSec - lastFeedtimecroquinettes; // interval de temps

    if (deltaSecondes >= FEED_DELAY_CROQUINETTES_SEC) // Si le délai de 30 min est écoulé
    {
      Serial.println("Délai écoulé, on peut donner une gourmandise/crpquinette");
      feedCat(croquinettes);                    // Nourrir le chat avec quelques croquettes
      lastFeedtimecroquinettes = maintenantSec; // Met à jour le dernier temps de gourmandise
      preferences.putULong("lastFeedtimecroquinettes", lastFeedtimecroquinettes); // Sauvegarder le temps de nourrissage dans la mémoire persistante
      Serial.println("El gazou est servi !");
    }
    delay(BoutonSec); // Anti-rebond
  }
  else
  {
    digitalWrite(led, LOW); // Éteint la LED}
    Serial.println("Extinction de la led");
    printImage(4);  // Afficher l'image du chat

    char message[56];                                                       // Nombre de caractères max pour le message
    unsigned long deltaSecondes = maintenantSec - lastFeedtimecroquinettes; // interval de temps entre maintenant et la derniere gourmandise
    const unsigned int deltaMinutes = deltaSecondes / 60;                   // conversion en minutes
    sprintf(message, "Dernières Croquinettes il y a %d min", deltaMinutes); // Prépare le message à afficher

    delay(1000);
  }
  }
}
}
// -------------------                BOUCLE LOOP (fin)                ------------------- /

void feedCat(int timeOpen)
{
  Serial.println("Nourrir le chat !");
  // Bonus: Prévenir le chat avec un son ou une led
openValve(timeOpen);   // Ouvrir la valve pour laisser tomber les croquettes
}
  
  

 
  // Bonus: Afficher un message sur ecran
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
  // lastFeedtime = preferences.getUShort("feedtime", 0); // 0 par défaut
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
  delay(timeOpen);                       // Attend 0,5 seconde pour laisser tomber la portion
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