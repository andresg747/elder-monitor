#### 09/10 2:49am
**Primera prueba - Botón de Pánico**

Al pulsar botón por 3 segundos, el led indicador enciende. Al soltarlo realiza las siguientes funciones:
+ Query de localización al módulo FONA.
+ Si el query es exitoso (GPSFIX), procede a enviar un msj de texto al número pregrabado con un mensaje de texto y un link (gmaps) con la localización exacta.

#### 09/10 5:00pm
**Mejorando Botón de Pánico**

Al pulsar el botón:

1. Chequea si hay GPSFIX:
    + Si hay, actualiza latitud y longitud. Escribe en Serial. Actualiza IoT.
	+ Si no hay, muestra "GPS No Disponible" en Serial.
2. Envia Mensaje:
	+ Si hay GPSFIX, concatena valores al SMS y envía mensaje SOS.
    + Si no hay GPSFIX envía mensaje SOS invitando a revisar io.adafruit para chequear última localización disponible. Se mantendrá buscando hasta tener GPSFIX de nuevo y vuelve a enviar mensaje con localización "Nueva localización disponible: xxxxxxxxxxx ". (Recomendacion, flag en main loop).
