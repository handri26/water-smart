#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include <Temboo.h>
#include <TembooSession.h>
#include <TembooNetworkClient.h>
#include "TembooAccount.h"

#define HIGH 1
#define LOW 0
#define INPUT 1
#define OUTPUT 0

// SocketConnection is a struct containing data needed
// for communicating with the network interface
SocketConnection theSocket;

// There should only be one TembooSession per device. It represents
// the connection to Temboo
TembooSession theSession;

// Limit the number of times the Choreo is to be run. This avoids
// inadvertently using up your monthly Choreo limit
int currentRun = 0;
const int MAX_RUNS = 1000;

// Defines a time in seconds for how long the Choreo
// has to complete before timing
const int CHOREO_TIMEOUT = 100;

int inputPin = 0;
int outputPin = 135;

bool digitalPinMode(int pin, int dir){
  FILE * fd;
  char fName[128];
  // Exporting the pin to be used
  if(( fd = fopen("/sys/class/gpio/export", "w")) == NULL) {
    printf("Error: unable to export pin\n");
    return false;
  }
  fprintf(fd, "%d\n", pin);
  fclose(fd);

  // Setting direction of the pin
  sprintf(fName, "/sys/class/gpio/gpio%d/direction", pin);
  if((fd = fopen(fName, "w")) == NULL) {
    printf("Error: can't open pin direction\n");
    return false;
  }
  if(dir == OUTPUT) {
    fprintf(fd, "out\n");
  } else {
    fprintf(fd, "in\n");
  }
  fclose(fd);

  return true;
}

int analogRead(int pin) {
  FILE * fd;
  char fName[64];
  char val[8];

  // open value file
  sprintf(fName, "/sys/devices/126c0000.adc/iio:device0/in_voltage%d_raw", pin);
  if((fd = fopen(fName, "r")) == NULL) {
    printf("Error: can't open analog voltage value\n");
    return 0;
  }
  fgets(val, 8, fd);
  fclose(fd);

  return atoi(val);
}

int digitalRead(int pin) {
  FILE * fd;
  char fName[128];
  char val[2];

  // Open pin value file
  sprintf(fName, "/sys/class/gpio/gpio%d/value", pin);
  if((fd = fopen(fName, "r")) == NULL) {
    printf("Error: can't open pin value\n");
    return false;
  }
  fgets(val, 2, fd);
  fclose(fd);

  return atoi(val);
}

bool digitalWrite(int pin, int val) {
  FILE * fd;
  char fName[128];

  // Open pin value file
  sprintf(fName, "/sys/class/gpio/gpio%d/value", pin);
  if((fd = fopen(fName, "w")) == NULL) {
    printf("Error: can't open pin value\n");
    return false;
  }
  if(val == HIGH) {
    fprintf(fd, "1\n");
  } else {
    fprintf(fd, "0\n");
  }
  fclose(fd);

  return true;
}

TembooError setup() {
  // We have to initialize the TembooSession struct exactly once.
  TembooError rc = TEMBOO_SUCCESS;

#ifndef USE_SSL
  rc = initTembooSession(
            &theSession,
            TEMBOO_ACCOUNT,
            TEMBOO_APP_KEY_NAME,
            TEMBOO_APP_KEY,
            &theSocket);
#else
  printf("Enabling TLS...\n");
  rc = initTembooSessionSSL(
            &theSession,
            TEMBOO_ACCOUNT,
            TEMBOO_APP_KEY_NAME,
            TEMBOO_APP_KEY,
            &theSocket,
            "/opt/iothub/artik/temboo/temboo_artik_library/lib/temboo.pem",
            NULL);
#endif

  return rc;
}

// Call a Temboo Choreo
void runPost(TembooSession* session, float temperature) {

  printf("\nRunning Post\n");

  // Initialize Choreo data structure
  TembooChoreo choreo;
  const char choreoName[] = "/Library/Utilities/HTTP/Post";
  initChoreo(&choreo, choreoName);

  // Set Choreo inputs
  ChoreoInput RequestHeadersIn;
  RequestHeadersIn.name = "RequestHeaders";
  RequestHeadersIn.value = "{\n   \"Content-Type\": \"application/json\",\n   \"Authorization\": \"Bearer f7ab542e5e554a26a6ec3f354f109655\"\n}";
  addChoreoInput(&choreo, &RequestHeadersIn);

  ChoreoInput URLIn;
  URLIn.name = "URL";
  URLIn.value = "https://api.samsungsami.io/v1.1/messages";
  addChoreoInput(&choreo, &URLIn);

  ChoreoInput DebugIn;
  DebugIn.name = "Debug";
  DebugIn.value = "true";
  addChoreoInput(&choreo, &DebugIn);

  ChoreoInput PasswordIn;
  PasswordIn.name = "Password";
  PasswordIn.value = "handri24";
  addChoreoInput(&choreo, &PasswordIn);

  ChoreoInput RequestBodyIn;
  RequestBodyIn.name = "RequestBody";
  char requestBody[265]="";
  sprintf(requestBody, "{\n   \"sdid\": \"0a81f4b7f7f84572bceecbc61e391061\",\n   \"type\": \"message\",\n   \"data\": {\n      \"temperature\": %f \n  }\n}", temperature);
  RequestBodyIn.value = requestBody;
  printf("Request Body is %s", RequestBodyIn.value);
  addChoreoInput(&choreo, &RequestBodyIn);

  int returnCode = runChoreo(&choreo, session, CHOREO_TIMEOUT);
  if (returnCode != 0) {
    printf("runChoreo failed.  Error: %d\n", returnCode);
  }

  // Print the response received from Temboo
  while (tembooClientAvailable(session->connectionData)) {
    printf("%c", readChoreoResult(&choreo, session));
  }

  // When we're done, close the connection
  tembooClientStop(session->connectionData);
}

int setup2(){
   if (!digitalPinMode(outputPin, OUTPUT))
      return -1;
   return 0;
}

int main(void) {
  if(setup() != TEMBOO_SUCCESS) {
    return EXIT_FAILURE;
  }
  while(currentRun < MAX_RUNS){
    int sensorVal = analogRead(inputPin);
    float voltage = sensorVal;
    voltage /= 1024.0;
    float temperatureC = (voltage - 0.5) * 100;
    float temperature = (temperatureC * 9.0 / 5.0) + 32.0;
    printf("current temperature is %f\n", temperature);
    currentRun++;
    runPost(&theSession, temperature);
    sleep(10);
    if(temperature > 300){
    digitalWrite(outputPin, HIGH);
    sleep(20);
    digitalWrite(outputPin, LOW);
    sleep(20);
    }
    else if(temperature < 300){
    digitalWrite(outputPin, HIGH);
    sleep(3);
    digitalWrite(outputPin, LOW);
    sleep(3);
    }
    else {
    digitalWrite(outputPin, LOW);
    sleep(1);
    }
  }
//  if (setup2() == -1)
//  {
//   exit(1);
//  }
//  while(1){
//    digitalWrite(outputPin, HIGH);
//    sleep(1);
//    digitalWrite(outputPin, LOW);
//    sleep(1);
//  }
//  return 0;
#ifdef USE_SSL
  // Free the SSL context and and set Temboo connections to no TLS
  endTembooSessionSSL(&theSession);
#endif
  return EXIT_SUCCESS;
}
