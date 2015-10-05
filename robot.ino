#include <Ultrasonic.h>
#include <AFMotor.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

/* Assign a unique ID to this sensor at the same time */
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

#define TRIGGER_PIN  15
#define ECHO_PIN     14
#define BEEP         16

Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN);
AF_DCMotor motorE(1); 
AF_DCMotor motorD(2);

boolean som = false;
boolean obstaculo;

const int margemAngulo = 3;
int anguloAlvo = 0;
int anguloAtual = 0;

const int vel = 180;

int velocidadeE = 0;
int velocidadeD = 0;

void displaySensorDetails(void) {
  sensor_t sensor;
  mag.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" uT");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" uT");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" uT");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

int verificarDistancia() {
  float cmMsec;
  long microsec = ultrasonic.timing();

  cmMsec = ultrasonic.convert(microsec, Ultrasonic::CM);
    
  Serial.print("CM: ");
  Serial.println(cmMsec);  
  if (cmMsec < 60) {
       Serial.println(microsec);
  }
  return (int) cmMsec; 
}

void andarPraFrente() {
  motorE.setSpeed(velocidadeE);   //Define a velocidade maxima
  motorE.run(FORWARD);            //Gira o motor sentido horario
  motorD.setSpeed(velocidadeD);   //Define a velocidade maxima
  motorD.run(FORWARD);            //Gira o motor sentido horario
  delay(100);
}

void darRe(int tempo) {
  motorE.setSpeed(vel);       //Define a velocidade maxima
  motorE.run(BACKWARD);       //Gira o motor sentido horario
  motorD.setSpeed(vel);       //Define a velocidade maxima
  motorD.run(BACKWARD);       //Gira o motor sentido horario
  delay(tempo);
  parar();
}

void parar() {
  motorE.setSpeed(0); 
  motorE.run(RELEASE);
  motorD.setSpeed(0); 
  motorD.run(RELEASE);
}

void virarDireita() {
  motorE.setSpeed(vel); 
  motorE.run(FORWARD);
  motorD.setSpeed(vel); 
  motorD.run(BACKWARD);
  delay(400);
  parar();  
}

void setup() {

  Serial.begin(9600);
  Serial.println("HMC5883 Magnetometer Test"); Serial.println("");
  pinMode(BEEP,OUTPUT);

  /* Initialise the sensor */
  if(!mag.begin()) {
    /* There was a problem detecting the HMC5883 ... check your connections */
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while(1);
  }
  
  /* Display some basic information on this sensor */
  displaySensorDetails();
  
  velocidadeE = vel;
  velocidadeD = vel;
  
  anguloAlvo = pegarBussola();
}

void loop() {
  if (!som) {
    analogWrite(BEEP,255);
    som = true;
  } else {
    analogWrite(BEEP,0);
    som = false;
  }
  
  anguloAtual = pegarBussola();
  calcDesiredTurn();
  
  if (verificarDistancia() < 25) obstaculo = true;
  else obstaculo = false;
  
  if (obstaculo) {
    //virarDireita
    parar();
    delay(200);
    darRe(500); 
    delay(500);
    virarDireita();
    delay(500);
    anguloAlvo = pegarBussola();
  } else {
    //andarPraFrent
    andarPraFrente();
  }
 delay(100); 
}

int pegarBussola() {
  /* Get a new sensor event */ 
  sensors_event_t event; 
  mag.getEvent(&event);
 
  /* Display the results (magnetic vector values are in micro-Tesla (uT)) */
  Serial.print("X: "); Serial.print(event.magnetic.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.magnetic.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.magnetic.z); Serial.print("  ");Serial.println("uT");

  // Hold the module so that Z is pointing 'up' and you can measure the heading with x&y
  // Calculate heading when the magnetometer is level, then correct for signs of axis.
  float heading = atan2(event.magnetic.x, event.magnetic.y); //referência é o eixo y!!
  
  // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your location.
  // Find yours here: http://www.magnetic-declination.com/
  // Mine is: -13* 2' W, which is ~13 Degrees, or (which we need) 0.22 radians
  // If you cannot find your Declination, comment out these two lines, your compass will be slightly off.
  float declinationAngle = -0.38; // como mudou a referência, utilizamos a declinação negativa.
  
  heading += declinationAngle;
  
  // Correct for when signs are reversed.
  if(heading < 0)
    heading += 2*PI;
    
  // Check for wrap due to addition of declination.
  if(heading > 2*PI)
    heading -= 2*PI;
   
  // Convert radians to degrees for readability.
  float headingDegrees = heading * 180/M_PI; 
  
  Serial.print("Heading (degrees): "); Serial.println(headingDegrees);
  return (int)headingDegrees;
}

void calcDesiredTurn(void) {
  // calculate where we need to turn to head to destination
  int erroAtual = anguloAlvo - anguloAtual;

  // adjust for compass wrap
  if (erroAtual < -180)      
    erroAtual += 360;
  if (erroAtual > 180)
    erroAtual -= 360;

  // calculate which way to turn to intercept the targetHeading
  if (abs(erroAtual) <= margemAngulo) {      // if within tolerance, don't turn
    velocidadeD = vel;
    velocidadeE = vel;
  }
  else if (erroAtual < 0) {
    velocidadeE = vel - 30;
    velocidadeD = vel;
  }
  else if (erroAtual > 0) {
    velocidadeD = vel - 30;
    velocidadeE = vel;
  }
  else {
    velocidadeD = vel;
    velocidadeE = vel;
  }
 
}  // calcDesiredTurn()
