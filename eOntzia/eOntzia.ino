 /*  GRUPO 2. Mehdi Amara. Jonathan Arizala. Mikel Diez.
    La Salle Berrozpe. 2015/2016. 26/02/2015
     Programacion Sensor Arduino + GPS + Envio de datos via SMS y wifi*/

// declaración de variables 

#include <SoftwareSerial.h>
#include <stdlib.h>
#include <stdio.h>
#include <Adafruit_GPS.h>
#include <avr/wdt.h>
int sensor_ultrasonido;
int Id_dispositivo = 5;
int Volumen = 0;
int fuego = 0;
int bateria = 1;
int f=0;
const int trigPin = 10 ;
const int echoPin = 11 ;
int led_wifi=6;
int humo=5 ;
String SMS_string ;
long duration, inches, cm, h; // Variables ;
int distance ;


long microsecondsToCentimeters(long microseconds);

// Calcula la distancia en cm
long microsecondsToCentimeters(long microseconds)
{
  return microseconds / 60;
}
;

// connect 12 to TX of Serial USB
// connect 13 to RX of serial USB

SoftwareSerial ser(12, 13); // RX, TX


//SoftwareSerial mySerial(8, 9);
//Adafruit_GPS GPS(&mySerial);
#define GPSECHO  false
int8_t respuesta;
int PinModuloOn = 2;   // Este pin es el que utilizaremos para encender y apagar el módulo.
//A la hora de encender y apagar el modulo, necesitaremos aplicarle un pulso en el pin 2. Un pulso para encenderlo y otro pulso para apagarlo
int sms_enviado = 0;
int sms_cantidad = 0;
String SMS_text ;
char aux_str[30];
char aux_string[30];
//char numero_telefono[] = "629219414";  // Aquí pondremos el número de teléfono con el que queramos que contacte.
char numero_telefono[] = "609671481"; 
char mensaje[200];
char texte[10];

bool IP_ERROR=0;
bool IP_NULL=0; 

boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

// this runs once

void setup() {                
  // declaramos entradas y salidas
  pinMode(sensor_ultrasonido,INPUT);
  pinMode(humo,INPUT);
  pinMode(led_wifi,OUTPUT); 
// ultrasonido
  pinMode (trigPin,OUTPUT);
  pinMode (echoPin,INPUT);
  delay(1000);
// Wifi  
  // enable debug serial
  Serial.begin(9600); 
 // enable software serial
  ser.begin(9600);
  
  // reset ESP8266
 bool OK ;
 Mandarcomando("AT\r\n",0);
 OK = ser.find("OK");
 blink_wifi(50,50);
 Serial.println ("esp6288 is Power ON ");
 bool RST;
 
 Mandarcomando("AT+RST\r\n",0);    // Resetearemos el módulo y le daremos un timeout de 2s.

 RST = ser.find("OK");
 int Counter_rst=0 ;
  do 
{
    if (Counter_rst <=10) 
    {
  Counter_rst= Counter_rst+1 ;
  //blink_wifi(100,200);
   blink_wifi(50,50);
   Mandarcomando("AT+RST\r\n",0); 
    }
else
 {
   software_Reboot();
 }
  
} while (RST==0);

int counter_tester=0 ;
bool wifi ;
  Mandarcomando("AT+CWMODE=1\r\n",1000);    // Configuraremos el módulo como Punto de acceso con un timeout de 1s. 
 
  do 
{ 
  
   if  (counter_tester<=7)  
  {
 blink_wifi(100,10);
 // Comandos para conectarnos a la red
 // Mandarcomando("AT+CWJAP=\"LSB_IKASLEAK\",\"gW1@j444\"\r\n",0);
  Mandarcomando("AT+CWJAP=\"iPhone\",\"09876543\"\r\n",0);
 // Mandarcomando("AT+CWJAP=\"M\",\"molella2016\"\r\n",0);
  wifi = ser.find("OK");
  counter_tester=counter_tester+1 ;
  }
  else
  {
    
    software_Reboot();
         
   }

} while (wifi==0);
 
  
delay(1);
Mandarcomando("AT+CIFSR\r\n",0);
blink_wifi(150,10);
//delay(5000)  ;
IP_ERROR =ser.find ("ERROR");

IP_NULL  =ser.find("0.0.0.0");

do
{ 
  blink_wifi(150,10);
  Mandarcomando("AT+CIFSR\r\n",0);
  blink_wifi(100,10);
  IP_ERROR =ser.find("ERROR");
  IP_NULL  =ser.find("0.0.0.0");
  
  delay(1);//delay(1000)  ;
 
} while((IP_ERROR==1)||(IP_NULL==1));

digitalWrite (led_wifi,HIGH);
Serial.print ("Device got an IP from router. ... "); //Device got an IP from router.

  delay(2000); 
      
 // delay(5000);
  Serial.begin(9600);    // Iniciamos la comunicación serial.
  Serial.println(" WIfi+ GSM + ULTRASONIDO");
  Serial.println("Iniciando...");    // Imprimimos el aviso de que comienza el programa.
  
  encender_modulo();    // Ejecutamos la función encargada de encender el módulo GSM.

  configuracion_SIM900_SMS(); //

  ULTRASONIDO (); //(Salida el volumen )
  sensor_humo();
  Mandando_datos_via_SMS ( Id_dispositivo ,Volumen ,fuego ,bateria  );

// Mandando_datos_via_GET(SMS_string, Id_dispositivo , Volumen , fuego , bateria);

}


// the loop
void loop() {

  SMS_string = "";
  recibiendo_datos_via_SMS ();
 
  if (Volumen<95 )
  {
  sms_enviado = 1;
  Mandando_datos_via_GET(SMS_string, Id_dispositivo , Volumen , fuego , bateria);
  }
  ULTRASONIDO();
  sensor_humo();
  Mandando_datos_via_SMS ( Id_dispositivo ,Volumen ,fuego ,bateria  );
  Mandando_datos_via_GET(SMS_string, Id_dispositivo , Volumen , fuego , bateria);
 
}
//FUNCION MANDAR COMANDO:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

String Mandarcomando(String comando, const int timeout)
{
  String respuesta = "";

  Serial.print(comando);
  ser.print(comando);
  long int tiempo = millis();


  while ( (tiempo + timeout) > millis())
  {
    while (ser.available())
    {


      char c = ser.read();
      respuesta += c;
    }
  }

  Serial.print(respuesta);

  return respuesta;
}

// FUNCION GSM ENCENDER::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void encender_modulo() {   // Función para encender el módulo.
  uint8_t respuesta = 0;
  do
  {
    // checks if the module is started
    respuesta = mandarcomandoAT("AT", "OK", 2000);    // Mandamos el comando AT para ver si responde el modulo.

    // En caso de que no responda significa que está apagado, así que procedemos a encenderlo.
    if (respuesta == 0)
    {
      // El modulo se enciende con un pulso y se vuelve a apagar con otro pulso.
      digitalWrite(PinModuloOn, HIGH);
      delay(3000);
      digitalWrite(PinModuloOn, LOW);

      respuesta = mandarcomandoAT("AT", "OK", 2000);    // Mandamos otra vez el comando AT para ver si responde. Si responde saldremos del loop.
    }
  } while (respuesta == 0);  // Mientras la respuesta sea 0 seguirá haciendo el loop.
}

//FUNCION RECIBIR COMANDOS AT::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

int8_t recibircomandoAT(unsigned int timeout)    // Función para la recepción de comandos
{
  uint8_t x = 0;
  uint8_t respuesta = 0;
  char respuesta_modulo[100];
  unsigned long anterior;
  memset(respuesta_modulo, '\0', 100);    // Vaciamos el string respuesta_modulo
  delay(100);
  while (Serial.available() > 0)    // Mientras haya datos en el serial los leemos.
  {
    Serial.read();
  }
  x = 0;
  anterior = millis();  // Guardamos el tiempo que lleva corriendo el programa.

  // Con este loop comparamos las respuestas del módulo.
  do
  {
    // Si hay datos en el buffer de entrada, leemos y analizamos la respuesta
    if (Serial.available() != 0)
    {
      respuesta_modulo[x] = Serial.read();  // Guardamos lo que hemos leído en la variable "respuesta_modulo".
      x++;
      //verificamos que la respuesta es la esperada
      if (strstr(respuesta_modulo, "+CMTI: \"SM\",1") != NULL)    // Comparamos la respuesta del módulo con la esperada.
      {
        respuesta = 1;    // Si coincide es que se trata de un SMS.
      }
      //if (strstr(respuesta_modulo, "+CLIP: \"629219414\",129,\"\",,\"\",0") != NULL)   // Comparamos la respuesta del módulo con la esperada.
     if (strstr(respuesta_modulo, "+CLIP: \"609671481\",129,\"\",,\"\",0") != NULL)   // Comparamos la respuesta del módulo con la esperada. 
      {
        respuesta = 2;    // Si coincide es que se trata de una llamada.
      }
    }
  } while ((respuesta == 0) && ((millis() - anterior) < timeout));  // Ejecutara el loop hasta que la respuesta sea 1 o pase el timeout.
  return respuesta;
}
//FUNCION MANDAR COMANDO AT::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

int8_t mandarcomandoAT(char* comandoAT, char* respuesta_esperada, unsigned int timeout) {   // Función encargada de mandar los comandos AT.

  uint8_t x = 0,  respuesta = 0;
  char respuesta_modulo[100];
  unsigned long anterior;

  memset(respuesta_modulo, '\0', 100);    // Vaciamos el string respuesta_modulo

  delay(100);

  // Mientras haya información en el serial lo leemos para dejarlo vacío.
  while ( Serial.available() > 0) Serial.read();

  Serial.println(comandoAT);    // Imprimimos el comando AT


  x = 0;
  anterior = millis();    // Guardamos el tiempo que lleva corriendo el programa en la variable "anterior".

  // Este loop verifica la respuesta recibida del módulo.
  do {
    // Si tenemos el serial con datos los leemos y analizamos la respuesta del módulo con la respuesta esperada.
    if (Serial.available() != 0) {
      respuesta_modulo[x] = Serial.read();    // Lo que leemos del serial lo metemos en la variable "respuesta_modulo".
      x++;
      // Comprobamos que la respuesta del módulo coincide con la respuesta esperada.
      if (strstr(respuesta_modulo, respuesta_esperada) != NULL)
      {
        respuesta = 1;
      }
    }
  } while ((respuesta == 0) && ((millis() - anterior) < timeout));   // Ejecutara el loop hasta que la respuesta sea 1 o pase el timeout.

  return respuesta;
}

// MODULO ULTRASONIDO:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

int ULTRASONIDO() {

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH); //Devuelve la longitud del pulso del pin Echo en us
  cm = microsecondsToCentimeters(duration); // Convierte el tiempo de recepción del eco en distancia
  Serial.print("Volumen: ");
  //Imprime valores por el puerto serie:
  Volumen = (-1.253 * cm) + 100.00;
  delay(1000);
  Serial.print(Volumen);
  Serial.println();

  return Volumen ;
}

// FUNCION CONFIGURAR GSM:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void configuracion_SIM900_SMS() {

  /* Para mandar los comandos AT se ha utilizado unas funciones mediante punteros para hacer
    el programa más simple y corto. Por lo tanto, hacemos referencia a la función "mandarcomandoAT" y introducimos los valores necesarios.*/
  // Introduciremos esto: ("el comando AT que queremos enviar", "la respuesta que esperamos recibir", el tiempo que queramos que corra la función antes de salir de ahí).
  mandarcomandoAT("AT+CPIN=3588", "OK", 3588);    // Mandamos el comando AT para introducir el PIN y añadimos nuestro numero PIN.
  delay(2000);

  Serial.println("Conectando a la red...");    // Avisamos de que estamos conectándonos a la red.

  /* A continuación mandaremos el comando encargado de establecer la conexión a la red pero con un signo de interrogación
    para saber si el módulo tiene conexión a la red. Si nos responde con una de las dos respuestas esperadas es suficiente para efectuar las llamadas y SMS */
  while ( (mandarcomandoAT("AT+CREG?", "+CREG: 0,1", 5000) ||
           mandarcomandoAT("AT+CREG?", "+CREG: 0,5", 5000)) == 0 );

  Serial.println("Realizando ajustes de SMS...");
  mandarcomandoAT("AT+CMGF=1", "OK", 1000);    // Introducimos este comando para decirle al módulo que los SMS son en modo texto.
  mandarcomandoAT("AT+CPMS=\"SM\",\"SM\",\"SM\"", "OK", 1000);   //Seleccionamos la memoria de la SIM.
  mandarcomandoAT("AT+CMGD=1,4", "OK", 1000);    // Borraremos todos los SMS que tengamos en la memoria. El 1 especifica el índex y el 4 que queremos borrar todos los SMS.

  Serial.println("Con cobertura");    // Imprimimos un aviso para saber que el modulo está listo para una nueva orden.

}

//FUNCION GPRS (mandar SMS):::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

int Mandando_datos_via_SMS ( int Id_disposotivo , int Volumen , int fuego , int  bateria  )

{


  if (sms_enviado == 0) // Introducimos un while para que siga intentando mandar el SMS hasta conseguirlo.
  {
    Serial.println("Mandando SMS...");    // Imprimimos el aviso de mandar SMS.
    //sms_enviado=0;

    // A la hora de mandar el SMS también necesitamos meter el número de teléfono en un string. Por lo tanto, usaremos otra vez la función sprintf
    // y el carácter %s para indicar que el número irá ahí y será un string. Este string la metemos en la variable aux_string.
    sprintf(aux_string, "AT+CMGS=\"%s\"", numero_telefono);

    // Mandamos la variable aux_string con nuestro comando AT y el número de teléfono y esperamos la respuesta ">".
    respuesta = mandarcomandoAT(aux_string, ">", 2000);    // Si la respuesta del módulo es válida recibiremos un 1 de lo contrario un 0.
    if (respuesta == 1)
    {


       Serial.print(Id_disposotivo);
      Serial.print("/");

      Serial.print(Volumen);
      Serial.print("/");

      Serial.print(fuego);
      Serial.print("/");

      Serial.print(bateria);
      // Serial.print("/");
      Serial.write(0x1A);    // Mandamos este carácter para que el módulo sepa que hemos terminado de escribir y que tiene que mandar el SMS.
      respuesta = mandarcomandoAT("", "", 5000);    // Esperamos la respuesta OK que identifica que el SMS se ha enviado (no necesitamos mandar un comando AT)
      if (respuesta == 1)
      {
        Serial.println("Enviado SMS");    // Si la respuesta del módulo es OK imprimimos el aviso de que todo a ido bien.
        sms_enviado = 1;  // Cambiamos la variable a 1 para salir del while.

        Serial.print("sms_enviado= ");
        Serial.println(sms_enviado);

        respuesta = 0;
        //sms_cantidad=1;
        delay(1000);
        
      }
      else
      {
        Serial.println("Error en envio");    // Si el modulo no responde OK imprimimos Error.
      }
    }
    else
    {
      Serial.println("Error en envio");    // Si no encontramos el carácter ">" imprimimos Error.
      Serial.println(respuesta, DEC);    // Imprimimos la respuesta y especificamos que es en Decimal.
    }
  }

  return sms_enviado ;
}

//FUNCION MANDAR DATOS A LA WEB::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


void Mandando_datos_via_GET(String SMS_string, int Id_disposotivo , int Volumen , int fuego , int  bateria)

{

  String state1 = String(Volumen);

  // TCP connection
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "eontzia.zubirimanteoweb.com"; //http://eontzia.zubirimanteoweb.com 
  cmd += "\",80";
  ser.println(cmd);
  Serial.println(cmd);

  if (ser.find("Error")) {
    Serial.println("AT+CIPSTART error");
    return;
  }

  // preparar GET string

  String getStr = "GET http://eontzia.zubirimanteoweb.com/app/nuevaLectura/";

  getStr += SMS_string;

  getStr += "\r\n\r\n";

  // send data length
  cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  ser.println(cmd);
  Serial.println(cmd);

  if (ser.find(">")) {
    ser.print(getStr);
    Serial.print(getStr);

  }
  else {
    ser.println("AT+CIPCLOSE");
    // alert user
    Serial.println("AT+CIPCLOSE");
  }
  //SMS_string="";
}


//FUCNION RECIBIR DATOS SMS:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

String recibiendo_datos_via_SMS ( )

{


  while (sms_enviado == 1)
  {
    
    //respuesta = 0;
    int comprobar = 0;
    int x = 0;
    int counter = 0 ;

    while ((respuesta == 0) && (counter <= 20))

    {
      SMS_string = "";

      ULTRASONIDO ();
      sensor_humo();
      

      respuesta = mandarcomandoAT("AT+CMGR=1", "+CMGR:", 1000);
     
      SMS_string += String(Id_dispositivo);
      SMS_string += "/";
      SMS_string += String(Volumen);
      SMS_string += "/";
      SMS_string += String(fuego);
      SMS_string += "/";
      SMS_string += String(bateria);

      counter = counter + 1 ;
      Serial.println(counter);
     if (Volumen >=95)
     break ;
    
    }
  
    Serial.println("Recibiendo SMS...");
    respuesta = 0;
    memset(mensaje, '\0', 200);
    x = 0;

    do
    {
      if ((Serial.available() > 0) && (counter < 20))

      {

        if  (x < 56)
        {
          SMS_string = "";
        }

        mensaje[x] = Serial.read(); // Mientras haya datos sin leer en el serial, sigo leyendo y introduciéndolo en la variable serial.

        if  ((x >= 56) && (x <= 65))

        {
          SMS_string += String(mensaje[x]) ;
        }

        x++;
        
        // Buscamos el OK de fin de SMS.
        if (strstr(mensaje, "OK") != NULL)

        {
          comprobar = 1;  // Si encontramos el OK salimos del loop.
        }
 
 if (Volumen >=95)
     break ;
   

     
      }

    } while ((comprobar == 0) && (counter < 20)); // Condición para salir del loop.

    mensaje [x] = '\0';    // Como ya tenemos el mensaje completo, introducimos el carácter de fin de cadena.

    Serial.println(mensaje);    // Imprimimos el mensaje que hemos recibido.

    mandarcomandoAT("AT+CMGD=1,4", "OK", 1000);
    sms_enviado = 0;

  }
  return  SMS_string;

}
//FUNCION LUZ WIFI:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void blink_wifi(int n ,int p)

 {

// the loop function runs over and over again forever
 
  digitalWrite(led_wifi, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(n);              // wait for a second
  digitalWrite(led_wifi, LOW);    // turn the LED off by making the voltage LOW
  delay(p);              // wait for a second
  
 }
 


void software_Reboot()
{
  wdt_enable(WDTO_15MS);
  delay(50);
  while(1)
  {

  }
}

// FUNCION SENSOR DE HUMO::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

int sensor_humo()
{
  f=digitalRead(humo);
  Serial.print("fuego: ");
  Serial.println(fuego);
  fuego=!f ;
  delay(50);
  return fuego ;
}





