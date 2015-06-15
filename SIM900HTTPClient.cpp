#include "SIM900HTTPClient.h"

SIM900HTTPClient::SIM900HTTPClient() : GSM(TX_PIN, RX_PIN) {	
}

bool SIM900HTTPClient::begin() {
	return GSMBegin();
}

bool SIM900HTTPClient::GSMBegin() {

	GSM.begin(BAUD_RATE);
	for(int i = 0; i < 5; ++i) {	
		if(sendATCommandAndExpects("AT", "OK")) {
            sendATCommandAndExpects("ATE0", "OK");	
            HTTPParam("CID", "1");
            HTTPParam("UA", "Arduino");
            HTTPParam("TIMEOUT", "60");
            save();
			return true;
        }
	}
	return false;
}

bool SIM900HTTPClient::sendATCommandAndExpects(
	const char * ATCommand, 
	const char * expectedResponse, 
	const unsigned int timeOut,
	const int responseCount) {	

	GSM.println(ATCommand);

	return waitForExpectedResponse(expectedResponse, timeOut, responseCount);
}

void SIM900HTTPClient::readResponse(const byte crlfCount) {

	static char _crlfBuf[2];
	static char _crlfFound;
	static bool bufferFull;

	// Check for timeout
	if(millis() - lastReceive >= timeOut) {
		responseReaderStatus = SIM900_RESPONSE_TIMEOUT;
		return;
	}

	// Check if rx just started
	if(responseReaderStatus == SIM900_RESPONSE_STARTING) {
		bufferFull = true;
		_crlfFound = 0;
		memset(_crlfBuf, 0, 2);
		responseReaderStatus = SIM900_RESPONSE_WAITING;
	}

	int availableBytes = GSM.available();

	while(availableBytes) {
		char nextChar = (char) GSM.read();
		--availableBytes;		
		
		if(ATResponseBufferLength < AT_RESPONSE_BUFFER_SIZE) {
			ATResponseBuffer[ATResponseBufferLength++] = nextChar;
			bufferFull = true;
		}

		// Looking for CRLF
		memmove(_crlfBuf, _crlfBuf + 1, 1);
		_crlfBuf[1] = nextChar;
		
		if(!strncmp(_crlfBuf, "\r\n", 2)) {
			if(++_crlfFound == crlfCount) {
				responseReaderStatus = SIM900_RESPONSE_FINISHED;
				return;
			}
		}

		
	} 
}

void SIM900HTTPClient::startReadResponse(unsigned int timeOut) {
	GSM.flush();
	ATResponseBufferLength = 0;
	lastReceive = millis();
	responseReaderStatus = SIM900_RESPONSE_STARTING;
	SIM900HTTPClient::timeOut = timeOut;
	memset(ATResponseBuffer, 0, AT_RESPONSE_BUFFER_SIZE);
}

void SIM900HTTPClient::waitForResponse(const unsigned int timeOut, const byte responseCount) {
	
	const byte crlfCount = responseCount * 2;

	startReadResponse(timeOut);
	do {
 		readResponse(crlfCount);
	} while(responseReaderStatus == SIM900_RESPONSE_WAITING);

	#ifdef SIM900HTTPCLIENT_DEBUG
	Serial.print(F("\nSTS: "));
	Serial.println(responseReaderStatus);
	Serial.println(F("RES: "));
	Serial.println(ATResponseBuffer);
	#endif
}

bool SIM900HTTPClient::waitForExpectedResponse(
	const char * expectedResponse, 
	const unsigned int timeOut, 
	const byte responseCount) {

	waitForResponse(timeOut, responseCount);

	#ifdef SIM900HTTPCLIENT_DEBUG
	Serial.print(F("EXP: "));
	Serial.println(expectedResponse);
	#endif

	if((responseReaderStatus != SIM900_RESPONSE_TIMEOUT) && (strstr(ATResponseBuffer, expectedResponse) != NULL)) {
		return true;
	} else {
		return false;
	}
}

bool SIM900HTTPClient::HTTPAction(const char * method) {
	
	GSM.print(F("AT+HTTPACTION="));	
	GSM.println(method);

	#ifdef SIM900HTTPCLIENT_DEBUG
	Serial.println("\nHTTPACTION");
	#endif

	return waitForExpectedResponse("OK");
}

bool SIM900HTTPClient::HTTPInit() {

	GSM.println(F("AT+HTTPINIT"));

	#ifdef SIM900HTTPCLIENT_DEBUG
	Serial.println("\nHTTPINIT");
	#endif

	return waitForExpectedResponse("OK");
}

bool SIM900HTTPClient::HTTPParam(const char * paramTag, const char * paramValue) {
	
	GSM.print(F("AT+HTTPPARA=\""));
	GSM.print(paramTag);
	GSM.print(F("\",\""));
	GSM.print(paramValue);
	GSM.println(("\""));

	#ifdef SIM900HTTPCLIENT_DEBUG
	Serial.println("\nHTTPPARAM");
	#endif

	return waitForExpectedResponse("OK");
}

bool SIM900HTTPClient::HTTPTerm() {

	GSM.println(F("AT+HTTPTERM"));

	#ifdef SIM900HTTPCLIENT_DEBUG
	Serial.println("\nHTTPTERM");
	#endif

	return waitForExpectedResponse("OK");
}

bool SIM900HTTPClient::HTTPRead(const int byteStart, int byteSize, char * result) {

	GSM.print(F("AT+HTTPREAD="));
	GSM.print(byteStart);
	GSM.print(F(","));
	GSM.println(byteSize);

	#ifdef SIM900HTTPCLIENT_DEBUG
	Serial.println("\nHTTPREAD");
	#endif

	if(waitForExpectedResponse("+HTTPREAD", 60000, 2)) {
		getResponseBody(ATResponseBuffer, byteSize);
		strncpy(result, ATResponseBuffer, byteSize);
		return true;
	} else {
		return false;
	}
}

bool SIM900HTTPClient::HTTPData(const char * data, const unsigned int timeOut) {

	int dataLength = strlen(data);
	GSM.print(F("AT+HTTPDATA="));
	GSM.print(dataLength);
	GSM.print(F(","));
	GSM.println(timeOut);

	#ifdef SIM900HTTPCLIENT_DEBUG
	Serial.print("\nHTTPDATA: ");
	Serial.println(data);
	#endif

	if(waitForExpectedResponse("DOWNLOAD")) {
		return sendATCommandAndExpects(data, "OK", timeOut);
	} else {
		return false;
	}
}

SIM900_RESPONSE_STATUS SIM900HTTPClient::get(const char * url, char * response, int responseLength) {

	if(get(url) == SIM900_RESPONSE_TIMEOUT)
		return SIM900_RESPONSE_TIMEOUT;

	HTTPRead(0, responseLength, response);	

	return SIM900_RESPONSE_FINISHED;
}

SIM900_RESPONSE_STATUS SIM900HTTPClient::get(const char * url) {

	HTTPParam("URL", url);
	HTTPAction(GET);

	waitForResponse(60000);	
	return responseReaderStatus;
}

SIM900_RESPONSE_STATUS SIM900HTTPClient::post(const char * url, const char * parameters, char * response, int responseLength) {

	HTTPParam("URL", url);
	
	if(!usePost)
		HTTPParam("CONTENT","application/x-www-form-urlencoded");

	return post(parameters, response, responseLength);
}

SIM900_RESPONSE_STATUS SIM900HTTPClient::post(const char * parameters, char * response, int responseLength) {

	if(!(HTTPData(parameters, 60000) && HTTPAction(POST)))
		return SIM900_RESPONSE_TIMEOUT;

	bool status200 = waitForExpectedResponse(",200,", 60000);

	if(responseReaderStatus == SIM900_RESPONSE_TIMEOUT)
		return SIM900_RESPONSE_TIMEOUT;

	if(!status200)
		return SIM900_RESPONSE_ERROR;

	HTTPRead(0, responseLength, response);

	return SIM900_RESPONSE_FINISHED;
}

void SIM900HTTPClient::getResponseBody(char * response, int responseLength) {

    byte i = 13;
    char _buf[5];
    while(response[i] != '\r') {
        ++i;
    }

    // Get returned response length
    strncpy(_buf, response + 13, (i - 13));
   	int _t = atoi(_buf);
   	responseLength = _t;

   	// Remove +HTTPREAD: xxx and OK string
    i += 2;
    byte tmp = strlen(response) - i;
    memcpy(response, response + i, tmp);

    if(!strncmp(response + tmp - 6, "\r\nOK\r\n", 6))
    	memset(response + tmp - 6, 0, i + 6);
}

bool SIM900HTTPClient::init() {
	HTTPTerm();
	return HTTPInit();
}

bool SIM900HTTPClient::save() {
	GSM.println(F("AT+HTTPSCONT"));
	return waitForExpectedResponse("OK");
}

bool SIM900HTTPClient::setURL(const char * url) {
	return HTTPParam("URL", url);
}

void SIM900HTTPClient::setPost() {
	usePost = HTTPParam("CONTENT", "application/x-www-form-urlencoded");
}

bool SIM900HTTPClient::connect() {

	GSM.println(F("AT+CREG?"));
	if(!waitForExpectedResponse("+CREG: 0,1"))
		return false;

	GSM.println(F("AT+SAPBR=0,1"));
	waitForExpectedResponse("OK");

	GSM.println(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
	waitForExpectedResponse("OK");

	GSM.println(F("AT+SAPBR=3,1,\"APN\",\"indosatgprs\""));
	waitForExpectedResponse("OK");

	GSM.println(F("AT+SAPBR=3,1,\"USER\",\"\""));
	waitForExpectedResponse("OK");

	GSM.println(F("AT+SAPBR=3,1,\"PWD\",\"\""));
	waitForExpectedResponse("OK");

	GSM.println(F("AT+SAPBR=1,1"));
	return waitForExpectedResponse("OK", 60000);
}

bool SIM900HTTPClient::connected() {
	GSM.println(F("AT+SAPBR=2,1"));
	return waitForExpectedResponse("+SAPBR: 1,1");
}