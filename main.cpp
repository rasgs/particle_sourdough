/* 
 * Particle Blueprint - Environmental Monitor Tutorial
 * https://github.com/particle-iot/blueprint-hello-world
 */

// Include Particle Device OS APIs
#include "Particle.h"

//library for the sensor
#include "DHT22Gen3_RK.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// The two parameters are any available GPIO pins. They will be used as output but the signals aren't
// particularly important for DHT11 and DHT22 sensors. They do need to be valid pins, however.
DHT22Gen3 dht(A4, A5);

// setup() runs once, when the device is first turned on
void setup() {

  // Initialize DHT sensor
  dht.setup();

  Log.info("Setup has finished!");
}

void sampleCallback(DHTSample sample) {
	// The callback is called at loop() (from dht.loop()) so it's safe to do anything you would normally
	// do at loop time (like publish) without having to worry about thread concurrency.

	if (sample.isSuccess()) {
		Log.info("sampleResult=%d tempF=%.1f tempC=%.1f humidity=%.1f tries=%d",
				(int) sample.getSampleResult(), sample.getTempF(), sample.getTempC(), sample.getHumidity(), sample.getTries() );
		Log.info("dewPointF=%.1f dewPointC=%.1f",
				sample.getDewPointF(), sample.getDewPointC() );

      Log.info("Sending the data to the cloud...");
      
      //create a JSON object that contains the temperature and humidity using the JSON helper
      char data[256] = {0};
      JSONBufferWriter writer(data, sizeof(data));
      writer.beginObject();
      writer.name("temperatureInC").value(sample.getTempC());
      writer.name("humidity").value(sample.getHumidity());
      writer.endObject();

      //publish the event to the cloud
      Particle.publish("environmental-data", writer.buffer() );
	}
	else {
		Log.info("sample is not valid sampleResult=%d", (int) sample.getSampleResult());
	}
}


// loop() runs over and over again, as quickly as it can execute.
// This is a very basic programming model (the superloop) - 
// for more advanced threading models, please see the Device OS documentation at docs.particle.io
void loop() {
  // How often to check the temperature in humidity in milliseconds
  const unsigned long CHECK_INTERVAL_IN_MS = 10 * 1000;
  static unsigned long lastCheck = 0;

  dht.loop();

	if (millis() - lastCheck >= CHECK_INTERVAL_IN_MS) {
		dht.getSample(A2, sampleCallback, &DHT22Gen3::sensorTypeDHT11);

    lastCheck = millis();
	}
}
