#### 09/10 2:49am
**Primera prueba - Botón de Pánico**

Al pulsar botón por 3 segundos, el led indicador enciende. Al soltarlo realiza las siguientes funciones:
+ Query de localización al módulo FONA.
+ Si el query es exitoso (GPSFIX), procede a enviar un msj de texto al número pregrabado con un mensaje de texto y un link (gmaps) con la localización exacta.

### 10/10 1:00am
**Mejorando Botón de Pánico**

Al pulsar el botón:

1. Chequea si hay GPSFIX y Envía Mensaje:
   + Si hay, actualiza latitud y longitud. Escribe en Serial. Actualiza IoT. Concatena valores al SMS y envía mensaje SOS.
   + Si no hay GPSFIX envía mensaje SOS invitando a revisar io.adafruit para chequear última localización disponible. Se mantendrá buscando hasta tener GPSFIX de nuevo y vuelve a enviar mensaje con localización "Nueva localización disponible: xxxxxxxxxxx ". (Recomendacion, flag en main loop).

### 10/10 2:00am

Creadas rutinas que se ejecutarán cada 15 seg y 60 seg respectivamente.
Para actualizar de manera constante Localización, Batería y (posteriormente) BPM.

### 12/10 11:30am
Agregado nuevo flag para enviar un único mensaje de alerta si el GPS no está disponible en ese instante. El mensaje invita al supervisor a revisar la última localización disponible en io.adafruit.com/andresg747/feeds/location

Se hicieron pruebas exitosas con función botón de pánico.

### 02/11 11:00pm
Al detectar fallas en tiempo de ejecución, se determinó que el stack de la SRAM estaba colapsando.
Se limpiaron variables y funciones varias intentando disminuir el espacio de almacenamiento utilizado por la SRAM y se corrigieron los fallos.
Se agregó una función de lectura de SMS que detecta comandos (.STATUS y .NUMERO) enviados por el supervisor y responde con SMS dependiendo del caso.

De igual manera, el espacio utilizado en SRAM sigue siendo alto. Aún falta agregar la lectura y cálculo de la frecuencia cardíaca.
Existe la probabilidad de que al agregar esta funcionalidad, la SRAM llegue a un nivel crítico que ponga en riesgo la estabilidad de la ejecución del programa


### 05/11 4:00pm
Se detectó un error que evitaba la conexión con el servidor de adafruit.io (MQTT broker) y cuya 'solución' fué des-comentar la línea #37 de la librería Adafruit_MQTT.h
Solución tomada del reporte del issue: https://github.com/adafruit/Adafruit_MQTT_Library/issues/54
Esta línea de código `#define MQTT_DEBUG` añade comentarios adicionales en la comunicación serial, referentes al envío de paquetes vía el protocolo MQTT. En el caso de este proyecto, se puede observar los paquetes enviados para localización, batería y BPM.

Se agregó un flag para el caso en el que se solicita .STATUS via SMS y ya se envió respuesta cuando no hay GPSFIX pero sin el flag nunca se enviaba el nuevo mensaje con la localización cuando se alcanzaba la conexión GPS.

**Sobre el uso de otro microcontrolador**
Como fué indicado anteriormente, se realizaron ciertos cambios en el código para optimizar el uso del espacio de almacenamiento, tanto en la memoria de programa (FLASH) pero principalmente en la memoria de ejecución (SRAM), del microcontrolador. Se determinó que el desbordamiento, o la sobre saturación de esta última memoria era la causante de que el programa en un momento determinado se reseteara.

Luego de ciertos cambios que se pueden observar en 4e97dd7, se eliminaron funciones inutilizadas, se redujo el espacio utilizado por variables cambiando su tipo a uno más eficiente y se evitó el uso de variables dinámicas. Esto aumentó el espacio de SRAM libre a casi 20%. Luego de este cambio, el dispositivo no volvió a resetearse de manera involuntaria.

Aún el proyecto, como está, no ha sido terminado. Se debe agregar una funcionalidad más para cubrir con los objetivos propuestos: **el sensor de frecuencia cardíaca**. Al revisar la librería utilizada por este sensor, se puede notar que su adición causaría, de nuevo, problemas de espacio en la SRAM. 

Dicho esto, por el poco tiempo que queda para la presentación del prototipo, surge la necesidad de agregar un segundo microcontrolador quedando el esquema de trabajo de la siguiente manera:

	+ Arduino Nano (ATMEGA328P 5V/16MHz) - Principal: Encargado del manejo del módulo de comunicación GSM/GPS, del procesamiento principal de variables y de la toma de decisiones en general.
	+ Trinket Pro (ATMEGA328P 5V/16MHz) - Secundario: Tiene como tarea única tarea el cálculo de la frecuencia cardíaca, y el envío de la misma a través de un canal serial hacia el microcontrolador principal.

El microcontrolador principal, se comunicará con el módulo FONA a través del un canal de SoftwareSerial (pines 10 y 11), y con el microcontrolador secundario a través del HardwareSerial (pines 0 y 1).

