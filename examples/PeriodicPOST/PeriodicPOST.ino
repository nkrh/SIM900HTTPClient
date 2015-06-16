/*
  Periodic POST

 This sketch demonstrates the SIM900HTTPClient library.

 Before use this example make sure you have configured the RX & TX pin
 and the APN stuff at SIM900HTTPClient.h and SIM900HTTPClient.cpp then
 change the setURL parameter to your URL

 Circuit:
 * Arduino UNO R3
 * SIM900 GSM Shield

 created 1 Mar 2015
 by Nico Kurniawan

 http://github.com/nkrh
 */

#include <SoftwareSerial.h>
#include "SIM900HTTPClient.h"

SIM900HTTPClient HTTPClient;
    
unsigned long int lastSend = 0;

void setup() {

	Serial.negin(9600);
    
    // Wait until the SIM900 module ready
    while(!HTTPClient.begin());
    
    // Open the bearer
    HTTPClient.connect();

    // Init HTTP service
    HTTPClient.init();

    // Set the URL target
    HTTPClient.setURL("yourdomain.com/save_sensor_data.php");

    // Set the Content-Type header to application to application/x-www-form-urlencoded
    HTTPClient.setPost();
    
    // Save the URL and Content-Type settings into module's NVRAM
    HTTPClient.save();
}

void loop() {
    

    // Check if the last send is 30 seconds ago
    unsigned long int nowMillis = millis();
    if(nowMillis - lastSend < 30000)
        return;    


    Serial.println("SENDING....");

    // If the service is disconnected, try to reconnect
    if(!HTTPClient.connected()) {
        Serial.println("RECONNECTING");
        HTTPClient.connect();
        HTTPClient.init();
        return;
    }
    
    // This is the post data, you can access this at variabel $_POST['d']
    char request[8];
    memset(request, 0, 8);
    strcat(request, "d=");
    strcat(request, readSensor());

    // Response length
    int responseLength = 200;

    // Store the response at this variable, 1 more byte to add \0 at the end
    char response[responseLength + 1];

    // Excute the POST request
    int result = HTTPClient.post(request, response, responseLength);

    switch(result) {
        case SIM900_RESPONSE_FINISHED: {

            Serial.println("RESPONSE:");
            Serial.println(response);

            // Clear the response
            memset(response, 0, responseLength);

            // Be ready to send 30 secs later
            lastSend = nowMillis;
            break;
        }
        case SIM900_RESPONSE_TIMEOUT: {

            // Maybe the HTTP service dead? try to re-init
            HTTPClient.init();
            Serial.println("TIMEOUT");
            break;
        }
        case SIM900_RESPONSE_ERROR: {

            // Unknown error....
            Serial.println("ERROR");
            break;
        }

    }
}

const char * readSensor() {
    return String(analogRead(A0)).c_str();
}