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

### 12/10 11:30am
Agregado nuevo flag para enviar un único mensaje de alerta si el GPS no está disponible en ese instante. El mensaje invita al supervisor a revisar la última localización disponible en io.adafruit.com/andresg747/feeds/location

Se hicieron pruebas exitosas con función botón de pánico.

### 02/11 11:00pm
Al detectar fallas en tiempo de ejecución, se determinó que el stack de la SRAM estaba colapsando.
Se limpiaron variables y funciones varias intentando disminuir el espacio de almacenamiento utilizado por la SRAM y se corrigieron los fallos.
Se agregó una función de lectura de SMS que detecta comandos (.STATUS y .NUMERO) enviados por el supervisor y responde con SMS dependiendo del caso.

De igual manera, el espacio utilizado en SRAM sigue siendo alto. Aún falta agregar la lectura y cálculo de la frecuencia cardíaca.
Existe la probabilidad de que al agregar esta funcionalidad, la SRAM llegue a un nivel crítico que ponga en riesgo la estabilidad de la ejecución del programa

Progreso estimado: 85%


