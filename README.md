# FirefIghter - Robot Bombero con ESP32 y Node-RED

Este proyecto consiste en un robot autónomo basado en **ESP32** que puede detectar fuego mediante sensores infrarrojos, moverse en distintas direcciones y activar una bomba de agua para apagarlo. Además, se integra con **Node-RED** mediante **MQTT**, permitiendo control manual y monitoreo desde una interfaz web.

## Componentes principales

* ESP32
* 3 Sensores de flama analógicos (izquierda, centro, derecha)
* Módulo de motor L298N 
* 4 Motores de DC con rueda
* Bomba de agua + relay
* Servomotor (para controlar la dirección del agua)
* Fuente de poder (baterías de 3V)
* Node-RED + Mosquitto

## Funcionalidades

* **Detección de fuego** usando sensores infrarrojos.
* **Movimiento** controlado por Node-RED:

  * Adelante
  * Atrás
  * Izquierda
  * Derecha
* **Modo automático de apagado de incendio**: cuando detecta fuego, el robot:

  * Se posiciona
  * Mueve el servomotor para rociar
  * Activa la bomba
* **Interfaz de control** en Node-RED con botones y alertas.
* **Comunicación MQTT** para enviar/recibir comandos y estados.

## Tópicos MQTT

| Propósito               | Tópico           | Dirección |
| ----------------------- | ---------------- | --------- |
| Movimiento del robot    | `Estado/Llantas` | Enviar    |
| Estado de la bomba      | `Estado/Bomba`   | Enviar    |
| Detección de fuego      | `Detectar/Fuego` | Recibir   |
| Estado del sistema      | `Sistema/Estado` | Recibir   |
| Posición del servomotor | `Posicion/Servo` | Recibir   |


## Lógica del robot

Cuando se detecta fuego:

1. El robot evalúa la dirección de la fuente (izquierda, centro o derecha).
2. Mueve las llantas hacia esa dirección y retrocede según sea necesario.
3. Activa la bomba y comienza el barrido del servomotor.
4. Publica estados en MQTT y regresa a modo reposo.

## Interfaz en Node-RED

* Botones para enviar comandos de movimiento.
* Switch para activar o desactivar la bomba manualmente.
* Indicadores visuales para mostrar:

  * Fuego detectado
  * Estado del robot

## Código

El proyecto está desarrollado en **Arduino IDE** usando las siguientes librerías:

* `ESP32Servo`
* `WiFi`
* `PubSubClient`


## Cómo comenzar

1. Configura tu broker MQTT (por ejemplo, Mosquitto).
2. Abre y ejecuta Node-RED.
3. Carga el código de ESP32 en tu placa desde Arduino IDE.
4. Conecta el ESP32 a tu red WiFi.
5. ¡Empieza a controlar el robot desde Node-RED!


## Licencia

Este proyecto es de libre uso.
