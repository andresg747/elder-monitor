// Trabajo de Grado
// Andrés García

// Libraries
#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"
#include <Wire.h>

// Variables
int ledPin = 6;
int blinkPin = 9;
int buttonPin = 7;
int current;        // Estado del botón, activo en bajo
char bpm_chr = "";
char buffer[3];
int bpm = 0;
long millis_held;   // Milisegundos transcurridos
long secs_held;
long prev_secs_held;
byte previous = HIGH;
unsigned long firstTime;
char replybuffer[30];
byte SOS_flag=0; // Por si se intentó enviar SOS y aún no se tiene GPSFIX o nó se pudo enviar para reintentarlo
byte aux_Enviar_SMS = 0;
byte aux_Enviar_SMS_STATUS = 0;
byte STATUS_flag = 0;
char sendto[13]="+584245328127";
uint16_t vbat;
float latitude, longitude, speed_kph, heading, altitude;
int smsnum; // Número de SMS en SIMCARD
// Para lectura SMS comando
char *numero;

#define quinceSeg (1000UL * 15) // Base rutina c/15 Seg
#define unMin (1000UL * 60)  // Base rutina c/1 min
static unsigned long aux60 = 0 - unMin;
static unsigned long aux15 = 0 - quinceSeg;

// FONA pins
#define FONA_RX              11   // FONA serial RX pin.
#define FONA_TX              10   // FONA serial TX pin.
#define FONA_RST             4   // FONA reset pin

// FONA GPRS
#define FONA_APN             "internet.movistar.ve"
#define FONA_USERNAME        ""
#define FONA_PASSWORD        ""

// Adafruit IO
#define AIO_SERVER           "io.adafruit.com" 
#define AIO_SERVERPORT       1883
#define AIO_USERNAME         "andresg747"
#define AIO_KEY              "90edb2e45d1c4837b71a4ac70e6c83bd"

// #define MAX_TX_FAILURES      3  // Maximum number of publish failures in a row before resetting the whole sketch.

// FONA instance & configuration
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);     // FONA software serial connection.
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);                 // FONA library connection.

Adafruit_MQTT_FONA mqtt(&fona, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

uint8_t txFailures = 0; // Cuantos publish fallidos consecutivos han ocurrido 

// Feeds
Adafruit_MQTT_Publish location_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/location/csv");
Adafruit_MQTT_Publish battery_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/battery");
Adafruit_MQTT_Publish bpm_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/bpm");

void setup() {
  pinMode(blinkPin, OUTPUT);        // pin that will blink to your heartbeat!
  pinMode(ledPin, OUTPUT);
  digitalWrite(buttonPin, HIGH);  // Enciende PullUp 
  digitalWrite(ledPin,LOW);

  //Wire
  Wire.begin();        // Inicializa comunicación I2C

  //Inicializa Puerto Serial y Conexión Módulo
  Serial.begin(115200);
  Serial.println(F("Inicializando (puede tardar hasta 10 seg)"));
  fonaSS.begin(4800);
  if (!fona.begin(fonaSS)) {
    halt(F("FONA no disponible"));
  }

  Serial.println(F("FONA Conectado!"));

  // Wait for FONA to connect to cell network (up to 8 seconds, then watchdog reset).
  Serial.println(F("Conectando a la red..."));
  while (fona.getNetworkStatus() != 1) {
    delay(500);
  }
  fona.setGPRSNetworkSettings(F(FONA_APN));
  delay(1000);
  Serial.println(F("Dehabilita GPRS"));
  fona.enableGPRS(false);
  delay(1000);
  Serial.println(F("Habilita GPRS"));
  while (!fona.enableGPRS(true)) {
    Serial.println(F("No se pudo conectar a la red de datos..."));
    delay(1000);
    Serial.println(F("Dehabilita GPRS"));
    fona.enableGPRS(false);
    delay(1000);
  }
  Serial.println(F("Conectado a la red de datos."));
  delay(1000);

  // Now make the MQTT connection.
  int8_t ret = mqtt.connect();
  if (ret != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println(F("No se pudo conectar a servidor MQTT..."));
  }
  Serial.println(F("MQTT Conectado!"));

  // Initial GPS read
  bool gpsFix = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);

  // Enable GPS.
  fona.enableGPS(true);

  delay(500);

  borrarSMS();
  checkGPS();
}

void loop() {
  //MQTT_connect();
  checkbutton();

  if ((long)(millis() - aux60) >= unMin){
    aux60 += unMin;
    // ++++++++  Cada 60 seg
    checkGPS();
    checkBattery();

  }else if((long)(millis() - aux15) >= quinceSeg){
    aux15 += quinceSeg;
    // ++++++++  Cada 15 seg
    checkBPM();
    leerSMS();
  }

  //Veriicamos flag para envío de SMS Alerta
  if(SOS_flag == 1){
    sendSMS_SOS();
  }
  if(STATUS_flag){
    sendSMS_STATUS();
  }

  delay(10);
}

void leerSMS(){
  smsnum = fona.getNumSMS();
  if(smsnum > 0){
    Serial.println(F("Nuevo Mensaje de Texto"));
    uint8_t n = 1; 
    while (true) {
      uint16_t smslen;
      char sender[13];

      Serial.print(F("\n\rReading SMS #")); Serial.println(n);
     uint8_t len = fona.readSMS(n, replybuffer, 250, &smslen); // pass in buffer and max len!
     // if the length is zero, its a special case where the index number is higher
     // so increase the max we'll look at!
     if (len == 0) {
      Serial.println(F("[empty slot]"));
      n++;
      continue;
     }
     if (! fona.getSMSSender(n, sender, sizeof(sender))) {
       // failed to get the sender?
      sender[0] = 0;
     }
     
     Serial.print(F("***** SMS #")); Serial.print(n);
     Serial.print(" ("); Serial.print(len); Serial.println(F(") bytes *****"));
     Serial.println(replybuffer);
     Serial.print(F("From: ")); Serial.println(sender);
     Serial.println(F("*****"));

     String mensaje = replybuffer;

     // if (strcasecmp(replybuffer, ".STATUS") == 0) {
     if (mensaje.substring(0, 7) == ".STATUS") {
       // Comando Status
      Serial.println(F("/////////////////////"));
      Serial.println(F("Solicita Status"));
      aux_Enviar_SMS_STATUS = 1;
      sendSMS_STATUS();
     }
     // if (strcasecmp(replybuffer, ".NUMERO") == 0) {
     if (mensaje.substring(0, 7) == ".NUMERO") {
      // Comando Cambiar Número
      Serial.println(F("/////////////////////"));
      Serial.println(F("Cambiar Numero a: "));
      Serial.println(mensaje.substring(8,21));
     }
     
     delay(3000);
     break;
   } 
   borrarSMS(); 
 }
 Serial.print(F("Hay "));
 Serial.print((smsnum));
 Serial.println(F(" SMS's en la SIM CARD."));
}

void checkbutton(){
  current = digitalRead(buttonPin);

  // if the button state changes to pressed, remember the start time 
  if (current == LOW && previous == HIGH && (millis() - firstTime) > 200) {
    firstTime = millis();
  }

  millis_held = (millis() - firstTime);
  secs_held = millis_held / 1000;
  // Chequea si el botón realmente fue pulsado (debounce)
  if (millis_held > 50) {
    // Pulsado por más de 2800ms
    if (current == LOW && secs_held > prev_secs_held ) {
      if (millis_held > 2900 )
      {
        digitalWrite(ledPin,HIGH);
      }
    }
    if (current == HIGH && previous == LOW) {
      if (millis_held >= 3000) {
        aux_Enviar_SMS = 1;
        sendSMS_SOS();
      }
    }
  }

  previous = current;
  prev_secs_held = secs_held;

  if(current == HIGH)
    digitalWrite(ledPin,LOW);
}

// Envio Mensaje Socorro
void sendSMS_SOS(){
  // Lectura GPS
  bool gpsFix = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);

  if(gpsFix){
    Serial.print(F("Latitud: "));
    Serial.print(latitude, 5);
    Serial.println("");

    Serial.print(F("Longitud: "));
    Serial.print(longitude, 5);
    Serial.println("");

    // Concateno mensaje con Latitud y Longitud
    char mensaje_txt[140];
    strcpy(mensaje_txt, "SOS! Ubicacion: https://maps.google.com/?q=");
    dtostrf(latitude,6,5,&mensaje_txt[strlen(mensaje_txt)]);
    strcat(mensaje_txt,",");
    dtostrf(longitude,6,5,&mensaje_txt[strlen(mensaje_txt)]);
    strcat(mensaje_txt," BPM: ");
    sprintf(buffer, "%u", bpm);
    strcat(mensaje_txt,buffer);

    // Imprimo mensaje en serial
    Serial.println(mensaje_txt);

    //Envío mensaje
    if (!fona.sendSMS(sendto, mensaje_txt)) {
      Serial.println(F("No se pudo enviar SMS"));
      SOS_flag = 1;
    } else {
      Serial.println(F("Mensaje enviado con exito!"));
      SOS_flag = 0;
      aux_Enviar_SMS = 0;
    }
    // Actualiza IoT
  }else if (!gpsFix){
    SOS_flag = 1;
    Serial.println(F("Para SMS-SOS Aun no hay GPSFIX, se intentara de nuevo. "));
    char mensaje_txt[140];
    strcpy(mensaje_txt, "SOS! GPS no disponible. Ultima ubicacion: https://io.adafruit.com/andresg747/ Se notificara de inmediato cuando GPS este disponible. ");
    strcat(mensaje_txt," BPM: ");
    sprintf(buffer, "%u", bpm);
    strcat(mensaje_txt,buffer);
    if(aux_Enviar_SMS){
      if (!fona.sendSMS(sendto, mensaje_txt)) {
        Serial.println(F("No se pudo enviar SMS"));
      } else {
        Serial.println(F("Mensaje enviado con exito!"));
        aux_Enviar_SMS = 0;
      }
    }
  }
}

// Envio Mensaje Previa solicitud STATUS
void sendSMS_STATUS(){
  // Lectura GPS
  bool gpsFix = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);

  if(gpsFix){
    // Concateno mensaje con Latitud y Longitud
    char mensaje_txt[140];
    strcpy(mensaje_txt, "Ubicacion: https://maps.google.com/?q=");
    dtostrf(latitude,6,5,&mensaje_txt[strlen(mensaje_txt)]);
    strcat(mensaje_txt,",");
    dtostrf(longitude,6,5,&mensaje_txt[strlen(mensaje_txt)]);
    strcat(mensaje_txt," BPM: ");
    sprintf(buffer, "%u", bpm);
    strcat(mensaje_txt,buffer);

    // Imprimo mensaje en serial
    Serial.println(mensaje_txt);

    //Envío mensaje
    if (!fona.sendSMS(sendto, mensaje_txt)) {
      Serial.println(F("No se pudo enviar SMS"));
    } else {
      Serial.println(F("Mensaje enviado con exito!"));
      STATUS_flag = 0;
    }
    // Actualiza IoT
  }else if (!gpsFix){
    STATUS_flag = 1;
    Serial.println(F("Para SMS-STATUS Aun no hay GPSFIX, se intentara de nuevo."));
    char mensaje_txt[140];
    strcpy(mensaje_txt, "GPS no disponible. Ultima ubicacion: https://io.adafruit.com/andresg747/ Se notificara de inmediato cuando GPS este disponible. ");
    strcat(mensaje_txt," BPM: ");
    sprintf(buffer, "%u", bpm);
    strcat(mensaje_txt,buffer);
    if(aux_Enviar_SMS_STATUS){
      if (!fona.sendSMS(sendto, mensaje_txt)) {
        Serial.println(F("No se pudo enviar SMS"));
      } else {
        Serial.println(F("Mensaje enviado con exito!"));
        aux_Enviar_SMS_STATUS = 0;
      }
    }
  }
}

// Lectura y actualización de GPS
void checkGPS(){
  bool gpsFix = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);

  if(gpsFix){
    Serial.print(F("Latitud: "));
    Serial.print(latitude, 5);
    Serial.println("");

    Serial.print(F("Longitud: "));
    Serial.print(longitude, 5);
    Serial.println("");

    // Actualiza IoT
    logLocation(latitude, longitude, altitude, location_feed);

  }else{
    Serial.print(F("GPS No disponible."));
  }
}

void checkBattery(){
  if (! fona.getBattPercent(&vbat)) {
    Serial.println(F("No pudo obtener nivel de Bateria"));
  } else {
    Serial.print(F("Porcentaje Bateria = ")); Serial.print(vbat); Serial.println(F("%"));
  }
  log(vbat, battery_feed);
}

void checkBPM(){
  Serial.println(F("Solicitando BPM"));
  Wire.requestFrom(1, 1);     // request 1 bytes from slave device #1
  while (Wire.available()) {  // slave may send less than requested
    bpm_chr = Wire.read();      // receive a byte as character
    bpm = (int)bpm_chr;
    Serial.print("BPM: ");
    Serial.println(bpm);          // print the int
  }
  log(bpm, bpm_feed);
}

// Manejo de errores
void halt(const __FlashStringHelper *error) {
  Serial.println(error);
  while (1) {
    digitalWrite(ledPin, LOW);
    delay(100);
    digitalWrite(ledPin, HIGH);
    delay(100);
  }
}

// Log battery
void log(uint32_t indicator, Adafruit_MQTT_Publish& publishFeed) {
  // Publish
  Serial.print(F("Publicando ")); Serial.println(indicator);
  if (!publishFeed.publish(indicator)) {
    Serial.println(F("Publish failed!"));
    txFailures++;
  }
  else {
    Serial.println(F("Publish succeeded!"));
    txFailures = 0;
  }
}

// Serialize the lat, long, altitude to a CSV string that can be published to the specified feed.
void logLocation(float latitude, float longitude, float altitude, Adafruit_MQTT_Publish& publishFeed) {
  // Initialize a string buffer to hold the data that will be published.
  char sendBuffer[120];
  memset(sendBuffer, 0, sizeof(sendBuffer));
  int index = 0;

  // Start with '0,' to set the feed value.  The value isn't really used so 0 is used as a placeholder.
  sendBuffer[index++] = '0';
  sendBuffer[index++] = ',';

  // Now set latitude, longitude, altitude separated by commas.
  dtostrf(latitude, 2, 6, &sendBuffer[index]);
  index += strlen(&sendBuffer[index]);
  sendBuffer[index++] = ',';
  dtostrf(longitude, 3, 6, &sendBuffer[index]);
  index += strlen(&sendBuffer[index]);
  sendBuffer[index++] = ',';
  dtostrf(altitude, 2, 6, &sendBuffer[index]);

  // Finally publish the string to the feed.
  Serial.print(F("Publicando Localizacion: "));
  Serial.println(sendBuffer);
  if (!publishFeed.publish(sendBuffer)) {
    Serial.println(F("Publish failed!"));
    txFailures++;
  }
  else {
    Serial.println(F("Publish succeeded!"));
    txFailures = 0;
  }
}

void borrarSMS(){
  smsnum = fona.getNumSMS();
  Serial.print(F("Hay "));
  Serial.print((smsnum));
  Serial.println(F(" SMS's en la SIM CARD."));
  if(smsnum > 0){
    for (int n = 1; n <= smsnum; ++n)
    {
      Serial.println(F("Borrando Mensaje #: "));
      Serial.println((n));
      fona.deleteSMS(n);
      delay(2000);
    }
  }
}
