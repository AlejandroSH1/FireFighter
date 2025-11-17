#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define IN1 27
#define IN2 26
#define IN3 25
#define IN4 33
#define ENA 14
#define ENB 32
#define pump 12
#define ServoPin 13
#define Servo2Pin 15
#define FlameLeft 34
#define FlameMiddle 39
#define FlameRight 35
#define inbuilt_led 2

unsigned long anterior = 0;
unsigned long intervalo = 0;
bool ejecutandose = false;
String accionActual = "";

int fireThreshold = 500;
int motorSpeed = 200;

// ----- WiFi y MQTT -----
const char* ssid = "LAPTOP-TJO9FBO2 1320"; //"MEGACABLE-2.4G-8BA5";"FERRUSCA2_4G";"GalaxyS24";"iPhone de Alex2";
const char* password = "Z-x64734"; //"4cH3TgbQ9Q";"pedroeselmejorprofe37";"holamario";"yapontedatos"; 
const char* mqtt_server = "10.25.86.116"; //"192.168.100.22";"192.168.43.114";

const char* topicFuego = "Detectar/Fuego";
const char* topicBomba = "Estado/Bomba";
const char* topicCarro = "Estado/Llantas";
const char* topicServo = "Posicion/Servo";

WiFiClient espClient;
PubSubClient client(espClient);

Servo myServo;
bool bombaEncendida = false;
bool fireDetected = false;

// Funciones de Movimiento
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

void stopMoving() {
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0); 
  analogWrite(ENB, 0);
}

// Funciones MQTT
void sendMQTT(const char* topic, const char* msg) {
  if (client.connected()) client.publish(topic, msg);
}

void startAction(String accion, unsigned long duracion) {
  accionActual = accion;
  anterior = millis();
  intervalo = duracion;
  ejecutandose = true;

  if (accion == "ADELANTE") moveForward();
  else if (accion == "ATRAS") moveBackward();
  else if (accion == "IZQUIERDA") turnLeft();
  else if (accion == "DERECHA") turnRight();
  else if (accion == "STOP") stopMoving();
}

void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)message[i];
  msg.trim();

  if (String(topic) == topicCarro) {
    if (msg == "ADELANTE") startAction("ADELANTE", 300);
    else if (msg == "ATRAS") startAction("ATRAS", 150);
    else if (msg == "IZQUIERDA") startAction("IZQUIERDA", 300);
    else if (msg == "DERECHA") startAction("DERECHA", 300);
    else if (msg == "STOP") startAction("STOP", 100);
  }

  if (String(topic) == topicBomba) {
    if (msg == "BOMBA_ON") {
      digitalWrite(pump, HIGH); bombaEncendida = true;
      sendMQTT(topicBomba, "Bomba encendida");
    } else if (msg == "BOMBA_OFF") {
      digitalWrite(pump, LOW); bombaEncendida = false;
      sendMQTT(topicBomba, "Bomba apagada");
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
      sendMQTT("Sistema/Estado", "Robot bombero conectado");
    } else {
      Serial.print("Error: ");
      Serial.println(client.state());
      delay(1000);
    }
  }
}

// Modo Fuego
void firefightMode(int left, int middle, int right) {
  static unsigned long fireMillis = 0;
  static int servoAngle = 90;
  static bool sweepingRight = true;

  digitalWrite(inbuilt_led, HIGH);

  // Movimiento basado en sensores
  if (middle < fireThreshold && right < fireThreshold){
    startAction("ATRAS", 150);
    startAction("DERECHA", 50);
  }
  else if (middle < fireThreshold && left < fireThreshold){
    startAction("ATRAS", 150);
    startAction("IZQUIERDA", 50);
  }
  else if (middle < fireThreshold){
    startAction("ADELANTE", 300);
  }
  else if (left < fireThreshold){
    startAction("ATRAS", 150);
    startAction("IZQUIERDA", 300);
  }
  else if (right < fireThreshold){
    startAction("ATRAS", 150);
    startAction("DERECHA", 300);
  }
  else stopMoving();

  digitalWrite(pump, HIGH);

  // Movimiento del servo
  if (millis() - fireMillis >= 150) {
    fireMillis = millis();
    if (sweepingRight) {
      servoAngle += 5;
      if (servoAngle >= 135) sweepingRight = false;
    } else {
      servoAngle -= 5;
      if (servoAngle <= 45) sweepingRight = true;
    }
    myServo.write(servoAngle);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); 
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); 
  pinMode(ENB, OUTPUT);
  pinMode(pump, OUTPUT); 
  pinMode(inbuilt_led, OUTPUT);
  pinMode(FlameLeft, INPUT); 
  pinMode(FlameMiddle, INPUT);
  pinMode(FlameRight, INPUT);

  myServo.attach(ServoPin);
  myServo.write(90);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(500);
  }
  Serial.println("\nWiFi conectado!");
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Terminar acciones automáticas por tiempo
  if (ejecutandose && millis() - anterior >= intervalo) {
    stopMoving();
    ejecutandose = false;
  }

  int leftSensor = analogRead(FlameLeft);
  int middleSensor = analogRead(FlameMiddle);
  int rightSensor = analogRead(FlameRight);

  fireDetected = (leftSensor < fireThreshold || middleSensor < fireThreshold || rightSensor < fireThreshold);

  if (fireDetected) {
    sendMQTT(topicFuego, "Fuego detectado");
    firefightMode(leftSensor, middleSensor, rightSensor);
  } else {
    sendMQTT(topicFuego, "Sin fuego");
    digitalWrite(pump, LOW);
    myServo.write(90);
    digitalWrite(inbuilt_led, LOW);
  }
}