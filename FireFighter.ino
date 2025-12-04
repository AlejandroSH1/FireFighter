#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Pines
#define IN1 27
#define IN2 26
#define IN3 13
#define IN4 33
#define ENA 14
#define ENB 32
#define pump 12
#define ServoAguaPin 23
#define ServoRadarPin 15
#define FlameLeft 34
#define FlameMiddle 39
#define FlameRight 35
#define trigPin 4
#define echoPin 2
#define move_forward 300  
#define turning_time 300   
#define move_backward 150   
#define stop_delay 100

// WiFi y MQTT
const char* ssid = "LAPTOP-TJO9FBO2 1320";  //"MEGACABLE-2.4G-8BA5";"FERRUSCA2_4G";"GalaxyS24";"iPhone de Alex2";
const char* password = "Z-x64734";  //"4cH3+TgbQ9Q";"pedroeselmejorprofe37";"holamario";"yapontedatos"; 
const char* mqtt_server = "10.25.82.35";

// Tópicos
const char* topicFuego = "Detectar/Fuego";       // Para mostrar si hay fuego
const char* topicBomba = "Estado/Bomba";         // Estado de la bomba
const char* topicCarro = "Estado/Llantas";       // Estado del movimiento
const char* topicServo = "Posicion/Servo";       // Ángulo del servomotor de Agua
const char* topicServoR = "Posicion/ServoRadar"; // Ángulo del servomotor del Radar
const char* topicValSrAg = "Mover/ServoAgua";    // Mover el servo desde MQTT
const char* topicValSrRa = "Mover/ServoRadar";   // Mover el servo desde MQTT
const char* topicModo = "Robot/Modo";            // Si se mueve manualmente o autónomamente

WiFiClient espClient;
PubSubClient client(espClient);

// Variables
unsigned long currentMillis;

enum State { MOV_FORWARD, OBSTACLE, SCAN_LEFT, SCAN_RIGHT, DECIDE_TURN, TURNING };
State state = MOV_FORWARD;
unsigned long servoMoveStart = 0;
int leftDist = 0;
int rightDist = 0;

unsigned long turnStart = 0;
const unsigned long turnDuration = 400;

int fireThreshold = 500;
int motorSpeed = 255;
int limitDist = 35;

Servo servoAgua;
Servo servoRadar;

bool bombaEncendida = false;
bool fireDetected = false;
String modoRobot = "MANUAL";

// Movimiento
void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
}

void moveBackward() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
}

void moveForwardUltra() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 100);
  analogWrite(ENB, 100);
}

void turnLeft() {
  digitalWrite(IN1, HIGH); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
}

void turnRight() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); 
  digitalWrite(IN4, LOW);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
}

void turnLeftUltra() {
  digitalWrite(IN1, HIGH); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, 200);
  analogWrite(ENB, 200);
}

void turnRightUltra() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); 
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 200);
  analogWrite(ENB, 200);
}

void slightLeft() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed * 0.6);
}

void slightRight() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, motorSpeed * 0.6);
  analogWrite(ENB, motorSpeed);
}

void stopMoving() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0); 
  analogWrite(ENB, 0);
}

//Funciones
void sendMQTT(const char* topic, const char* msg) {
  if (client.connected()) {
    client.publish(topic, msg);
    Serial.printf("Publicado -> [%s]: %s\n", topic, msg);
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)message[i];
  msg.trim();
  Serial.printf("Mensaje MQTT recibido [%s]: %s\n", topic, msg.c_str());

  if (String(topic) == topicCarro) {
    if (msg == "ADELANTE") {
      moveForward(); 
    } else if (msg == "ATRAS") {
      moveBackward(); 
    } else if (msg == "IZQUIERDA") {
      turnLeft(); 
    } else if (msg == "DERECHA") {
      turnRight(); 
    } else if (msg == "STOP") {
      stopMoving(); 
    }
  }

  if (String(topic) == topicBomba){
    if (msg == "BOMBA_ON") {
      digitalWrite(pump, HIGH);
      bombaEncendida = true;
      sendMQTT(topicBomba, "Bomba encendida");
    } else if (msg == "BOMBA_OFF") {
      digitalWrite(pump, LOW);
      bombaEncendida = false;
      sendMQTT(topicBomba, "Bomba apagada");
    }
  }

  if (String(topic) == topicModo) {
    if (msg == "AUTO") {
      modoRobot = "AUTO";
    } else if (msg == "MANUAL") {
      modoRobot = "MANUAL";
      stopMoving();
    }
  }

  if (String(topic) == topicValSrAg) {
    int ang = msg.toInt();
    if (ang > 20 && ang <= 160) {
      servoAgua.write(ang);
      sendMQTT(topicServo, msg.c_str());
      Serial.printf("Servo Agua ajustado a %d°\n", ang);
    }
    if (ang == 0) {
      servoAgua.write(90);
    }
  }

  if (String(topic) == topicValSrRa) {
    int ang = msg.toInt();
    if (ang >= 20 && ang <= 160) {
      servoRadar.write(ang);
      sendMQTT(topicServoR, msg.c_str());
      Serial.printf("Servo Radar ajustado a %d°\n", ang);
    }
    if (ang == 0) {
      servoRadar.write(90);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando al broker...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado!");
      client.subscribe(topicCarro);
      client.subscribe(topicBomba);
      client.subscribe(topicModo);
      client.subscribe(topicValSrAg);
      client.subscribe(topicValSrRa);
      Serial.println("Suscrito a Tópicos");
      sendMQTT("Sistema/Estado", "Robot bombero conectado");
    } else {
      Serial.print("Error: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

//Escaneo con el ultrasónico
long getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  return distance;
}

void ultrasonico() {
  currentMillis = millis();
  long dist = getDistance();

  switch (state) {
    case MOV_FORWARD:
      moveForwardUltra();
      if (dist > 0 && dist < limitDist) {
        stopMoving();
        delay(100);
        servoRadar.write(90); 
        delay(200);
        state = OBSTACLE;
      }
      break;

    case OBSTACLE:
      servoRadar.write(150);
      servoMoveStart = currentMillis;
      state = SCAN_LEFT;
      break;

    case SCAN_LEFT:
      if (currentMillis - servoMoveStart >= 600) {
        leftDist = getDistance();
        servoRadar.write(30);
        servoMoveStart = currentMillis;
        state = SCAN_RIGHT;
      }
      break;

    case SCAN_RIGHT:
      if (currentMillis - servoMoveStart >= 600) {
        rightDist = getDistance();
        servoRadar.write(90);
        servoMoveStart = currentMillis;
        state = DECIDE_TURN;
      }
      break;

    case DECIDE_TURN:
      if (currentMillis - servoMoveStart >= 500) {
        if (leftDist > rightDist && leftDist > limitDist) {
          turnLeftUltra();
          turnStart = currentMillis;
          state = TURNING;
        } else if (rightDist > limitDist) {
          turnRightUltra();
          turnStart = currentMillis;
          state = TURNING;
        } else {
          turnRightUltra();
          turnStart = currentMillis * 2;
          state = TURNING;
        }
      }
      break;

    case TURNING:
      if (currentMillis - turnStart >= turnDuration) {
        stopMoving();
        delay(100);
        state = MOV_FORWARD;
      }
      break;
  }
}

// Deteccion de Fuego
void firefightMode(int left, int middle, int right) {
  // Fuego en la derecha y en medio
  if (middle < fireThreshold && right < fireThreshold) {
    moveBackward();
    delay(move_backward);
    stopMoving();
    delay(stop_delay);
    slightRight();
    delay(turning_time);
    stopMoving();
    delay(stop_delay);
  }
  // Fuego en la izquierda y en medio
  else if (middle < fireThreshold && left < fireThreshold) {
    moveBackward();
    delay(move_backward);
    stopMoving();
    delay(stop_delay);
    slightLeft();
    delay(turning_time);
    stopMoving();
    delay(stop_delay);
  }
  // Fuego solo en medio
  else if (middle < fireThreshold) {
    moveForward();
    delay(move_forward);
    stopMoving();
    delay(stop_delay);
  }
  // Fuego solo a la izquierda
  else if (left < fireThreshold) {
    moveBackward();
    delay(move_backward);
    stopMoving();
    delay(stop_delay);
    turnLeft();
    delay(turning_time);
    stopMoving();
  }
  // Fuego solo a la derecha
  else if (right < fireThreshold) {
    moveBackward();
    delay(move_backward);
    stopMoving();
    delay(stop_delay);
    turnRight();
    delay(turning_time);
    stopMoving();
  }
  else {
    stopMoving();
  }

  digitalWrite(pump, HIGH);
  for (int angle = 45; angle <= 135; angle += 5) {
    servoAgua.write(angle);
    delay(150);
  }
  for (int angle = 135; angle >= 45; angle -= 5) {
    servoAgua.write(angle);
    delay(150);
  }
  for (int angle = 45; angle <= 90; angle += 5) {
    servoAgua.write(angle);
    delay(150);
  }

  reset();
}

void reset() {
  moveBackward();
  delay(move_backward);
  stopMoving();   
  digitalWrite(pump, LOW);
  servoAgua.write(90);
}

// SetUp
void setup() {
  Serial.begin(9600);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(FlameLeft, INPUT);
  pinMode(FlameMiddle, INPUT);
  pinMode(FlameRight, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  digitalWrite(pump, LOW);
  servoAgua.attach(ServoAguaPin);
  servoAgua.write(90);
  servoRadar.attach(ServoRadarPin);
  servoRadar.write(90);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// Loop
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Leer sensores de flama
  int leftSensor = analogRead(FlameLeft);
  int middleSensor = analogRead(FlameMiddle);
  int rightSensor = analogRead(FlameRight);

  if (leftSensor < fireThreshold || middleSensor < fireThreshold || rightSensor < fireThreshold) {
    fireDetected = true;
    sendMQTT(topicFuego, "Fuego detectado");
  } else {
    fireDetected = false;
    sendMQTT(topicFuego, "Sin fuego");
    }

  if (fireDetected) {
    firefightMode(leftSensor, middleSensor, rightSensor);
  }

  if (modoRobot == "AUTO") {
      ultrasonico();
  }

  char bufferAgua[10];
  char bufferRadar[10];

  sprintf(bufferAgua, "%d", servoAgua.read());
  sprintf(bufferRadar, "%d", servoRadar.read());

  sendMQTT(topicServo, bufferAgua);   // Servo de agua
  sendMQTT(topicServoR, bufferRadar); // Servo del radar
}
