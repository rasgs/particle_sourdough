#include <Wire.h>
#include "../../lib/SCD30/src/SparkFun_SCD30_Arduino_Library.h"
#include "../../lib/Bosch-BME68x/src/bme68xLibrary.h"
#include "../../lib/vl6180x/src/vl6180x.h"
#include "Particle.h"

#define VL6180X_ADDRESS 0x29 // Default I2C address for VL6180x
#define BME_ADDRESS 0x76 // Default I2C address for BME68x
#define BLYNK_AUTH_TOKEN "dz1SzlRRpOFotqyS5zmmKiDzg0uI62vL"

PRODUCT_ID(1);
PRODUCT_VERSION(3);

VL6180x vl6180(VL6180X_ADDRESS);
SCD30 scd30;
Bme68x bme;

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// Time variables
unsigned long previousMillis = 0;
const unsigned long interval = 60000; // Interval in milliseconds

// Structure to hold sensor data
struct SensorData {
  // VL6180
  int vl6180Range;
  float vl6180Ambient;
  // SCD30
  bool scd30Available;
  float scd30CO2;
  float scd30Temp;
  float scd30Hum;
  // BME68x
  bool bmeAvailable;
  float bmeTemp;
  float bmeHum;
  float bmePressure;
  float bmeGasResistance;
};

SensorData sensorData;

// Function to poll all sensors and store results in sensorData
void readSensors(SensorData &data) {
  // VL6180
  data.vl6180Range = vl6180.getDistance();
  data.vl6180Ambient = vl6180.getAmbientLight(GAIN_1);

  // SCD30
  data.scd30Available = scd30.dataAvailable();
  if (data.scd30Available) {
    data.scd30CO2  = scd30.getCO2();
    data.scd30Temp = scd30.getTemperature();
    data.scd30Hum  = scd30.getHumidity();
  }

  // BME68x
  bme68xData bmeData;
  data.bmeAvailable = bme.fetchData();
  if (data.bmeAvailable) {
    bme.getData(bmeData);
    data.bmeTemp         = bmeData.temperature;
    data.bmeHum          = bmeData.humidity;
    data.bmePressure     = bmeData.pressure / 100.0; // Convert Pa to hPa if needed
    data.bmeGasResistance = bmeData.gas_resistance;
  }
}

// Function to print sensor data to Serial Monitor
void printData(const SensorData &data) {
  // VL6180
  Serial.print("VL6180 Ambient: ");
  Serial.print(data.vl6180Ambient);
  Serial.println(" lux");
  
  Serial.print("VL6180 Range: ");
  Serial.print(static_cast<float>(data.vl6180Range));
  Serial.println(" mm");

  // SCD30
  if (data.scd30Available) {
    Serial.print("SCD30 CO2(ppm): ");
    Serial.print(data.scd30CO2);
    Serial.print(" Temperature(C): ");
    Serial.print(data.scd30Temp);
    Serial.print(" Humidity(%): ");
    Serial.println(data.scd30Hum);
  } else {
    Serial.println("SCD30 Data not available");
  }

  // BME68x
  if (data.bmeAvailable) {
    Serial.print("BME68x Temperature(C): ");
    Serial.print(data.bmeTemp);
    Serial.print(" Humidity(%): ");
    Serial.print(data.bmeHum);
    Serial.print(" Pressure(hPa): ");
    Serial.print(data.bmePressure);
    Serial.print(" Gas Resistance(Ohms): ");
    Serial.println(data.bmeGasResistance);
  } else {
    Serial.println("BME68x Data not available");
  }
  Serial.println();
}

// Function to build JSON data and publish via Particle.publish
void publishDataJSON(const SensorData &data) {
  char jsonBuffer[512] = {0};
  JSONBufferWriter writer(jsonBuffer, sizeof(jsonBuffer));
  
  writer.beginObject();
    // VL6180 data
    writer.name("vl6180Ambient").value(data.vl6180Ambient);
    writer.name("vl6180Range").value(data.vl6180Range);
    
    // SCD30 data
    if (data.scd30Available) {
      writer.name("scd30CO2").value(data.scd30CO2);
      writer.name("scd30Temperature").value(data.scd30Temp);
      writer.name("scd30Humidity").value(data.scd30Hum);
    } else {
      writer.name("scd30Data").value("Not available");
    }
    
    // BME68x data
    if (data.bmeAvailable) {
      writer.name("bme68xTemperature").value(data.bmeTemp);
      writer.name("bme68xHumidity").value(data.bmeHum);
      writer.name("bme68xPressure").value(data.bmePressure);
      writer.name("bme68xGasResistance").value(data.bmeGasResistance);
    } else {
      writer.name("bme68xData").value("Not available");
    }
  writer.endObject();

  Particle.publish("measurement", writer.buffer());
}

void publishData(const SensorData &data) {
  char eventData[20] = {0};

  Particle.publish("BLYNK_AUTH_TOKEN", String(BLYNK_AUTH_TOKEN), PRIVATE);


  sprintf(eventData, "%.2f", data.vl6180Ambient);
  Particle.publish("meas_vl6180Ambient", eventData, PRIVATE);
  sprintf(eventData, "%d", data.vl6180Range);
  Particle.publish("meas_vl6180Range", eventData, PRIVATE);
  
  if (data.scd30Available) {
    sprintf(eventData, "%.2f", data.scd30CO2);
    Particle.publish("meas_scd30CO2", eventData, PRIVATE);
    sprintf(eventData, "%.2f", data.scd30Temp);
    Particle.publish("meas_scd30Temperature", eventData, PRIVATE);
    sprintf(eventData, "%.2f", data.scd30Hum);
    Particle.publish("meas_scd30Humidity", eventData, PRIVATE);
  }

  if (data.bmeAvailable) {
    sprintf(eventData, "%.2f", data.bmeTemp);
    Particle.publish("meas_bme68xTemperature", eventData, PRIVATE);
    sprintf(eventData, "%.2f", data.bmeHum);
    Particle.publish("meas_bme68xHumidity", eventData, PRIVATE);
    sprintf(eventData, "%.2f", data.bmePressure);
    Particle.publish("meas_bme68xPressure", eventData, PRIVATE);
    // Particle.publish("bme68xGasResistance", String(data.bmeGasResistance), PRIVATE);
  }
}

void setup() {

  Serial.begin(115200);
  Wire.begin();

  // Initialize VL6180
  if(vl6180.VL6180xInit() != 0){
    Serial.println("FAILED TO INITALIZE VL6180"); //Initialize device and check for errors
  }; 
  vl6180.VL6180xDefautSettings();


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

    // Poll sensors only once
    readSensors(sensorData);

    // Print results locally
    printData(sensorData);

    // Publish sensor readings to Particle Cloud
    publishData(sensorData);
  }
}
