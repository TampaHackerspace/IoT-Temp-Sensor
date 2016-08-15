
const int delayBetweenReads = 5000;

const int sensorPin = A0;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  int sensorVal = analogRead(sensorPin);
  double voltage = (sensorVal / 1024.0) * 5.0;
  double currentTemp = (voltage - .5) * 100;
  Serial.println(String(currentTemp));

  
  delay(delayBetweenReads);
}
