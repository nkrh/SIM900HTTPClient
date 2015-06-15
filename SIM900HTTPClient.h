/*
  SIM900HTTPClient.h - Library for using SIM900 HTTP service.
  Created by Nico Kurniawan, Maret 1, 2015.
  Released into the public domain.

  http://github.com/nkrh
*/

#ifndef SIM900_H
#define SIM900_H
#define SIM900HTTPCLIENT_DEBUG

#include <Arduino.h>
#include <SoftwareSerial.h>

#define RX_PIN 8
#define TX_PIN 7

#define AT_RESPONSE_BUFFER_SIZE 200
#define DEFAULT_TIMEOUT 10000
#define BAUD_RATE 2400
#define POST "1"
#define GET "0"

enum SIM900_RESPONSE_STATUS {
	SIM900_RESPONSE_WAITING, 
	SIM900_RESPONSE_FINISHED, 
	SIM900_RESPONSE_TIMEOUT, 
	SIM900_RESPONSE_BUFFER_FULL, 
	SIM900_RESPONSE_STARTING,
	SIM900_RESPONSE_ERROR
};

class SIM900HTTPClient {
	public:
		SIM900HTTPClient();
		bool begin();
		bool sendATCommandAndExpects(
			const char * ATCommand, 
			const char * expectedATResponse, 
			const unsigned int timeOut = DEFAULT_TIMEOUT, 
			const int responseCount = 1);
		SIM900_RESPONSE_STATUS post(const char * url, const char * parameters, char * response, int responseLength);
		SIM900_RESPONSE_STATUS post(const char * parameters, char * response, int responseLength);
		SIM900_RESPONSE_STATUS get(const char *url, char * response, int responseLength);
		SIM900_RESPONSE_STATUS get(const char *url);
		void waitForResponse(const unsigned int timeOut = DEFAULT_TIMEOUT, const byte responseCount = 1);
		bool waitForExpectedResponse(
			const char * expectedResponse, 
			const unsigned int timeOut = DEFAULT_TIMEOUT, 
			const byte responseCount = 1);
		bool connect();
		bool connected();
		bool init();
		bool save();
		bool setURL(const char * url);
		void setPost();
	protected:
		char ATResponseBuffer[AT_RESPONSE_BUFFER_SIZE];
		SoftwareSerial GSM;
		bool GSMBegin();
	private:
		unsigned int timeOut;
		unsigned long lastReceive;
		unsigned int ATResponseBufferLength;
		bool usePost;
		SIM900_RESPONSE_STATUS responseReaderStatus;
		bool HTTPTerm();
		bool HTTPInit();
		bool HTTPParam(const char * paramTag, const char * paramValue);
		bool HTTPData(const char * data, const unsigned int timeOut);
		bool HTTPRead(const int byteStart, int byteSize, char * result);
		bool HTTPAction(const char * method);
		void getResponseBody(char * response, int responseLength);
		void readResponse(const byte crlfCount = 2);
		void startReadResponse(const unsigned int timeOut = DEFAULT_TIMEOUT);
};

#endif