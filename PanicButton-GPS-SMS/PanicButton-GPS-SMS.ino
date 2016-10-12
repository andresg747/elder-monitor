// Tesis 8 Oct
// Andrés García
// v0.1

// Libraries
#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"

// Variables
const int ledPin = 6;
int buttonPin = 7;
// Para cálculo de tiempo, rutinas de 15 y 60 seg
int current;  // Estado actual Botón Activo en Bajo
long millis_held;
long secs_held;
long prev_secs_held;
byte previous = HIGH;
unsigned long firstTime;

int SOS_flag=0; // Por si se intentó enviar SOS y aún no se tiene GPSFIX o nó se pudo enviar para reintentarlo
int aux_Enviar_SMS = 0; // Por si ya se envió el primer mensaje diciendo que está disponible GPS
char sendto[]="04245328127";
uint16_t vbat;
float latitude, longitude, speed_kph, heading, altitude;
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
// Geofence
const float maxDistance = 100;
float homeLatitude;
float homeLongitude;

#define quinceSeg (1000UL * 15) // Base rutina c/15 Seg
#define unMin (1000UL * 60)  // Base rutina c/1 min

// FONA pins
#define FONA_RX              2   // FONA serial RX pin.
#define FONA_TX              3   // FONA serial TX pin.
#define FONA_RST             4   // FONA reset pin

// FONA GPRS
#define FONA_APN             "internet.movistar.ve"
#define FONA_USERNAME        ""
#define FONA_PASSWORD        ""

// Adafruit IO
#define AIO_SERVER           "io.adafruit.com" 
#define AIO_SERVERPORT       1883
#define AIO_USERNAME         "andresg747"
#define AIO_KEY              "1486c6e39b2f43b88bfa1961028a6ae9"

#define MAX_TX_FAILURES      3  // Maximum number of publish failures in a row before resetting the whole sketch.

// FONA instance & configuration
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);     // FONA software serial connection.
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);                 // FONA library connection.

// MQTT
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the FONA MQTT class by passing in the FONA class and MQTT server and login details.
//Adafruit_MQTT_FONA mqtt(&fona, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);
Adafruit_MQTT_FONA mqtt(&fona, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

uint8_t txFailures = 0; // Cuantos publish fallidos consecutivos han ocurrido 

// Feeds
Adafruit_MQTT_Publish location_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/location/csv");
Adafruit_MQTT_Publish battery_feed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/battery");

void setup() {

  pinMode(ledPin, OUTPUT);
  digitalWrite(buttonPin, HIGH);  // Enciende PullUp 

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
  delay(2000);
  Serial.println(F("Dehabilita GPRS"));
  fona.enableGPRS(false);
  delay(2000);
  Serial.println(F("Habilita GPRS"));
  if (!fona.enableGPRS(true)) {
    halt(F("No se pudo conectar a la red de datos..."));
  }
  Serial.println(F("Conectado a la red de datos."));
  delay(3000);

  // Now make the MQTT connection.
  int8_t ret = mqtt.connect();
  if (ret != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    halt(F("No se pudo conectar a servidor MQTT..."));
  }
  Serial.println(F("MQTT Conectado!"));

  // Initial GPS read
  bool gpsFix = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);
  // CASA -- 10.078431,-69.2805031
  homeLatitude = 10.078431L;
  homeLongitude = -69.2805031L;

  // Use the watchdog to simplify retry logic and make things more robust.
  // Enable this after FONA is intialized because FONA init takes about 8-9 seconds.
  //Watchdog.enable(8000);
  Watchdog.reset();

  // Enable GPS.
  fona.enableGPS(true);

}

void loop() {
  MQTT_connect();

  // Watchdog reset at start of loop--make sure everything below takes less than 8 seconds in normal operation!
  Watchdog.reset();

  checkbutton();

  static unsigned long aux60 = 0 - unMin;
  static unsigned long aux15 = 0 - quinceSeg;


  if ((long)(millis() - aux60) >= unMin){
    aux60 += unMin;
    // Cada 60 seg
    //checkGPS();
    //checkBattery();

  }else if((long)(millis() - aux15) >= quinceSeg){
    aux15 += quinceSeg;
    // Cada 15 seg
    checkGPS();
    checkBattery();
    if(SOS_flag == 1){
      sendSMS_SOS();
    }
  }
  delay(10);
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
    Serial.print("Latitud: ");
    printFloat(latitude, 5);
    Serial.println("");

    Serial.print("Longitud: ");
    printFloat(longitude, 5);
    Serial.println("");

    // Concateno mensaje con Latitud y Longitud
    char mensaje_txt[140];
    strcpy(mensaje_txt, "SOS! Ubicacion: https://maps.google.com/?q=");
    dtostrf(latitude,6,5,&mensaje_txt[strlen(mensaje_txt)]);
    strcat(mensaje_txt,",");
    dtostrf(longitude,6,5,&mensaje_txt[strlen(mensaje_txt)]);
    strcat(mensaje_txt," BPM: XX");

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
    Serial.println(F("Aun no hay GPSFIX, se reintentara de nuevo. "));
    char mensaje_txt[140];
    strcpy(mensaje_txt, "SOS! Sin señal GPS. Ultima ubicacion: https://io.adafruit.com/andresg747/ BPM: XX Se notificara de inmediato cuando GPS este disponible.");
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

// Lectura y actualización de GPS
void checkGPS(){
  bool gpsFix = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);

  if(gpsFix){
    Serial.print("Latitud: ");
    printFloat(latitude, 5);
    Serial.println("");

    Serial.print("Longitud: ");
    printFloat(longitude, 5);
    Serial.println("");

    // Calcula la distancia entre viejas y nuevas coordenadas
    // ************ DESCOMENTA para continuar con GEOFENCING
    //float distance = distanceCoordinates(latitude, longitude, homeLatitude, homeLongitude);

    // Actualiza IoT
    logLocation(latitude, longitude, altitude, location_feed);

  }else{
    Serial.print("GPS No disponible.");
  }
}

void checkBattery(){
  if (! fona.getBattPercent(&vbat)) {
    Serial.println(F("No pudo obtener nivel de Bateria"));
  } else {
    Serial.print(F("Porcentaje Bateria = ")); Serial.print(vbat); Serial.println(F("%"));
  }
  logBatteryPercent(vbat, battery_feed);
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

// Funcion específica para imprimir valores float en Serial con más precisión que la función nativa
void printFloat(float value, int places) {
  // this is used to cast digits 
  int digit;
  float tens = 0.1;
  int tenscount = 0;
  int i;
  float tempfloat = value;

    // make sure we round properly. this could use pow from <math.h>, but doesn't seem worth the import
  // if this rounding step isn't here, the value  54.321 prints as 54.3209

  // calculate rounding term d:   0.5/pow(10,places)  
  float d = 0.5;
  if (value < 0)
    d *= -1.0;
  // divide by ten for each decimal place
  for (i = 0; i < places; i++)
    d/= 10.0;    
  // this small addition, combined with truncation will round our values properly 
  tempfloat +=  d;

  // first get value tens to be the large power of ten less than value
  // tenscount isn't necessary but it would be useful if you wanted to know after this how many chars the number will take

  if (value < 0)
    tempfloat *= -1.0;
  while ((tens * 10.0) <= tempfloat) {
    tens *= 10.0;
    tenscount += 1;
  }


  // write out the negative if needed
  if (value < 0)
    Serial.print('-');

  if (tenscount == 0)
    Serial.print(0, DEC);

  for (i=0; i< tenscount; i++) {
    digit = (int) (tempfloat/tens);
    Serial.print(digit, DEC);
    tempfloat = tempfloat - ((float)digit * tens);
    tens /= 10.0;
  }

  // if no places after decimal, stop now and return
  if (places <= 0)
    return;

  // otherwise, write the point and continue on
  Serial.print('.');  

  // now write out each decimal place by shifting digits one by one into the ones place and writing the truncated value
  for (i = 0; i < places; i++) {
    tempfloat *= 10.0; 
    digit = (int) tempfloat;
    Serial.print(digit,DEC);  
    // once written, subtract off that digit
    tempfloat = tempfloat - (float) digit; 
  }
}

// Calcula distancia entre dos puntos usando la formula Haversine: http://www.movable-type.co.uk/scripts/latlong.html
float distanceCoordinates(float flat1, float flon1, float flat2, float flon2) {

  // Variables
  float dist_calc=0;
  float dist_calc2=0;
  float diflat=0;
  float diflon=0;

  // Calculations
  diflat  = radians(flat2-flat1);
  flat1 = radians(flat1);
  flat2 = radians(flat2);
  diflon = radians((flon2)-(flon1));

  dist_calc = (sin(diflat/2.0)*sin(diflat/2.0));
  dist_calc2 = cos(flat1);
  dist_calc2*=cos(flat2);
  dist_calc2*=sin(diflon/2.0);
  dist_calc2*=sin(diflon/2.0);
  dist_calc +=dist_calc2;

  dist_calc=(2*atan2(sqrt(dist_calc),sqrt(1.0-dist_calc)));

  dist_calc*=6371000.0; //Converting to meters

  return dist_calc;
}

// Conexion MQTT
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}

// Log battery
void logBatteryPercent(uint32_t indicator, Adafruit_MQTT_Publish& publishFeed) {
  // Publish
  Serial.print(F("Publicando Porcentaje Bateria: ")); Serial.println(indicator);
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
