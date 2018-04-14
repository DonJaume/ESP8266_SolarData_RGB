# ESP8266 SolarData-RGB

Se trata de un sencillo circuito que nos muestra en una tira LED WS2812B, por medio de colores, el estado de las bandas HF de radioaficionados, obteniendo dicha información desde la conocida página web hamqsl.com.

Para el control y conexión a la red, se utiliza el módulo ESP8266, utilizado en este circuito el primer modelo ESP-01, que es el mas simple y pequeño de la serie pero suficiente para este proyecto.

## Diseño del circuito
...

...

...


## Programación
1. Instalar la última versión de [Arduino](https://www.arduino.cc/en/Main/Software).
2. Instalar la tarjeta del ESP8266.
	- Abrir la aplicación de aplicación de Arduino e ir a Preferencias desde el menú Archivo.
	- Introducir *http://arduino&#46;esp8266&#46;com/stable/package_esp8266com_index&#46;json* en "Gestor de URLs Adicionales de Tarjetas" (puede introducir varias direcciones separandolas por comas ',').
	- Abrir *Gestor de tarjetas* desde el menú Herramientas/Placa. Buscamos por ESP8266 e instalamos.
	- Podéis encontrar mas información u otros modos de instalación en su [página de GitHub.](https://github.com/esp8266/Arduino) 
3. Instalación de la librería que controla los LED's.
	- Desde el menú Programa/Incluir Librería, abrir *Gestionar Librerías*. Buscar e instalar la librería llamada **Adafruit NeoPixel** by Adafruit.


