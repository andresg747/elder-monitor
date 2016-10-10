// Tesis 8 Oct
// Andrés García
// v0.1

// Libraries
#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"

// LED pin
const int ledPin = 6;
int buttonPin = 7;
int current; // Activo en bajo.
long millis_held;
long secs_held;
long prev_secs_held;
byte previous = HIGH;
unsigned long firstTime;
int SOS_flag=0; // Por si se intentó enviar SOS y aún no se tiene GPSFIX o nó se pudo enviar.
char sendto[]="04245328127";
uint16_t vbat;

#define quinceSeg (1000UL * 15) // Base rutina c/15 Seg
#define unMin (1000UL * 60)  // Base rutina c/1 min

// Latitude & longitude for distance measurement
float latitude, longitude, speed_kph, heading, altitude;

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

// FONA pins configuration
#define FONA_RX              2   // FONA serial RX pin (pin 2 for shield).
#define FONA_TX              3   // FONA serial TX pin (pin 3 for shield).
#define FONA_RST             4   // FONA reset pin (pin 4 for shield)

// FONA instance & configuration
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);     // FONA software serial connection.
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);                 // FONA library connection.

void setup() {

  pinMode(ledPin, OUTPUT);
  digitalWrite(buttonPin, HIGH);  // Turn on 20k pullup resistors to simplify switch input

  // Initialize serial output.
  Serial.begin(115200);
  Serial.println(F("Inicializando (puede tardar hasta 10 seg)"));

  // Initialize the FONA module
  Serial.println(F("Initializing FONA....(may take 10 seconds)"));
  fonaSS.begin(4800);

  if (!fona.begin(fonaSS)) {
    halt(F("FONA no disponible"));
  }

  Serial.println(F("FONA Conectado!"));

  // Use the watchdog to simplify retry logic and make things more robust.
  // Enable this after FONA is intialized because FONA init takes about 8-9 seconds.
  //Watchdog.enable(8000);
  Watchdog.reset();

  // Enable GPS.
  fona.enableGPS(true);

}

void loop() {

  // Watchdog reset at start of loop--make sure everything below takes less than 8 seconds in normal operation!
  Watchdog.reset();

  checkbutton();

  static unsigned long aux60 = 0 - unMin;
  static unsigned long aux15 = 0 - quinceSeg;


  if ((long)(millis() - aux60) >= unMin){
    aux60 += unMin;
    // Cada 60 seg
    checkGPS();
    if (! fona.getBattPercent(&vbat)) {
      Serial.println(F("No pudo obtener nivel de Bateria"));
    } else {
      Serial.print(F("Porcentaje Bateria = ")); Serial.print(vbat); Serial.println(F("%"));
    }
  }else if((long)(millis() - aux15) >= quinceSeg){
    aux15 += quinceSeg;
    // Cada 15 seg
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
        sendSMS_SOS();
      }
    }
  }

  previous = current;
  prev_secs_held = secs_held;

  if(current == HIGH)
    digitalWrite(ledPin,LOW);
}

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
    }
    // Actualiza IoT
  }else if (!gpsFix){
    SOS_flag = 1;
    Serial.println(F("Aun no hay GPSFIX, se reintentara de nuevo. "));
    char mensaje_txt[140];
    strcpy(mensaje_txt, "SOS! Sin señal GPS. Ultima ubicacion: https://io.adafruit.com/andresg747/ BPM: XX Se notificara de inmediato cuando GPS este disponible.");
    if (!fona.sendSMS(sendto, mensaje_txt)) {
      Serial.println(F("No se pudo enviar SMS"));
    } else {
      Serial.println(F("Mensaje enviado con exito!"));
    }
  }
}

void checkGPS(){
  bool gpsFix = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);

  if(gpsFix){
    Serial.print("Latitud: ");
    printFloat(latitude, 5);
    Serial.println("");

    Serial.print("Longitud: ");
    printFloat(longitude, 5);
    Serial.println("");
  }else{
    Serial.print("GPS No disponible.");
  }
}

// Halt function called when an error occurs.  Will print an error and stop execution while
// doing a fast blink of the LED.  If the watchdog is enabled it will reset after 8 seconds.
void halt(const __FlashStringHelper *error) {
  Serial.println(error);
  while (1) {
    digitalWrite(ledPin, LOW);
    delay(100);
    digitalWrite(ledPin, HIGH);
    delay(100);
  }
}

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

