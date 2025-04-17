#include <Wire.h>
#include "../../lib/SCD30/src/SparkFun_SCD30_Arduino_Library.h"
#include "../../lib/Bosch-BME68x/src/bme68xLibrary.h"
#include "../../lib/vl6180x/src/vl6180x.h"
#include "Particle.h"

#define VL6180X_ADDRESS 0x29 // Default I2C address for VL6180x
#define BME_ADDRESS 0x77 // Default I2C address for BME68x
#define BLYNK_AUTH_TOKEN "dz1SzlRRpOFotqyS5zmmKiDzg0uI62vL"


PRODUCT_ID(1);
PRODUCT_VERSION(4);

VL6180x vl6180(VL6180X_ADDRESS);
SCD30 scd30;
Bme68x bme;

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// Time variables
const unsigned long interval = 50000; // Interval in milliseconds - 10 delays in total
unsigned long previousMillis = interval;

// Structure to hold sensor data
struct SensorData {
  // VL6180
  bool vl6180Available;
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
    bme.setOpMode(BME68X_FORCED_MODE);
  }
}

// Function to print sensor data to Serial Monitor
void printData(const SensorData &data) {
  // VL6180
  if(data.vl6180Available){
    Serial.print("VL6180 Ambient: ");
    Serial.print(data.vl6180Ambient);
    Serial.println(" lux");
    
    Serial.print("VL6180 Range: ");
    Serial.print(static_cast<float>(data.vl6180Range));
    Serial.println(" mm");
  } else {
    Serial.println("VL6180 Data not available");
  }

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
    if (data.vl6180Available) {
      // VL6180 data
      writer.name("vl6180Ambient").value(data.vl6180Ambient);
      writer.name("vl6180Range").value(data.vl6180Range);
    }
    else {
      writer.name("vl6180Data").value("Not available");
    }
    
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

    writer.name("BLYNK_AUTH_TOKEN").value(BLYNK_AUTH_TOKEN);

  writer.endObject();

  Particle.publish("measurement", writer.buffer());
}

void publishData(const SensorData &data) {

  Particle.publish("BLYNK_AUTH_TOKEN", BLYNK_AUTH_TOKEN, PRIVATE);
  delay(1000);
  Particle.publish("meas_vl6180Ambient", String(data.vl6180Ambient, 2), PRIVATE);
  delay(1000);
  Particle.publish("meas_vl6180Range", String(data.vl6180Range), PRIVATE);
  
  if (data.scd30Available) {
    delay(1000);
    Particle.publish("meas_scd30CO2", String(data.scd30CO2,2), PRIVATE);
    delay(1000);
    Particle.publish("meas_scd30Temp", String(data.scd30Temp,2), PRIVATE);
    delay(1000);
    Particle.publish("meas_scd30Hum", String(data.scd30Hum,2), PRIVATE);
  }

  if (data.bmeAvailable) {
    delay(1000);
    Particle.publish("meas_bme68xTemp", String(data.bmeTemp,2), PRIVATE);
    delay(1000);
    Particle.publish("meas_bme68xHum", String(data.bmeHum,2), PRIVATE);
    delay(1000);
    Particle.publish("meas_bme68xPress", String(data.bmePressure,2), PRIVATE);
    delay(1000);
    Particle.publish("bme68xGasResistance", String(data.bmeGasResistance, 2), PRIVATE);
    delay(1000);
  }
}

void init_sensors() {

  // Initialize VL6180
  if(vl6180.VL6180xInit() == 0){
    Serial.println("VL6180 Initialized");
    Log.info("VL6180 Initialized");
    sensorData.vl6180Available = true;

    vl6180.VL6180xDefautSettings();
    Serial.println("VL6180 Default Settings Applied");
    Log.info("VL6180 Default Settings Applied");
  }
  else{
    sensorData.vl6180Available = false;
    Serial.println("FAILED TO INITALIZE VL6180"); //Initialize device and check for errors
    Log.error("VL6180 Initialization failed");
  }

  // Initialize SCD30
  if (scd30.begin())
  {
    sensorData.scd30Available = true;
    Serial.println("SCD30 Initialized");
    Log.info("SCD30 Initialized");
  }
  else
  {
    sensorData.scd30Available = false;
    Serial.println("Failed to detect and initialize SCD30!");
    Log.error("SCD30 Initialization failed");
  }


  // Initialize BME68x
  bme.begin(BME_ADDRESS, Wire); // Initialize BME68x with I2C address and Wire object
  if(bme.checkStatus())
	{
		if (bme.checkStatus() == BME68X_ERROR)
		{
			Serial.println("Sensor error:" + bme.statusString());
      Log.error("BME68x Initialization failed:" + bme.statusString());
			return;
		}
		else if (bme.checkStatus() == BME68X_WARNING)
		{
			Serial.println("Sensor Warning:" + bme.statusString());
      Log.warn("BME68x Warning:" + bme.statusString());
		}
	}
  else{
    /* Set the default configuration for temperature, pressure and humidity */
    bme.setTPH();
    /* Set the heater configuration to 300 deg C for 100ms for Forced mode */
    bme.setHeaterProf(300, 100);
    bme.setOpMode(BME68X_FORCED_MODE);
    sensorData.bmeAvailable = true;
    Serial.println("BME68x Initialized");
    Log.info("BME68x Initialized");
  }
}

void setup() {

  Serial.begin(115200);
  Wire.begin();
  delay(500);
  Log.info("Starting sensor setup...");
  init_sensors();
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
    publishDataJSON(sensorData);
    Log.info("Running loop...");
  }
}
