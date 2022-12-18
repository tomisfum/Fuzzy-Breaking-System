#include "UbidotsEsp32Mqtt.h"
#include <Fuzzy.h>

const char *UBIDOTS_TOKEN = "ubidots tokens";
const char *WIFI_SSID = "your ssid";
const char *WIFI_PASS = "your password";
const char *DEVICE_LABEL = "device label ubidots";
const char *VARIABLE_LABEL = "variabel label ubidots";

// Motor A
const int motor1Pin1 = 27;
const int motor1Pin2 = 26;
const int enable1Pin1 = 22;

// Motor B
const int motor2Pin1 = 25;
const int motor2Pin2 = 33;
const int enable1Pin2 = 23;

const int trigPin2 = 21;
const int echoPin2 = 19;


// PWM
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 255;

//ultrasonic
long duration;
int distance;

/ Fuzzy
Fuzzy *fuzzy = new Fuzzy();

FuzzySet *dekat     = new FuzzySet(0, 20, 40, 60);
FuzzySet *medium_in = new FuzzySet(50, 70, 90, 110);
FuzzySet *jauh      = new FuzzySet(100, 120, 140, 160 );

FuzzySet *lambat        = new FuzzySet(0, 50, 75, 100);
FuzzySet *medium_out    = new FuzzySet(80 , 125, 150, 190);
FuzzySet *cepat         = new FuzzySet(170, 200, 225, 255);

const int PUBLISH_FREQUENCY = 5000;

unsigned long timer;
uint8_t analogPin = 34; //GPIO34 ADC_CH6.

Ubidots ubidots(UBIDOTS_TOKEN);

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

int ultrasonic, motor;

void setup()
{
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin1, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(enable1Pin2, OUTPUT);

  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  ledcSetup(pwmChannel, freq, resolution);

  // GPIO
  ledcAttachPin(enable1Pin1, pwmChannel);
  ledcAttachPin(enable1Pin2, pwmChannel);

  Serial.begin(115200);

  //input
  FuzzyInput *fuzz_jarak = new FuzzyInput(1);

  fuzz_jarak->addFuzzySet(dekat);
  fuzz_jarak->addFuzzySet(medium_in);
  fuzz_jarak->addFuzzySet(jauh);
  fuzzy->addFuzzyInput(fuzz_jarak);

  //output
  FuzzyOutput *fuzz_kecepatan = new FuzzyOutput(1);

  fuzz_kecepatan->addFuzzySet(lambat);
  fuzz_kecepatan->addFuzzySet(medium_out);
  fuzz_kecepatan->addFuzzySet(cepat);
  fuzzy->addFuzzyOutput(fuzz_kecepatan);

  //Fuzzy Rule Base

  //Rule01
  FuzzyRuleAntecedent *if_jarak_dekat = new FuzzyRuleAntecedent();
  if_jarak_dekat->joinSingle(dekat);
  FuzzyRuleConsequent *then_kecepatan_lambat = new FuzzyRuleConsequent();
  then_kecepatan_lambat->addOutput(lambat);

  FuzzyRule* fuzzyRule01 = new FuzzyRule(1, if_jarak_dekat, then_kecepatan_lambat);

  //Rule02
  FuzzyRuleAntecedent *if_jarak_medium = new FuzzyRuleAntecedent();
  if_jarak_medium->joinSingle(medium_in);
  FuzzyRuleConsequent *then_kecepatan_medium = new FuzzyRuleConsequent();
  then_kecepatan_medium->addOutput(medium_out);

  FuzzyRule* fuzzyRule02 = new FuzzyRule(2, if_jarak_medium, then_kecepatan_medium);

  //Rule03
  FuzzyRuleAntecedent *if_jarak_jauh = new FuzzyRuleAntecedent();
  if_jarak_jauh->joinSingle(jauh);
  FuzzyRuleConsequent *then_kecepatan_cepat = new FuzzyRuleConsequent();
  then_kecepatan_cepat->addOutput(cepat);

  FuzzyRule* fuzzyRule03 = new FuzzyRule(3, if_jarak_jauh, then_kecepatan_cepat);

  //Added fuzzy rules
  fuzzy->addFuzzyRule(fuzzyRule01);
  fuzzy->addFuzzyRule(fuzzyRule02);
  fuzzy->addFuzzyRule(fuzzyRule03);

  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();

  timer = millis();
}

void loop()
{
  jarak();
  int input_value = distance;
  fuzzy->setInput(1, input_value);
  fuzzy->fuzzify();
  float output_value = fuzzy->defuzzify(1);
  Serial.print("output: ");
  Serial.println(output_value);
  dutyCycle = output_value;
  if (distance > 160) {
    dutyCycle = 255;
  }
  ledcWrite(pwmChannel, dutyCycle);
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);

  ultrasonic = distance;
  motor = dutyCycle;

  if (!ubidots.connected())
  {
    ubidots.reconnect();
  }
  if (abs(millis() - timer) > PUBLISH_FREQUENCY) // trigger regularly for 5 seconds
  {
    int value = distance;
    ubidots.add("ultrasonic", value); // Enter the Labels variable and value values ​​to be sent to the database
    delay (500);
    int value1 = dutyCycle;
    ubidots.add("motor", value1); // Enter the Labels variable and value values ​​to be sent to the database
    ubidots.publish("Demo");
    timer = millis();
    delay (500);
  }
  ubidots.loop();
}

void jarak () {
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  duration = pulseIn(echoPin2, HIGH);
  distance = duration / 58.2;
  Serial.print("Jarak : ");
  Serial.print(distance);
  Serial.println(" cm");
}
