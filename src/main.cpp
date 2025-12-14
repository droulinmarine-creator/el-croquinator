#include <Arduino.h>
#include <Adafruit_SSD1306.h> // Ecran OLED
#include <Servo.h>            // Servomoteur
#include "virtuabotixRTC.h"   // RTC module 2
#include <Preferences.h>      // Persistent memory
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

// --- INCLUSIONS PERSO ---
#include "config.h" //  pins, constantes et variables globales
#include "images.h" //  image de chat

// --- OBJETS GLOBAUX ---
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN); // Ecran OLED
virtuabotixRTC myRTC(DS1302_CLK_PIN, DS1302_DAT_PIN, DS1302_RST_PIN);      // RTC module 2
Servo monServomoteur;                                                      // Servomoteur
Preferences preferences;                                                   // Persistent memory

// -------------------           DECLARATION DES FONCTIONS (début)           ------------------- /                                                           // (setup) Connecte la mémoire persistante
void setupWiFi();                                                                       // (setup) Connecte le wifi
void getSavedSettings();                                                                // (setup) Récupère la data de la mémoire persistante
void setupScreen();                                                                     // (setup) Connecte l'écran OLED
void manageDisplay();                                                                   // (loop) gère les temps d'affichage sur l'écran oled
void printMessage(const char *title, const char *message, unsigned int displayTimeSec); // Affiche un message sur l'écran OLED
void printImage(unsigned int displayTimeSec);                                           // Affichage d'une image au centre de l'écran
void displayScreen(unsigned int displayTimeSec);                                        // Affiche l'écran de bord
void reinitialiserCompteurs();                                                          // Réinitialise les compteurs
boolean verifierPlageHoraire();                                                         // Vérifie l'heure, retourne (0) en dehors || (1) dans la plage horaire
int calculerMasseEngloutie();
boolean verifierRegime();
boolean detecterCroquettes();                                                  // Détecte la présence de croquette, retourne (0) abscence || (1) présence
void feedCat(boolean grossePortion);                                           // Distribue les (0) Croquinettes || (1) Croquettes
void openValve(unsigned int timeOpen);                                         // Contrôle le servomoteur
void syncRTCFromWiFi();                                                        // Synchronise la date RTC avec la date WiFi
unsigned long getRtcSecondsFromMidnight();                                     // Retourne le nombre de secondes depuis minuit
void convertSecondsFromMidnightToTime(unsigned long seconds, char *outBuffer); // Convertit des secondes en heure HH:MM (fournir le buffer de sortie)
void printWiFiTime();                                                          // (debug) Imprime la date wifi
void getRtcTime();                                                             // (debug) Imprime le temps RTC dans la console
// -------------------           DECLARATION DES FONCTIONS (fin)           ------------------- /

// -------------------                INITIALISATION (début)                ------------------- /
void setup()
{
  DEBUG_INIT(9600);                                             // Initialisation de la communication filaire                                              // wait until Arduino Serial Monitor opens
  DEBUG_PRINTLN(F("START " __FILE__ " from " __DATE__ "\r\n")); //  Just to know which program is running

  setupScreen();                                               // Initialisation de l'écran OLED
  getSavedSettings();                                          // Récupération de la mémoire persistante
  setupWiFi();                                                 // Configuration du WiFi
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER); // NTP server config
  printWiFiTime();                                             // Récupération de l'heure depuis le WiFi
  syncRTCFromWiFi();                                           // Syncrhonisation de l'horloge interne
  monServomoteur.attach(SERVO_PIN);                            // Configuration du Servomoteur
  monServomoteur.write(ANGLE_FERMETURE);                       // S'assure que la valve est fermée au démarrage
  pinMode(BOUTON_PIN, INPUT);                           // Configuration du bouton poussoir
}
// -------------------                INITIALISATION (fin)                ------------------- /

// -------------------                BOUCLE LOOP (début)                ------------------- /
void loop()
{
  // DEBUG_PRINTLN("Début de la Boucle principale");
  // getRtcTime();

  myRTC.updateTime();                          // Always update time
  maintenantSec = getRtcSecondsFromMidnight(); // Vérifier l'heure

  // --------- AutoCatFeed (début) --------- //
  dansLaPlageHoraire = verifierPlageHoraire(); // Vérifie la plage horaire et réinitialise les compteurs
  if (dansLaPlageHoraire == true)
  {
    // DEBUG_PRINTLN(maintenantSec);
    //  Si le chat est affamé : Verifier le délai depuis le dernier croq
    long deltaSecondes = maintenantSec - lastFeedTimeCroquettes; // interval de temps
    const unsigned long delay = delayDistributionCroquettesSec + compteurAbsenceChat * SNOOZE_DELAY_SEC;
    // char message[56];
    // sprintf(message, "Maintenant: %d s - lastFeed: %d s - delta: %d s - delay: %d s", maintenantSec, lastFeedTimeCroquettes, deltaSecondes, delay);
    // DEBUG_PRINTLN(message);
    if (deltaSecondes > 0 && (unsigned int)deltaSecondes >= delay) // Si le délai de 2H est écoulé
    {
      feedCat(1); // Donner des croquettes
    }
  }
  // --------- AutoCatFeed (fin) --------- //

  // --------- Inputs bouton (debut) --------- //
  // 1. Lecture de l'état actuel du bouton
  int reading = digitalRead(BOUTON_PIN);
  if (reading != buttonState)
  {                        // on vient d'appuyer sur le bouton
    buttonState = reading; // mettre à jour 'buttonState'

    // 3. Gestion de l'événement PUSH (Appui)
    if (buttonState == !BUTTON_DEFAULT_STATE)
    {
      pressStartTime = millis();
      DEBUG_PRINTLN("Le bouton est appuye");
      //  Réinitialiser le compte de clic si le délai est dépassé
      if (millis() - lastClickTime > DOUBLE_CLICK_TIMEOUT_MS)
      {
        clickCount = 0;
      }
    }
    // 4. Gestion de l'événement RELEASE (Relâchement)
    else
    { // on vient de relacher le bouton
      releaseTime = millis();
      unsigned long pressDuration = releaseTime - pressStartTime;
      DEBUG_PRINT("-> BOUTON RELACHE (Duree: ");
      DEBUG_PRINT(pressDuration);
      DEBUG_PRINTLN("ms)");

      // Détection de l'appui LONG
      if (pressDuration >= LONG_PRESS_MIN_MS * 2)
      {
        // CAS n°-1 : Appui TRES TRES LONG
        DEBUG_PRINTLN("--- APPUIS TRES TRES LONG DETECTE ---");
        reinitialiserCompteurs();
      }
      else if (pressDuration >= LONG_PRESS_MIN_MS)
      {
        // CAS n°0 : Appui TRES LONG
        DEBUG_PRINTLN("--- APPUIS TRES LONG DETECTE ---");
        syncRTCFromWiFi();
      }
      else if (pressDuration >= SHORT_PRESS_MAX_MS)
      {
        // CAS n°1 : Appui LONG
        DEBUG_PRINTLN("--- APPUIS LONG DETECTE ---");
        clickCount = 0; // Annule tout double-clic potentiel
        // Affiche le temps restant avant les prochaines croquettes
        char message[81]; // Nombre de caractères max pour le message
        const unsigned int deltaMinutes = (lastFeedTimeCroquinettes + FEED_DELAY_CROQUINETTES_SEC - maintenantSec) / 60;
        sprintf(message, "El Gazou a englouti %d/%dg de croquettes. Prochaines croquinettes disponibles dans %d min.", masseEngloutieParLeChatEnG, RATION_QUOTIDIENNE_G, deltaMinutes);
        printMessage("Autocroq", message, DISPLAY_TIME_SEC);
      }
      // Détection de l'appui COURT (potentiel simple ou double clic)
      else
      {
        clickCount++;
        lastClickTime = millis(); // Enregistre le moment du clic pour le timeout
      }
    }
  }
  // 5. Détection du DOUBLE-CLIC ou du SIMPLE-CLIC
  // On vérifie s'il y a eu un ou plusieurs clics, et si le délai de double-clic est écoulé
  if (clickCount > 0 && (millis() - lastClickTime > DOUBLE_CLICK_TIMEOUT_MS))
  {
    if(clickCount==3) 
    // CAS n°1 : Triple click ou plus : Distribution dose normal de croquettes
    {
      feedCat(1);
    }
    else if(clickCount==2)
    {  //CAS n °2 : Double clic : Distribution de croquinettes
       DEBUG_PRINTLN("--- DOUBLE APPUIS DETECTE ---");
      feedCat(0);     // Donner des croquinettes
         }
    else if(clickCount==1)
    {
       // CAS n°3 : Simple clic : Affichage écran 
        DEBUG_PRINTLN("--- APPUIS COURT (SIMPLE CLIC) DETECTE ---");
        displayScreen(DISPLAY_TIME_SEC); // Afficher les dernières croquettes et croquinettes servies
    }
    clickCount = 0; // Réinitialisation
  }
  // Stocke l'état pour la prochaine itération
  lastButtonState = reading;
  // --------- Inputs bouton (fin) --------- //

  manageDisplay(); // Loop Ecran OLED
  delay(1);        // Délais de fin de boucle
}
// -------------------                BOUCLE LOOP (fin)                ------------------- /

// -------------------       FONCTIONS: Nourir le chat (début)       ------------------- /
void reinitialiserCompteurs()
{
  DEBUG_PRINTLN("Reinitialisation des compteurs.");
  compteurDeCroquettes = 0;
  compteurDeCroquinettes = 0;
  compteurAbsenceChat = 0;
  lastFeedTimeCroquettes = ((HEURE_DEBUT_MIAM * 60 + MINUTE_DEBUT_MIAM) * 60) - FEED_DELAY_CROQUETTES_SEC;
  lastFeedTimeCroquinettes = 0;
  delayDistributionCroquettesSec = FEED_DELAY_CROQUETTES_SEC;

  preferences.begin("croquinator", true);
  lastFeedTimeCroquettes = preferences.putULong("croquetteTime", lastFeedTimeCroquettes);
  lastFeedTimeCroquinettes = preferences.putULong("croquinetteTime", lastFeedTimeCroquinettes);
  compteurDeCroquettes = preferences.putUInt("compteurCroquette", compteurDeCroquettes);
  compteurDeCroquinettes = preferences.putUInt("compteurCroquinette", compteurDeCroquinettes);
  preferences.end(); // Ferme l'accès à la mémoire. C'est CRUCIAL.

  DEBUG_PRINTLN("Compteurs reinitialises.");
  printMessage("Compteurs", "Reinitialisation des compteurs.", DISPLAY_TIME_SEC);
}
boolean verifierPlageHoraire()
{
  myRTC.updateTime();
  int h = myRTC.hours;
  int m = myRTC.minutes;
  int s = myRTC.seconds;
  // Tout les jours à minuit
  if (h == 0 && m == 0 && s == 0)
  {
    syncRTCFromWiFi(); // Resynchroniser l'horloge
    reinitialiserCompteurs();
  }
  // Vérifier la plage horaire
  if ((h > HEURE_DEBUT_MIAM || (h == HEURE_DEBUT_MIAM && m >= MINUTE_DEBUT_MIAM)) &&
      (h < HEURE_FIN_MIAM || (h == HEURE_FIN_MIAM && m <= MINUTE_FIN_MIAM)))
  {
    // DEBUG_PRINTLN("Dans la plage horaire de nourrissage.");
    return true; // Nous somme dans la plage horaire
  }
  else
  { // Nous ne somme pas dans la plage horaire
    // DEBUG_PRINTLN("En dehors de la plage horaire de nourrissage.");
    return false;
  }
}
int calculerMasseEngloutie()
{
  return compteurDeCroquettes * RATION_CROQUETTES_G + compteurDeCroquinettes * RATION_CROQUINETTES_G;
};
void optimiserDelayDistributionCroquettes(){
  masseEngloutieParLeChatEnG = calculerMasseEngloutie();
  const long finDeLaPlageDansSec = ((HEURE_FIN_MIAM * 60 + MINUTE_FIN_MIAM) * 60) - maintenantSec;
  const int nombreDistributionCroquettesRestant = (RATION_CROQUETTES_G - masseEngloutieParLeChatEnG) / RATION_CROQUETTES_G;
  if(finDeLaPlageDansSec > 0 && nombreDistributionCroquettesRestant > 0){
   delayDistributionCroquettesSec = finDeLaPlageDansSec / nombreDistributionCroquettesRestant;
   DEBUG_PRINTLN("Délai de distribution des croquettes ajusté");
   DEBUG_PRINT("Temps restant en heure : ");
   char timeBuffer[6];
   convertSecondsFromMidnightToTime(finDeLaPlageDansSec, timeBuffer);
   DEBUG_PRINTLN(timeBuffer);
   DEBUG_PRINT("Nombre de distributions restantes: ");
   DEBUG_PRINTLN(nombreDistributionCroquettesRestant);
   DEBUG_PRINT("Nouveau délai (min): ");
   DEBUG_PRINTLN(delayDistributionCroquettesSec / 60);

    }
}
boolean verifierRegime()
{
  DEBUG_PRINT("Verification du régime du chat... ");
  masseEngloutieParLeChatEnG = calculerMasseEngloutie();
  DEBUG_PRINT(masseEngloutieParLeChatEnG);
  if (masseEngloutieParLeChatEnG < RATION_QUOTIDIENNE_G)
  {
    DEBUG_PRINTLN("g engloutis. El Gazou respecte son régime.");
    return true;
  }
  else
  {
    DEBUG_PRINTLN("g engloutis. El Gazou a suffisamment mangé !");
    return false;
  }
}
boolean detecterCroquettes()
{
  const boolean presence = digitalRead(IR_PIN);
  DEBUG_PRINT("Detection des croquettes... ");
  if (presence == LOW)
  {
    DEBUG_PRINTLN("Des croquetttes sont dans la gamelle");
    return true;
  }
  else
  {
    DEBUG_PRINTLN("Pas de croquetttes dans la gamelle");
    return false;
  }
}
void openValve(unsigned int timeOpen)
{
  DEBUG_PRINT("Ouverture de la valve (");
  DEBUG_PRINT(timeOpen);
  DEBUG_PRINTLN(" ms).");

  monServomoteur.write(ANGLE_OUVERTURE); // Ouvre
  delay(timeOpen);                       // Attend 0,5 seconde pour laisser tomber la portion
  monServomoteur.write(ANGLE_FERMETURE); // Ferme
}
/* Fonction pour nourrir le chat
Vérifie la présence de croquettes et les distribue
@args
grossePortion : true (croquettes) | false (croquinettes)
*/
void feedCat(boolean grossePortion)
{
  DEBUG_PRINTLN("Nourrir le chat !");
  const boolean presenceDeCroquettes = detecterCroquettes(); // Vérifier si il y a des croquettes
  const boolean leRegimeEstRespecte = verifierRegime();      // Vérifier la quantité engloutée

  // CAS n°1 - Il y a déja des croquettes
  if (presenceDeCroquettes == true)
  {
    if (grossePortion == true)
    { // Croquettes
      DEBUG_PRINTLN("Distribution des croquettes reportee");
      compteurAbsenceChat++;
      printMessage("No gazou", "Gazou est absent, distribution des croquettes reportee de 30min..", DISPLAY_TIME_SEC);
    }
    else
    { // Croquinettes
      DEBUG_PRINTLN("Pas de croquinettes pour les chats qui ne mangent pas");
      printMessage("No way", "Il y a deja des croquettes dans la gamelles !", DISPLAY_TIME_SEC);
    }
  }
  // Fin du CAS n°1 - Il y a déja des croquettes

  // CAS n°2 - Le régime n'est pas respecté
  else if (leRegimeEstRespecte == false)
  {
    printMessage("No Grazou", "Distribution annulee. Gazou a suffisamment mange aujourd'hui !", DISPLAY_TIME_SEC);
  }
  // Fin du CAS n°2 - Le régime n'est pas respecté

  // CAS n°3 - Il n'y a pas de croquettes et le régime est respecté
  else
  {
    DEBUG_PRINT("Distribution ");
    printImage(DISPLAY_TIME_SEC); // Afficher l'image du chat
    // Bonus: Prévenir le chat avec un son ou une led

    // CAS n°3a - Croquettes
    if (grossePortion == true)
    {
      DEBUG_PRINTLN(" des croquettes.");
      openValve(CROQUETTES);                  // Nourrir le chat avec une portion complète
      lastFeedTimeCroquettes = maintenantSec; // Met à jour le dernier temps de nourrissage
      compteurDeCroquettes++;                 // Mise à jour du compteur de croquettes
      optimiserDelayDistributionCroquettes();
      DEBUG_PRINTLN("Reinitialisation du compteur d'absence.");
      compteurAbsenceChat = 0;

      // Sauvegarder dans la mémoire persistante
      preferences.begin("croquinator", false);
      preferences.putULong("croquetteTime", lastFeedTimeCroquettes);
      preferences.putUInt("compteurCroquette", compteurDeCroquettes);
      preferences.end(); // Ferme l'accès à la mémoire. C'est CRUCIAL.

      printMessage("Miam", "El Gazou a eu sa dose", DISPLAY_TIME_SEC);
      DEBUG_PRINTLN("El Gazou a eu sa dose");
    }

    // CAS n°3b - Croquinettes
    else
    {
      DEBUG_PRINTLN(" des croquinettes.");
      // Vérifier que le delay des croquinettes est dépassé
      unsigned long deltaSecondes = maintenantSec - lastFeedTimeCroquinettes; // interval de temps
      if (deltaSecondes >= FEED_DELAY_CROQUINETTES_SEC)                       // Si le délai de 30 min est écoulé
      {
        DEBUG_PRINTLN("Délai écoulé, on peut donner une gourmandise/croquinette");
        openValve(CROQUINETTES);                  // Nourrir le chat avec quelques croquettes
        lastFeedTimeCroquinettes = maintenantSec; // Met à jour le dernier temps de gourmandise
        compteurDeCroquinettes++;                 // Mise à jour du compteur de croquinettes
        optimiserDelayDistributionCroquettes();

        // Sauvegarder dans la mémoire persistante
        preferences.begin("croquinator", false);
        preferences.putULong("croquinetteTime", lastFeedTimeCroquinettes);
        preferences.putUInt("compteurCroquinette", compteurDeCroquinettes);
        preferences.end(); // Ferme l'accès à la mémoire. C'est CRUCIAL.

        DEBUG_PRINTLN("El gazou est servi !");
        printMessage("Miaou", "El Gazou est servi !", DISPLAY_TIME_SEC);
      }
      else
      {
        DEBUG_PRINTLN("El gazou a deja eu sa gourmandise.");
        char message[56];                                                       // Nombre de caractères max pour le message
        const unsigned int deltaMinutes = deltaSecondes / 60;                   // conversion en minutes
        sprintf(message, "Dernieres Croquinettes il y a %d min", deltaMinutes ); // Prépare le message à afficher
        printMessage("No way", message, DISPLAY_TIME_SEC);
      }
    }
  }
  // Fin du CAS n°3 - Il n'y a pas de croquettes et le régime est respecté
};
// -------------------       FONCTIONS: Nourir le chat (fin)       ------------------- /

// -------------------       FONCTIONS: Setup mémoire et WiFi (début)       ------------------- /
void getSavedSettings()
{
  preferences.begin("croquinator", true);
  lastFeedTimeCroquettes = preferences.getULong("croquetteTime", 0);     // 0 par défaut
  lastFeedTimeCroquinettes = preferences.getULong("croquinetteTime", 0); // 0 par défaut
  compteurDeCroquettes = preferences.getUInt("compteurCroquette", 0);
  compteurDeCroquinettes = preferences.getUInt("compteurCroquinette", 0);
  preferences.end(); // Ferme l'accès à la mémoire. C'est CRUCIAL.
  printMessage("Memory", "Donnees recuperees depuis la memoire", DISPLAY_TIME_SEC);

  // DEBUG_PRINTLN("Données récupérées depuis la mémoire :");
  // char message[50];
  // sprintf(message, "Croquettes   : %02d - lastTime: %lu", compteurDeCroquettes, lastFeedTimeCroquettes);
  // DEBUG_PRINTLN(message);
  // sprintf(message, "Croquinettes : %02d - lastTime: %lu", compteurDeCroquinettes, lastFeedTimeCroquinettes);
  // DEBUG_PRINTLN(message);
}
void setupWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    DEBUG_PRINT("Connecting to WiFi ..");
    printMessage("WiFi", "Connexion au WiFi...", DISPLAY_TIME_SEC);
    while (WiFi.status() != WL_CONNECTED)
    {
      DEBUG_PRINT('.');
      delay(1000);
    }
  }
  else
  {
    DEBUG_PRINTLN("WiFi connected.");
    printMessage("WiFi", "WiFi connected.", DISPLAY_TIME_SEC);
  }
}
void printWiFiTime() // (debug) Imprimer la date du WiFi
{
  DEBUG_PRINTLN("Récupération de la date depuis le wifi...");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    DEBUG_PRINTLN("Failed to obtain time");
  }
  // Convert to string
  char timeStr[19];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
  // strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  DEBUG_PRINT("Date actuelle: ");
  DEBUG_PRINTLN(timeStr);
}
// -------------------       FONCTIONS: Setup mémoire et WiFi (fin)       ------------------- /

// -------------------       FONCTIONS: Ecran OLED (début)       ------------------- /
// Initialisation de l'écran OLED
void setupScreen()
{
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADRESS))
  {
    DEBUG_PRINTLN(F("Erreur de communication avec le chipset SSD1306… arrêt du programme."));
    while (1)
      ; // Arrêt du programme (boucle infinie)
  }
  else
  {
    DEBUG_PRINTLN(F("Initialisation du contrôleur SSD1306 réussi."));
  }
  printImage(DISPLAY_TIME_SEC);
}
// Fonction d'affichage principal, appeler dans la loop pour gérer les temps de display
void manageDisplay()
{
  unsigned long currentMillis = millis();
  if (currentMillis - screenTimerStart >= screenDuration)
  {                      // Si le délais d'affichage est dépassé
    oled.clearDisplay(); // Écrit des zéros dans le buffer
    oled.display();      // Envoie le buffer noir à l'écran
    // DEBUG_PRINTLN(F("OLED effacé (écran noir)."));
  }
}
void printMessage(const char *title, const char *message, unsigned int displayTimeSec)
{
  // On configure le timer
  screenDuration = displayTimeSec * 1000UL; // Conversion en ms
  screenTimerStart = millis();
  // On affiche
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setCursor(0, 1);
  oled.setTextSize(2);
  oled.println(title);
  oled.println();
  oled.setTextSize(1);
  oled.println(message);
  oled.display();
}
// Affichage d'une image au centre de l'écran
void printImage(unsigned int displayTimeSec)
{
  screenDuration = displayTimeSec * 1000UL;
  screenTimerStart = millis();

  oled.clearDisplay();
  oled.drawBitmap(
      (oled.width() - IMAGE_WIDTH) / 2,
      (oled.height() - IMAGE_HEIGHT) / 2,
      IMAGE_CHAT, IMAGE_WIDTH, IMAGE_HEIGHT, WHITE);
  oled.display();
}
void displayScreen(unsigned int displayTimeSec)
{
  screenDuration = displayTimeSec * 1000UL;
  screenTimerStart = millis();

  const byte GRID_LINE_0 = 1;
  const byte GRID_LINE_1 = 20;
  const byte GRID_LINE_2 = 35;
  const byte GRID_LINE_3 = 50;
  const byte GRID_COL_0 = 0;
  const byte GRID_COL_1 = 75;
  const byte GRID_COL_2 = 95;

  char dateBuffer[11]; // Espace pour "DD/MM/YYYY\0"
  // Formatage de l'heure et de la date dans les buffers de char
  sprintf(dateBuffer, "%02d/%02d/%04d", myRTC.dayofmonth, myRTC.month, myRTC.year);
  // snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%04d", myRTC.dayofmonth, myRTC.month, myRTC.year);
  //  Convertir en heure
  char heureActuelle[6];
  char heureCroquettes[6];
  char heureCroquinettes[6];
  char heureProchaineCroquettes[6];
  const int prochainCroqSec = lastFeedTimeCroquettes + delayDistributionCroquettesSec + compteurAbsenceChat * SNOOZE_DELAY_SEC;
  convertSecondsFromMidnightToTime(maintenantSec, heureActuelle);
  convertSecondsFromMidnightToTime(lastFeedTimeCroquettes, heureCroquettes);
  convertSecondsFromMidnightToTime(lastFeedTimeCroquinettes, heureCroquinettes);
  convertSecondsFromMidnightToTime(prochainCroqSec, heureProchaineCroquettes);

  // Serial.print("Maintenant sec : ");
  // Serial.println(maintenantSec);
  // Serial.print("Heure actuelle : ");
  // Serial.println(heureActuelle);
  // Serial.print("Date : ");
  // Serial.println(dateBuffer);

  // Convertir en string
  char nombreDeCroquettes[3];   // Espace pour "00\0"
  char nombreDeCroquinettes[3]; // Espace pour "00\0"
  sprintf(nombreDeCroquettes, "%02d", compteurDeCroquettes);
  sprintf(nombreDeCroquinettes, "%02d", compteurDeCroquinettes);

  oled.clearDisplay();      // Vidange du buffer de l'écran OLED
  oled.setTextColor(WHITE); // Couleur "blanche" (ou colorée, si votre afficheur monochrome est bleu, jaune, ou bleu/jaune)
  oled.setTextSize(1);      // Sélection de l'échelle 1:1

  oled.setCursor(GRID_COL_0, GRID_LINE_0);
  oled.print(dateBuffer);
  oled.setCursor(GRID_COL_2, GRID_LINE_0);
  oled.print(heureActuelle);

  oled.setCursor(GRID_COL_0, GRID_LINE_1);
  oled.print("Croquettes");
  oled.setCursor(GRID_COL_1, GRID_LINE_1);
  oled.print(nombreDeCroquettes);
  oled.setCursor(GRID_COL_2, GRID_LINE_1);
  oled.print(heureCroquettes);

  oled.setCursor(GRID_COL_0, GRID_LINE_2);
  oled.print("Croquinettes");
  oled.setCursor(GRID_COL_1, GRID_LINE_2);
  oled.print(nombreDeCroquinettes);
  oled.setCursor(GRID_COL_2, GRID_LINE_2);
  oled.print(heureCroquinettes);

  oled.setCursor(GRID_COL_0, GRID_LINE_3);
  oled.print("Prochain croq");
  oled.setCursor(GRID_COL_2, GRID_LINE_3);
  oled.print(heureProchaineCroquettes);

  oled.display();
}
// -------------------       FONCTIONS: Ecran OLED (fin)      ------------------- /

// -------------------       FONCTIONS: RTC et conversion de temps (début)       ------------------- /
void syncRTCFromWiFi()
{

  DEBUG_PRINTLN("Synchronisation de l'horloge RTC avec le WiFi..");
  printMessage("Horloge", "Synchronisation de l'horloge RTC avec le WiFi..", DISPLAY_TIME_SEC);
  setupWiFi();        // Vérifier que le WiFi est connecté
  struct tm timeinfo; //  Récupérer l'heure du WiFi
  if (getLocalTime(&timeinfo))
  {
    // Transférer les données de la structure tm au RTC
    // La structure tm utilise 1900 comme année de base (tm_year est l'année - 1900).
    // Elle utilise 0-11 pour les mois (tm_mon) et 0-6 pour les jours de la semaine (tm_wday).
    myRTC.setDS1302Time(
        timeinfo.tm_sec,        // secondes (0-59)
        timeinfo.tm_min,        // minutes (0-59)
        timeinfo.tm_hour,       // heures (0-23)
        timeinfo.tm_wday,       // jour de la semaine (0=dimanche, 6=samedi)
        timeinfo.tm_mday,       // jour du mois (1-31)
        timeinfo.tm_mon + 1,    // mois (0-11) -> on ajoute 1 pour obtenir 1-12
        timeinfo.tm_year + 1900 // année (depuis 1900)
    );
    getRtcTime();
    if (myRTC.year == 2000)
    {
      DEBUG_PRINTLN("Synchronisation RTC échouée, vérifier la batterie ou le branchement.");
      printMessage("Horloge", "Echec de synchronisation de l'heure RTC, verifier la batterie ou le branchement.", 15 * 60);
      // Ressayer toutes les 15min
      delay(15 * 60 * 1000);
      syncRTCFromWiFi();
    }
    else
    {
      DEBUG_PRINT("WiFi: Date / Heure: ");
      DEBUG_PRINT(timeinfo.tm_mday);
      DEBUG_PRINT("/");
      DEBUG_PRINT(timeinfo.tm_mon + 1);
      DEBUG_PRINT("/");
      DEBUG_PRINT(timeinfo.tm_year + 1900);
      DEBUG_PRINT(" ");
      DEBUG_PRINT(timeinfo.tm_hour);
      DEBUG_PRINT(":");
      DEBUG_PRINT(timeinfo.tm_min);
      DEBUG_PRINT(":");
      DEBUG_PRINTLN(timeinfo.tm_sec);
      DEBUG_PRINTLN("Synchronisation RTC réussie.");
      printMessage("Horloge", "Synchronisation de l'heure reussie", DISPLAY_TIME_SEC);
    }
  }
  else
  {
    DEBUG_PRINTLN("Synchronisation RTC échouée car l'heure WiFi n'a pas pu être récupérée.");
    printMessage("Horloge", "Synchronisation RTC echouee car l'heure WiFi n'a pas pu etre recuperee.", DISPLAY_TIME_SEC);
  }
}
void getRtcTime()
{
  myRTC.updateTime();
  DEBUG_PRINT("RTC : Date / Heure: ");
  DEBUG_PRINT(myRTC.dayofmonth);
  DEBUG_PRINT("/");
  DEBUG_PRINT(myRTC.month);
  DEBUG_PRINT("/");
  DEBUG_PRINT(myRTC.year);
  DEBUG_PRINT(" ");
  DEBUG_PRINT(myRTC.hours);
  DEBUG_PRINT(":");
  DEBUG_PRINT(myRTC.minutes);
  DEBUG_PRINT(":");
  DEBUG_PRINTLN(myRTC.seconds);
}
unsigned long getRtcSecondsFromMidnight()
{
  myRTC.updateTime();
  unsigned long totalSeconds = myRTC.hours * 3600 + myRTC.minutes * 60 + myRTC.seconds;
  return totalSeconds;
}
void convertSecondsFromMidnightToTime(unsigned long seconds, char *outBuffer)
{
  // --- CONVERSION DE SEC EN HH:MM ---
  // On s'assure que la valeur reste dans les limites de 24h (86400 secondes)

  long totalSec = seconds % 86400;
  if (totalSec < 0)
    totalSec += 86400; // Gestion si le temps est négatif

  int h = totalSec / 3600;
  int m = (totalSec % 3600) / 60;

  sprintf(outBuffer, "%02dh%02d", h, m);
}
// -------------------       FONCTIONS: RTC et conversion de temps (fin)       ------------------- /