import Particle from 'particle:core';
import { getUnixTime } from 'vendor:date-fns';

export default async function convertTemperatureToFahrenheit({ event }) {
  let data;

  // Log the incoming event data for troubleshooting
  console.log("Incoming event data:", event);

  // Ensure eventData is defined and try to parse it
  if (!event.eventData) {
    console.error("eventData is undefined or missing.");
    return
  }

  try {
    // Parse the incoming event data (assuming it is Base64-encoded JSON)
    data = JSON.parse(event.eventData);
  } catch (err) {
    console.error("Invalid JSON format", event.eventData);
    throw err;
  }

  // Check we have two fields, temperatureInC and humidity
  if (!data.temperatureInC || !data.humidity) {
    console.error("Missing required fields in eventData", data);
    return
  }

  // Convert the temperature from Celsius to Fahrenheit
  const temperatureInF = (data.temperatureInC * 9/5) + 32;

  // Log the converted temperature for troubleshooting
  console.log("Converted temperature:", temperatureInF);

  // Store in ledger in Fahrenheit
  const envDataLedger = Particle.ledger("env-data", { deviceId: event.deviceId });
  let { data: tsLedger } = envDataLedger.get();

  // Make sure we have an object to write to
  // Note: Ledger instances that don't exist yet return a valid object and
  // upon running set() they will be created.
  tsLedger.log = tsLedger.log || []

  const convertedEnvReading = JSON.stringify({
    temperatureInF,
    humidity: data.humidity
  });

  // Build and add a new entry
  let entry = {
      ts: getUnixTime(new Date()),    // timestamp
      publish: convertedEnvReading || ""  // Default empty string just in case
  };
  tsLedger.log.push(entry);

  // Only keep the last 1 hour of entries as that is all we ever need
  // at 10 seconds per entry, that is 360 entries
  while (tsLedger.log.length > 360) {
    let dropped = tsLedger.log.shift();
    console.log(`Ledger oversize. Drop entry from ${dropped.ts}`);
  }

  // Write back to the Cloud Ledger
  envDataLedger.set(tsLedger);

  // Pull the setpoint from the configuration document store
  const configLedger = Particle.ledger("configuration");
  const configData = configLedger.get();
  const temperatureSetPointInF = configData.data.temperatureSetPointInF;

  // If the setpoint is breached, we need to trigger the alarm
  // This goes into an SMS so has to fit in 200 chars
  if (temperatureInF > temperatureSetPointInF) {
    console.log("Temperature exceeded the setpoint");
    const payload = JSON.stringify({
      message: "Temperature exceeded the setpoint for device" + event.deviceId + " at " + new Date().toISOString() + " with temperature " + temperatureInF
    });
    Particle.publish("alarm", payload, { productId: 33703 });
  } else {
    console.log("Temperature is within the setpoint");
  }
}
