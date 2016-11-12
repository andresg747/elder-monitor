
// Calcula distancia entre dos puntos usando la formula Haversine: http://www.movable-type.co.uk/scripts/latlong.html
// float distanceCoordinates(float flat1, float flon1, float flat2, float flon2) {

//   // Variables
//   float dist_calc=0;
//  float dist_calc2=0;
//  float diflat=0;
//  float diflon=0;

//   // Calculations
//  diflat  = radians(flat2-flat1);
//  flat1 = radians(flat1);
//  flat2 = radians(flat2);
//  diflon = radians((flon2)-(flon1));

//  dist_calc = (sin(diflat/2.0)*sin(diflat/2.0));
//  dist_calc2 = cos(flat1);
//  dist_calc2*=cos(flat2);
//  dist_calc2*=sin(diflon/2.0);
//  dist_calc2*=sin(diflon/2.0);
//  dist_calc +=dist_calc2;

//  dist_calc=(2*atan2(sqrt(dist_calc),sqrt(1.0-dist_calc)));

//   dist_calc*=6371000.0; //Converting to meters

//   return dist_calc;
// }

// Conexion MQTT
// void MQTT_connect() {
//  int8_t ret;

//   // Stop if already connected.
//  if (mqtt.connected()) {
//    return;
//  }

//  Serial.print("Connecting to MQTT... ");

//   while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
//    Serial.println(mqtt.connectErrorString(ret));
//    Serial.println("Retrying MQTT connection in 5 seconds...");
//    mqtt.disconnect();
//     delay(5000);  // wait 5 seconds
//   }
//   Serial.println("MQTT Connected!");
// }



// // Funcion específica para imprimir valores float en Serial con más precisión que la función nativa
// void printFloat(float value, int places) {
//   // this is used to cast digits 
//   int digit;
//   float tens = 0.1;
//   int tenscount = 0;
//   int i;
//   float tempfloat = value;

//   // make sure we round properly. this could use pow from <math.h>, but doesn't seem worth the import
//   // if this rounding step isn't here, the value  54.321 prints as 54.3209

//   // calculate rounding term d:   0.5/pow(10,places)  
//   float d = 0.5;
//   if (value < 0)
//     d *= -1.0;
//   // divide by ten for each decimal place
//   for (i = 0; i < places; i++)
//     d/= 10.0;    
//   // this small addition, combined with truncation will round our values properly 
//   tempfloat +=  d;

//   // first get value tens to be the large power of ten less than value
//   // tenscount isn't necessary but it would be useful if you wanted to know after this how many chars the number will take

//   if (value < 0)
//     tempfloat *= -1.0;
//   while ((tens * 10.0) <= tempfloat) {
//     tens *= 10.0;
//     tenscount += 1;
//   }


//   // write out the negative if needed
//   if (value < 0)
//     Serial.print('-');

//   if (tenscount == 0)
//     Serial.print(0, DEC);

//   for (i=0; i< tenscount; i++) {
//     digit = (int) (tempfloat/tens);
//     Serial.print(digit, DEC);
//     tempfloat = tempfloat - ((float)digit * tens);
//     tens /= 10.0;
//   }

//   // if no places after decimal, stop now and return
//   if (places <= 0)
//     return;

//   // otherwise, write the point and continue on
//   Serial.print('.');  

//   // now write out each decimal place by shifting digits one by one into the ones place and writing the truncated value
//   for (i = 0; i < places; i++) {
//     tempfloat *= 10.0; 
//     digit = (int) tempfloat;
//     Serial.print(digit,DEC);  
//     // once written, subtract off that digit
//     tempfloat = tempfloat - (float) digit; 
//   }
// }
