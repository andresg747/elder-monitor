
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
