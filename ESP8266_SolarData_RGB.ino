#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>



#define TRIGGER_PIN 0               //pin utilizado para resetear los valores de por defecto. Con esto se inicia el asistente de configuración. (debe pulsarse cuando los LED están en azul)
#define PIN 2                       //pin de salida donde se conecta el bus de información de los neopixels (LED's)
#define NUMPIXELS 8                 //numero de LED's conectados a la matriz
#define TIEMPO_RESET 2500           //tiempo en milisegundos del tiempo de ventana para resetear los valores por defecto.

#define L_Good      0x9600          //Verde
#define L_Poor      0x960000        //Rojo
#define L_Fair      0x969600        //Amarillo
#define L_Reset     0x96            //Azul
#define L_Off       0x0             //Apagado
#define L_Program   0x892496        //Morado

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


//------------------------------------------------------------- VARIABLES GLOBALES -------------------------------------------------------------
char host[20] = "www.hamqsl.com";   //dir 0   host por defecto al que se va a conectar
char Url[20] = "/solarxml.php";     //dir 20  direccion por defecto del host donde hacemos la petición del XML
char Tiempo[10] = "5";              //dir 40  tiempo en segundos por defecto para apagar los leds (si apagar es false no se tiene en cuenta)
char Apagar[5] = "true";            //dir 50  establece si se apagarán los LED tras un periodo de tiempo
char Brillo[3] = "30";              //dir 55  intensidad de iluminación de los LED comprendido en tre 0 y 255

char Estado[8];                     //variable usada para almacenar "G","F","P" (Good, Fair, Poor) correlativamente segun nos los proporciona el host.
char brillo = 30;                   //variable convertida a información interpretable de la variable Brill[3] declarada anteriormente
int TmpApagado = 5000;              //variable convertida a tipo int multiplicada por 1000, tiempo tras apadado en milisegundos
bool apagar = true;                 //variable convertida a tipo booeana

bool shouldSaveConfig = false;      //Bandera para guardar los datos

//------------------------------------------------------------- Llamada activa el guardado de datos -------------------------------------------------------------
void saveConfigCallback () {
  shouldSaveConfig = true;
}

//------------------------------------------------------------- Llamada antes de entrar en modo AP -------------------------------------------------------------
void configModeCallback(WiFiManager *myWiFiManager){
  allColor(L_Program);
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

//------------------------------------------------------------- Función para grabar en la EEPROM -------------------------------------------------------------
void grabar(int addr, char* inchar, int tamano) {
  for (int i = 0; i < tamano; i++) {
    if(inchar[i] == 0) EEPROM.write(addr+i, 255);
    else EEPROM.write(addr+i, inchar[i]);
  }
  EEPROM.commit();
  delay(20);
}

//------------------------------------------------------------- Función para leer la EEPROM -------------------------------------------------------------
void leer(int addr, int tm, char* lect) 
{
   byte lectura;
   int e = 0;
   for (int i = addr; i < (addr + tm); i++) 
   {
      lectura = EEPROM.read(i);
      if (lectura != 255) lect[e] = (char)lectura;
      else lect[e] = 0;
      e++;     
   }
}

//------------------------------------------------------------- CARGAR DATOS DE LA EEPROM -------------------------------------------------------------
void cargaDatos()
{
  leer(0,20, host);
  leer(20,20, Url);     
  leer(40,10, Tiempo);
  leer(50,5, Apagar);
  leer(55,3, Brillo);

  refrescaDatos();
}

//------------------------------------------------------------- CONVIERTE LOS DATOS A SUS VARIABLES UTILIZABLES -------------------------------------------------------------
void refrescaDatos()
{  
  String temp="";
  
  for(int i= 0; i<10; i++)  
    if(Tiempo[i] != 0) temp+= Tiempo[i];
  TmpApagado = temp.toInt() * 1000;

  temp="";
  for(int i= 0; i<3; i++)  
    if(Brillo[i] != 0) temp+= Brillo[i];
  brillo = (char)temp.toInt();  

  if(Apagar[0] == 'f') apagar = false;
  else if(Apagar[0] == 'F') apagar = false;
  else apagar = true;
  
  Serial.println();
  Serial.println("Carga de datos:");
  Serial.print("Host------> "); Serial.println(host);
  Serial.print("Url-------> "); Serial.println(Url);
  Serial.print("Tiempo----> "); Serial.println(Tiempo);
  Serial.print("Apagar----> "); Serial.println(Apagar);
  Serial.print("Brillo----> "); Serial.println(Brillo);
}

//------------------------------------------------------------- FUNCION "SPLIT" -------------------------------------------------------------
//Busca los estados de propagación en el texto recibido del XML consultado en la web
//introduce en la matriz de 8 bytes los caracteres 'G','F','P' segun los estados GOOD, FAIR o POOR respectivamente
bool Split(String texto)
{
  bool retorno = false;
  int puntero = 0;
  for(int i =0; i < 4; i++)
  {
    puntero = texto.indexOf("\"day\">", puntero + 1);
    if(puntero < 0) break;
    Estado[i] = texto[puntero + 6];
    retorno = true;
  }
  puntero = 0;
  for(int i =0; i < 4; i++)
  {
    puntero = texto.indexOf("\"night\">", puntero + 1);
    if(puntero < 0) break;
    Estado[i+4] = texto[puntero + 8];
  }
  return retorno;
}

//------------------------------------------------------------- FUNCIÓN TRADUCTOR DE CARÁCTERES A COLORES -------------------------------------------------------------
//traduce los caracteres 'G','F','P' a los colores correspondientes
uint32_t SetColor (char est)
{
  if(est == 'G') return L_Good;      //Verde
  if(est == 'F') return L_Fair;  //Amarillo
  if(est == 'P') return L_Poor;     //Rojo
  return pixels.Color(0,0,0);
}

//------------------------------------------------------------- FUNCIÓN PINTA TODOS LOS LEDS  -------------------------------------------------------------
void allColor(uint32_t _color)
{
  for(int i=0; i< NUMPIXELS; i++)  
    pixels.setPixelColor(i, _color);
  pixels.show();
}

//------------------------------------------------------------- SECUENCIA DE APAGADO  -------------------------------------------------------------
void Sec_Apagado()
{
  delay(TmpApagado);              //espera el tiempo programado
        
        for(int i=0; i< 4; i++)         //realizamos 4 parpadeos de los leds
        {
          pixels.setBrightness(10);
          pixels.show();
          delay(500);
          pixels.setBrightness(brillo);
          pixels.show();
          delay(500);
          Serial.print("*");
        }        
        allColor(L_Off);            //apagamos todos los leds
}

//---------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------- SETUP -------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void setup() {
  
  Serial.begin(115200);                 //iniciamos puerto serie a 115200
  Serial.println();

  pinMode(TRIGGER_PIN, INPUT);          //pin TRIGGER como entrada
  pixels.begin();                       //iniciamos la libreria de los neopixels
  pixels.setBrightness(brillo);         //asigna el la intensidad de brillo de los led's
  allColor(L_Off);                      //apaga todos los LED's

  EEPROM.begin(256);                    //iniciamos la libreria de manejo de la EEPROM

  

  //Creamos las variables para los nuevos parámetros que serán visualizados en la web de configuración
  WiFiManagerParameter custom_text0("<p><strong> Servidor Solar-Terrestrial Data XML </p> </strong>");
  WiFiManagerParameter custom_xml_host("host", "host XML", host, 21);
  WiFiManagerParameter custom_xml_url("url", "url XML", Url, 21);
  WiFiManagerParameter custom_text1("<p><strong> Configuración </p> </strong>");
  WiFiManagerParameter custom_Brillo("Brillo", "Brillo 0-255", Brillo, 4);
  WiFiManagerParameter custom_Apagar("Apagar", "Apagar true or false", Apagar, 6);
  WiFiManagerParameter custom_Tiempo("Tiempo", "Tiempo apagado en segundos", Tiempo, 11);

  WiFiManager wifiManager;              //iniciamos la libreria del mánager Wifi

  //Seteamos la llamada a la función de guardado de datos
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //Seteamos la llamada a función antes de entrar en modo AP
  wifiManager.setAPCallback(configModeCallback);
  
    
  //Introducimos los parametros personalizados de la web 
  wifiManager.addParameter(&custom_text0);  
  wifiManager.addParameter(&custom_xml_host);
  wifiManager.addParameter(&custom_xml_url);
  wifiManager.addParameter(&custom_text1);
  wifiManager.addParameter(&custom_Brillo);
  wifiManager.addParameter(&custom_Apagar);
  wifiManager.addParameter(&custom_Tiempo);


  
  //********************************** VENTANA PARA RESETEAR A VALORES DE FABRICA *****************
  allColor(L_Reset);                                                     //ponemos todos los led en azul  
  delay(TIEMPO_RESET);                                                  //esperamos el tiempo asignado
  
  if ( digitalRead(TRIGGER_PIN) == LOW ) wifiManager.resetSettings();   //si el botón está pulsado reseteamos valores por defecto
  else 
  {
    cargaDatos();                                                       //sino cargamos los valores guardados en la EEPROM
    pixels.setBrightness(brillo);                                       //seteamos el brillo de los led's según la configuración guardada
  } 
    
  allColor(L_Off);                                                     //apagamos todos los led's
  
  wifiManager.setDebugOutput(false);                                    //modo DEBUG de wifimanager apagado

  //************************************************FIN VENTANA ************************************

  //Loop del mánager
  //Si no consigue conectarse a la Wifi o hemor realizado un reinicio de valores de "fabrica" crea un Acces Point con la web de configuración
  //debe redireccionar automaticamente, en todo caso su dirección es 192.168.4.1
  if (!wifiManager.autoConnect("AP_SolarData")) {
    Serial.println("Fallo de conexión");
    delay(3000);
    //Si los datos de conexíon no son correctos resetea el ESP
    ESP.reset();
    delay(5000);
  }

  //Wifi conectada
  Serial.println();
  Serial.println("Conectado:");

  //Leemos los datos devueltos de la página de configuración y los asignamos a sus variables
  strcpy(host, custom_xml_host.getValue());
  strcpy(Url, custom_xml_url.getValue());
  strcpy(Brillo, custom_Brillo.getValue());
  strcpy(Apagar, custom_Apagar.getValue());
  strcpy(Tiempo, custom_Tiempo.getValue());

  //Salvamos parametros personalizados
  if (shouldSaveConfig) {
    Serial.println("Guardando configuración");
           
    grabar(0, host, 20);    
    grabar(20, Url, 20);    
    grabar(40, Tiempo, 10);    
    grabar(50, Apagar, 5);    
    grabar(55, Brillo, 3);

    refrescaDatos();
  }

  Serial.print("Ip local: ");
  Serial.println(WiFi.localIP());

}



//--------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------- LOOP -----------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------------------
void loop() {
   //Conexion al host ***************************************************
  
    Serial.println(""); 
    Serial.print("Conectando a ");
    Serial.println(host);
    
    WiFiClient client;                              //Creamos el cliente de conexión TCP
    const int httpPort = 80;                        //puerto a usar
    if (!client.connect(host, httpPort)) {          //intentamos conectar al host y puertos asignados
      Serial.println("Conexión fallida");
      return;
    }
    
    
    String url(Url);                                //Convertimos a string la url de la petición XML
    Serial.print("Solicitando URL: ");
    Serial.println(url);
    
    // Enviamos la petición al servidor
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    
    while (client.available() == 0) {               //esperamos respuesta del servidor  
        if (millis() - timeout > 5000) {
        Serial.println(">>> Tiempo de espera del cliente!");
        client.stop();                              //si pasa el tiempo de espera cerramos la conexión
        return;
      }
    }
    
    while(client.available()){                      //si tenemos respuesta del servidor
      String line = client.readStringUntil('\r');   //leemos las lineas enviadas
      if(Split(line)) 
      { 
        client.stop();                                //las enviamos al decodificador para obtener los datos requeridos
        Serial.print(line);                           //imprimimos las lineas por el puerto serie   
        break;
      }   
    }
    
    Serial.println();
    Serial.println("Cerrando conexión...");

    //Seteamos los colores de los pixels según los datos obtenidos del XML
    for(int i=0; i < NUMPIXELS; i++) pixels.setPixelColor(i, SetColor(Estado[i]));
    pixels.show();

    
    if(apagar)  Sec_Apagado();                        //Si en la configuración está seteado el apagado de los leds
    
      
    Serial.println("Durmiendo ESP...");
    ESP.deepSleep(0);                       //Una vez finalizada la consulta, se apaguen o no los LED pasamos el ESP en modo de bajo consumo
                                            //solo despierta al resetearlo
}
