#include <Wire.h>
#include "../../lib/SCD30/src/SparkFun_SCD30_Arduino_Library.h"
#include "../../lib/Bosch-BME68x/src/bme68xLibrary.h"
#include "../../lib/vl6180x/src/vl6180x.h"
#include "Particle.h"

#define VL6180X_ADDRESS 0x29 // Default I2C address for VL6180x
#define BME_ADDRESS 0x76 // Default I2C address for BME68x

VL6180x vl6180(VL6180X_ADDRESS);
SCD30 scd30;
Bme68x bme;

const unsigned long interval = 5000; // Time interval in milliseconds
unsigned long previousMillis = 0;

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

void setup() {

  Serial.begin(115200);
  Wire.begin();

  // Initialize VL6180
  if(vl6180.VL6180xInit() != 0){
    Serial.println("FAILED TO INITALIZE VL6180"); //Initialize device and check for errors
  }; 
  vl6180.VL6180xDefautSettings();
  delay(1000); // delay 1s

  // Initialize SCD30
  if (!scd30.begin()) {
    Serial.println("Failed to detect and initialize SCD30!");
    while (1);
  }

  // Initialize BME68x
  bme.begin(BME_ADDRESS, Wire); // Initialize BME68x with I2C address and Wire object
	/* Set the default configuration for temperature, pressure and humidity */
	bme.setTPH();
	/* Set the heater configuration to 300 deg C for 100ms for Forced mode */
	bme.setHeaterProf(300, 100);



  Log.info("Setup has finished!");
  
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Read from VL6180
    int range = vl6180.getDistance();
    float ambient = vl6180.getAmbientLight(GAIN_1);
    
    Serial.print("VL6180 Ambient: ");
    Serial.print(ambient);
    Serial.println(" lux");
    
    Serial.print("VL6180 Range: ");
    Serial.print(static_cast<float>(range));
    Serial.println(" mm");

    // Read from SCD30
    if (scd30.dataAvailable()) {
      Serial.print("SCD30 CO2(ppm): ");
      Serial.print(static_cast<float>(scd30.getCO2()));
      Serial.print(" Temperature(C): ");
      Serial.print(scd30.getTemperature());
      Serial.print(" Humidity(%): ");
      Serial.println(scd30.getHumidity());
    } else {
      Serial.println("SCD30 Data not available");
    }

    // Read from BME68x
    bme68xData data;
    if (bme.fetchData())
    {
      bme.getData(data);
      Serial.print("BME68x Temperature(C): ");
      Serial.print(data.temperature);
      Serial.print(" Humidity(%): ");
      Serial.print(data.humidity);
      Serial.print(" Pressure(hPa): ");
      Serial.print(data.pressure / 100.0);
      Serial.print(" Gas Resistance(Ohms): ");
      Serial.println(data.gas_resistance);
    } else {
      Serial.println("BME68x Data not available");
    }

    Serial.println();
  }
}
