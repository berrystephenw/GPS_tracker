/*
 * Project Gps-test
 * Description:
 * Author:
 * Date:
 */

// This #include statement was automatically added by the Particle IDE.
//#include <AssetTracker.h>

/* -----------------------------------------------------------
This example shows a lot of different features. As configured here
it will check for a good GPS fix every 10 minutes and publish that data
if there is one. If not, it will save you data by staying quiet. It also
registers 3 Particle.functions for changing whether it publishes,
reading the battery level, and manually requesting a GPS reading.
---------------------------------------------------------------*/

// Getting the library
#include "AssetTracker.h"
#define NUM_SAVE 360    // number of saved positions
// if there is a position every 10 minutes this gives 3600 minutes of coverage or 
// 3600/60 or 60 hours, this is longer than the battery can last

// Set whether you want the device to publish data to the internet by default here.
// 1 will Particle.publish AND Serial.print, 0 will just Serial.print
// Extremely useful for saving data while developing close enough to have a cable plugged in.
// You can also change this remotely using the Particle.function "tmode" defined in setup()
int transmittingData = 1;

// Used to keep track of the last time we published data
long lastPublish = 0;

// How many minutes between publishes? 10+ recommended for long-time continuous publishing!
int delayMinutes = 10;

// Creating an AssetTracker named 't' for us to reference
AssetTracker t = AssetTracker();

// keep track of GPS positions
float lat[NUM_SAVE];
float lon[NUM_SAVE];
int gps_index = 0;

// A FuelGauge named 'fuel' for checking on the battery state
FuelGauge fuel;

// setup() and loop() are both required. setup() runs once when the device starts
// and is used for registering functions and variables and initializing things
void setup() {
    // Sets up all the necessary AssetTracker bits
    t.begin();

    // Enable the GPS module. Defaults to off to save power.
    // Takes 1.5s or so because of delays.
    t.gpsOn();

    // Opens up a Serial port so you can listen over USB
    Serial.begin(9600);

    // These three functions are useful for remote diagnostics. Read more below.
    Particle.function("tmode", transmitMode);
    Particle.function("batt", batteryStatus);
    Particle.function("gps", gpsPublish);
    Particle.function("gps list", gpsPublishList);
    Particle.function("reset log", gpsResetList);
}

// loop() runs continuously
void loop() {
    // You'll need to run this every loop to capture the GPS output
    t.updateGPS();

    if (gps_index >= NUM_SAVE) {
        gps_index = 0;
    }

    // if the current time - the last time we published is greater than your set delay...
    if (millis()-lastPublish > delayMinutes*60*1000) {
        // Remember when we published
        lastPublish = millis();

        //String pubAccel = String::format("%d,%d,%d", t.readX(), t.readY(), t.readZ());
        //Serial.println(pubAccel);
        //Particle.publish("A", pubAccel, 60, PRIVATE);

        // Dumps the full NMEA sentence to serial in case you're curious
        Serial.println(t.preNMEA());

        // GPS requires a "fix" on the satellites to give good data,
        // so we should only publish data if there's a fix
        if (t.gpsFix()) {
            // Only publish if we're in transmittingData mode 1;
            if (transmittingData) {
                // Short publish names save data!
                Particle.publish("G", t.readLatLon(), 60, PRIVATE);
            }
            lat[gps_index] = t.readLat();
            lon[gps_index] = t.readLon();
            gps_index++;
            // but always report the data over serial for local development
            Serial.println(t.readLatLon());
        }
    }
}

// Allows you to remotely change whether a device is publishing to the cloud
// or is only reporting data over Serial. Saves data when using only Serial!
// Change the default at the top of the code.
int transmitMode(String command) {
    transmittingData = atoi(command);
    return 1;
}

// reset the counter for the log
int gps_index_orig;
int gpsResetList(String coemmand){
    gps_index_orig = gps_index;
    gps_index = 0;
    return gps_index_orig;
}
// Actively ask for a GPS reading if you're impatient. Only publishes if there's
// a GPS fix, otherwise returns '0'
int gpsPublish(String command) {
    //Particle.publish("GT", t.readLatLon(), 60, PRIVATE);
    if (t.gpsFix()) {
        Particle.publish("G", t.readLatLon(), 60, PRIVATE);

        // uncomment next line if you want a manual publish to reset delay counter
        // lastPublish = millis();
        return 1;
    } else {
      return 0;
    }
}


// dumps the entire log - can be up to 360 pairs of GPS cordinates 
char output[60];
int gpsPublishList(String command) {
        int i;
        for(i=0;i++;i<gps_index) {
            snprintf(output, 60, "%f, %f", t.readLat(), t.readLon());
            Particle.publish("GL", output, 60, PRIVATE);
        }
        // uncomment next line if you want a manual publish to reset delay counter
        // lastPublish = millis();
        return 1;
}

// Lets you remotely check the battery status by calling the function "batt"
// Triggers a publish with the info (so subscribe or watch the dashboard)
// and also returns a '1' if there's >10% battery left and a '0' if below
int batteryStatus(String command){
    // Publish the battery voltage and percentage of battery remaining
    // if you want to be really efficient, just report one of these
    // the String::format("%f.2") part gives us a string to publish,
    // but with only 2 decimal points to save space
    Particle.publish("B",
          "v:" + String::format("%.2f",fuel.getVCell()) +
          ",c:" + String::format("%.2f",fuel.getSoC()),
          60, PRIVATE
    );
    // if there's more than 10% of the battery left, then return 1
    if (fuel.getSoC()>10){ return 1;}
    // if you're running out of battery, return 0
    else { return 0;}
}
