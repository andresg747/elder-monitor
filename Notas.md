#### 09/10 2:49am
**Primera prueba - Botón de Pánico**

Al pulsar botón por 3 segundos, el led indicador enciende. Al soltarlo realiza las siguientes funciones:
+ Query de localización al módulo FONA.
+ Si el query es exitoso (GPSFIX), procede a enviar un msj de texto al número pregrabado con un mensaje de texto y un link (gmaps) con la localización exacta.


**Mejorando Botón de Pánico**

#### 10/10 1:00am

Al pulsar el botón:

1. Chequea si hay GPSFIX y Envía Mensaje:
   + Si hay, actualiza latitud y longitud. Escribe en Serial. Actualiza IoT. Concatena valores al SMS y envía mensaje SOS.
   + Si no hay GPSFIX envía mensaje SOS invitando a revisar io.adafruit para chequear última localización disponible. Se mantendrá buscando hasta tener GPSFIX de nuevo y vuelve a enviar mensaje con localización "Nueva localización disponible: xxxxxxxxxxx ". (Recomendacion, flag en main loop).

#### 10/10 2:00am

Creadas rutinas que se ejecutarán cada 15 seg y 60 seg respectivamente.
Para actualizar de manera constante Localización, Batería y (posteriormente) BPM.

Tomar en cuenta: GPS Margen de error +- 40mts (malas condiciones);