//**************************************LIB**************************************************
#include "Arduino.h"
//#include <Adafruit_Sensor.h>
//#include <DHT.h>
//#include <DHT_U.h>

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
//**************************************Detector Setup**************************************************
#define               refresh            (5000)             // delay beetween screen refreshes 
#define               waitingtime        (5)                // waiting time before skip connection check
#define               PulsePin           (3)                // connection pulse in PIN locked to pin 2-3 in for arduino uno
#define               ADCPin             (A0)               // ADC pin
#define               PulseMax           (1024)             // adc channel of max pulse duration
#define               N                  (1024)             // number of channels
#define               PulseMin           (5)                // channel cut (short pulses due to noise)

//**************************************DHT22 Setup**************************************************
#define               DHTPIN             (12)                 // connection to PIN
#define               DHTTYPE            DHT22               // DHT 22 (AM2302)

//**************************************General Setup**************************************************
unsigned int wait = 0;                                           // time waited for screen refresh
//**************************************DHT22 Setup**************************************************
//float T = 0;                                                 // temperature
//float H = 0;                                                 // humidity
//int delayMS;
//int THtime = 0;
//**************************************Detector Setup**************************************************
//byte channels[N];                                          // channels array
volatile int Pulse = 0;                                   // duration of current pulse
volatile int Pulse0 = 0;                                  // duration of current pulse
volatile bool Rflag = false;


//**************************************DHT**************************************************
//DHT_Unified dht(DHTPIN, DHTTYPE);
//**************************************Starting functions**************************************************

/*
void dataprint() {
  int i = 0;
  Serial.println(F("C "));
  Serial.println(count);
  while (i < N) {
    Serial.print(channels[i]);
    Serial.print(F(" "));
    i++;
  }
  Serial.println(0);
}

void THPrint(bool chk) {

  if (chk == true) {
    Serial.print(F(" H "));
    Serial.print(H);
    Serial.print(F(" T "));
    Serial.print(T);
  }
  else {
    Serial.print(F(" H "));
    Serial.print(1);
    Serial.print(F(" T "));
    Serial.print(1);
  }
  Serial.println();
}


void THSETUP(void) {
  //Serial.println(F("DHT start"));
  dht.begin();
  delay(2000);
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  //Serial.print  (F("Sensor:       ")); Serial.println(sensor.name);
  //Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

void THRead(void) {
  // Delay between measurements.
  if ((millis() - THtime) > delayMS ) {
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    T = event.temperature;
    dht.humidity().getEvent(&event);
    H = event.relative_humidity;
    if (isnan(T) || isnan(H)) {
      T = 0;
      H = 0;
      THPrint(false);
    }
    else {
      THPrint(true);
    }
    THtime = millis();
  }

}
*/
void DetectorStart(void) {
  analogReference(EXTERNAL);
  //setting up pin
  // set prescale to 16
  sbi(ADCSRA, ADPS2) ;
  cbi(ADCSRA, ADPS1) ;
  cbi(ADCSRA, ADPS0) ;

  pinMode(PulsePin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PulsePin), PulseGet, LOW);
  /*
  int i = 0;
  while (i < N) {
    channels[i] = 0;
    i++;
  }
  */
}

void PulseGet() {
  while (true) {
    Pulse0 = Pulse;
    Pulse = analogRead(ADCPin);
    if (Pulse < Pulse0) {
      Serial.println(Pulse0);
      Rflag = true;
      break;
    }
  }
  Pulse = 0;
}


void DetectorUpdate() {
  if (Rflag == true) {
    Rflag = false;
    //if (Pulse > PulseMin && Pulse < PulseMax) {
      //Pulse = map(Pulse, 0, 1023, 0, N);
      //channels[Pulse] = channels[Pulse] + 1;
      //Serial.println(Pulse0);
    //}
  }
}

void setup(void) {
  //Start Serial comunication
  Serial.begin(115200);
  // DHT sensor reading
  //THSETUP();
  //Start detector
  DetectorStart();
}

void loop(void) {
  DetectorUpdate();
  /*
  if (millis() - wait >= refresh) {
    THRead();
    wait = millis();
    dataprint();
  }
  */
}








