#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Wire.h>
#include <iostream>
#include <sstream>
#include <ESP32Encoder.h>
#include <AceSorting.h>


// MOTOR
#define MOTOR_IN1 48
#define MOTOR_IN2 47

// ENCODER
#define encoderPinA 11
#define encoderPinB 12
ESP32Encoder motorEncoder;

// LOCK
#define LOCK 10

// Front Measurement Sensor
#define trigPin 5
#define echoPin 6 

// Rear Measurement 2 Sensor
#define trigPin2 2
#define echoPin2 42

//define sound speed in in/uS
#define SOUND_SPEED_IN 0.013504

#define NUM_VALUES 5  // Number of values to average
#define NUM_VALUES2 10

float frontValues[NUM_VALUES];
float rearValues[NUM_VALUES];

float frontSpeedMedian[NUM_VALUES2];
float rearSpeedMedian[NUM_VALUES2];

float frontSpeedMedianSorted[NUM_VALUES2];
float rearSpeedMedianSorted[NUM_VALUES2];

int valueIndex = 0;  // Index to track the current position in the array
int speedIndex = 0;

long durationFront;
float distanceInchFront;

long durationRear;
float distanceInchRear;

float avgFront = 0;
float avgRear = 0;
float avgFrontSpeed;
float avgRearSpeed;

int encoderPosition = 0;

// Front Motion Sensor
#define MOTION_SENSOR_PIN 16
#define LED_PIN 15

int motionStateCurrent  = LOW; // current state of motion sensor's pin
int motionStatePrevious = LOW; // previous state of motion sensor's pin

// Rear Motion Sensor
#define MOTION_SENSOR_PIN2 40
#define LED_PIN2 39

int motionStateCurrent2  = LOW; // current state of motion sensor's pin
int motionStatePrevious2 = LOW; // previous state of motion sensor's pin

// Mobile App
const char* ssid = "SmartGate1";  
const char* password = "012345678";

// GATE LOGIC VARIABLES
#define CLOSE true
#define OPEN false

bool gateState = CLOSE;
bool gateMoving = false;
bool motionSensor = false;
bool measurementSensor = false;
bool rfidSensor = false;
bool gateOpening = true;

bool mobileOpen = false;
bool mobileClose = false;

AsyncWebServer server(80);
AsyncWebSocket wsGateInput("/GateInput");
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(

<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<style>
    .td {
        display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh; 
    }
    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }
    .button {
        font-family: 'Poppins', sans-serif;
        display: inline-block;
        outline: none;
        cursor: pointer;
        font-size: 18px;
        line-height: 1;
        border-radius: 500px;
        transition-property: background-color,border-color,color,box-shadow,filter;
        transition-duration: .3s;
        border: 1px solid transparent;
        letter-spacing: 2px;
        width: 200px;
        text-transform: uppercase;
        white-space: nowrap;
        font-weight: 700;
        text-align: center;
        padding: 20px 0;
        color: #659DBD;
        background-color: #fff;
        height: 64px;
        vertical-align: middle;
        margin-left: -50px;
        :hover{
            transform: scale(1.04);
            background-color: #BC986A;
        }
                
    }

    .button:focus {
    background-color: #fff;
    }

    .button:active {
    background-color: #DAAD86;
    box-shadow: none;
    }
</style>
    </head>

<body class = "noselect" align="center" style="background-color:#659DBD">
    <h1 style="color: #fff ;text-align:center; font-family: 'Poppins', sans-serif;">Gate Remote Control</h1>
    <div id="statusDisplay" style="color: #fff; text-align:center; font-family: 'Poppins', sans-serif;"></div>
    <table id="mainTable" style="width:100%;margin:auto;table-layout:fixed" CELLSPACING=25>
        <tr>
          <td></td>
          <td>
            <button class="button" ontouchstart='getStatus()'>Status</button>
          </td>
          <td></td>
        </tr>
        <tr>
          <td></td>
          <td>
            <button class="button" ontouchstart='sendButtonInput("OPEN")'>Open Gate</button>
          </td>
          <td></td>    
        </tr>
        <tr>
          <td></td>
          <td>
            <button class="button" ontouchstart='sendButtonInput("CLOSE")'>Close Gate</button>
          </td>
          <td></td>
        </tr>      
      </table>

    <script>
        var webSocketGateInputUrl = "ws:\/\/" + window.location.hostname + "/GateInput";
        var webSocketGateInput;
        
        function onGateInputWebSocketEvent(data) {
        console.log("Received data:", data);
        // Check if the received data is a valid JSON
        try {
            const parsedData = JSON.parse(data);
            if (parsedData.hasOwnProperty("type")) {
                // Check the type of the received data
                if (parsedData.type === "status") {
                    updateStatusDisplay(parsedData.status);
                }
            }
        } catch (error) {
            console.error("Error parsing WebSocket message:", error);
        }
    }

        function initGateInputWebSocket() 
        {
            webSocketGateInput = new WebSocket(webSocketGateInputUrl);
            webSocketGateInput.onopen = function(event){};
            webSocketGateInput.onclose   = function(event){setTimeout(initGateInputWebSocket, 2000);};
            webSocketGateInput.onmessage = function(event){onGateInputWebSocketEvent(event.data);};        
        }

        initGateInputWebSocket();

        function sendButtonInput(value) 
        {
            console.log(value);
            webSocketGateInput.send(value);
        }
        
        function updateStatusDisplay(status) {
            const statusDisplay = document.getElementById("statusDisplay");

            // Update the text based on the received status
            if (status === "Open" || status === "Close") {
                statusDisplay.textContent = `Gate Status: ${status}`;
            } else if (status === "Moving"){
                statusDisplay.textContent = "Gate Status: Opening/Closing";
            }
            else{
              statusDisplay.textContent = "Gate Status: Unknown (Is SmartGate Connected?)";
            }
        }

        function getStatus() {
            // Replace this with the actual logic to retrieve the gate status
            webSocketGateInput.send("STATUS");
        }
        webSocketGateInput.onmessage = function(event) {
            const receivedData = event.data;
            console.log("Received data:", event.data);
            // Check if the received data is a valid JSON
            try {
                const data = JSON.parse(receivedData);
                if (data.hasOwnProperty("type")) {
                    // Check the type of the received data
                    if (data.type === "status") {
                        updateStatusDisplay(data.status);
                    }
                }
            } catch (error) {
                console.error("Error parsing WebSocket message:", error);
            }
        };

        window.onload = initGateInputWebSocket;
        document.getElementById("mainTable").addEventListener("touchend", function(event){
            event.preventDefault()
        });
    </script>
</body>
</html>)HTMLHOMEPAGE";


void setup() {
  
  Serial.begin(115200);
  // "Mobile App" start-up
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.softAPIP());
    server.on("/", HTTP_GET, handleRoot);
    server.onNotFound(handleNotFound);

    wsGateInput.onEvent(onGateInputWebSocketEvent);
    server.addHandler(&wsGateInput);

    server.begin();
    Serial.println("HTTP server started");

  // Motor start-up  
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);
    pinMode(encoderPinA, INPUT);
    pinMode(encoderPinB, INPUT);
    motorEncoder.attachFullQuad(encoderPinA, encoderPinB);
  // LOCK start-up
    pinMode(LOCK, OUTPUT);
  
  // Front Measurement Sensor Start-up
    pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
    pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  // Rear Measurement Sensor Start-up
    pinMode(trigPin2, OUTPUT);
    pinMode(echoPin2, INPUT);

  // Front Motion Sensor Start-up
    pinMode(MOTION_SENSOR_PIN, INPUT); // set ESP32 pin to input mode
    pinMode(LED_PIN, OUTPUT);          // set ESP32 pin to output mode
 
  // Rear Motion Sensor Start-up
    pinMode(MOTION_SENSOR_PIN2, INPUT); // set ESP32 pin to input mode
    pinMode(LED_PIN2, OUTPUT);          // set ESP32 pin to output mode

    for (int i = 0; i < 50; i++)
    {
      measurementLoop();
      delay(100);
    }

    
}

void loop() {
  //Measurement Sensor Loop
  measurementLoop();
  
  // Motion Sensor Loop
  motionLoop();
  logicLoop();
  
  //motorTest();
  //findPosition();
  wsGateInput.cleanupClients();
  //delay(100);
}
void logicLoop(){
  // STATE: Gate is CLOSED
  if (gateState == CLOSE && gateMoving == false)
  {
    // GATE WILL OPEN if 1, 2, or 3 is true, change state to moving
    // motion detected
    if (motionSensor == true)
    {
      // Object too close reads really high speeds
      if (avgFront > 10)
      {
        // Measurement sensor detects animal or adult approaching
        if (((frontSpeedMedianSorted[NUM_VALUES / 2] >= 20.0 && frontSpeedMedianSorted[NUM_VALUES / 2] < 35.0)) || (rearSpeedMedianSorted[NUM_VALUES / 2] >= 20.0 && rearSpeedMedianSorted[NUM_VALUES / 2] < 35.0))
        {
          gateMoving = true;
        }
      }

    }
    else if (mobileOpen)
    {
      gateMoving = true;
      mobileOpen = false;
    }
  }
  // STATE: Gate is OPEN
  else if (gateState == OPEN && gateMoving == false)
  {
    // GATE WILL CLOSE
    if (motionSensor == false)
    {
      delay(3000);
      Serial.println("Gate closing");
      gateMoving = true;
    }
    // TODO: app requests to close
    else if (mobileClose)
    {
      gateMoving = true;
      mobileClose = false;
    }
  }

  // STATE: MOVING
  if (gateMoving == true)
  {
    // CLOSE gate
    if (gateState == OPEN)
    {
      // GATE WILL STAY IN THIS BLOCK OF CODE UNTIL GATE IS FINISHED OPEN/CLOSED
      while (encoderPosition >= 15)
      {
        encoderPosition = motorEncoder.getCount();
        Serial.print("Encoder position: ");
        Serial.println(encoderPosition);
        delay(100);
        measurementLoop();
        while ((avgFront < 12.0 || frontSpeedMedianSorted[NUM_VALUES / 2] > 20.0) || avgRear < 12.0 || rearSpeedMedianSorted[NUM_VALUES / 2] > 20.0) // object too close
        {
          // Stop the motor
          Serial.println("Object too close to gate");
          digitalWrite(MOTOR_IN1, 0);
          digitalWrite(MOTOR_IN2, 0);
          //delay for 3 seconds but measure anyways
          for (int i = 0; i < 30; i++)
          {
            Serial.println("Object too close to gate");
            measurementLoop();
            delay(100);
          }
        }
        if (encoderPosition <= 15) {
            // Stop the motor
            digitalWrite(MOTOR_IN1, 0);
            digitalWrite(MOTOR_IN2, 0);
            Serial.println("Gate Closed");
            gateState = CLOSE;
            gateMoving = false;
            //delay(5000);
          }
          else
          {
            //Serial.println("Closing gate");
            // Move motor
            digitalWrite(MOTOR_IN1, HIGH);
            digitalWrite(MOTOR_IN2, LOW);
          }
      }
    }
    // OPEN gate
    else
    {
      while (encoderPosition <= 2500)
      {
        encoderPosition = motorEncoder.getCount();
        Serial.print("Encoder position: ");
        Serial.println(encoderPosition);
        // Open Solenoid lock
        if (encoderPosition < 100)
        {
          Serial.println("open lock");
          digitalWrite(LOCK, HIGH);
        }
        else if (encoderPosition > 100)
        {
          Serial.println("close lock");
          digitalWrite(LOCK, LOW);
        }
        // Motor to open gate
        delay(100);
        measurementLoop();
        while ((avgFront < 12.0 || frontSpeedMedianSorted[NUM_VALUES / 2] > 20.0) || avgRear < 12.0 || rearSpeedMedianSorted[NUM_VALUES / 2] > 20.0) // object too close
        {
          // Stop the motor
          Serial.println("Object too close to gate");
          digitalWrite(MOTOR_IN1, 0);
          digitalWrite(MOTOR_IN2, 0);
          //delay for 3 seconds but measure anyways
          for (int i = 0; i < 30; i++)
          {
            Serial.println("Object too close to gate");
            measurementLoop();
            delay(100);
          }
          
        }
        Serial.println("HERE");
        if (encoderPosition >= 2500) {
            // Stop the motor
            digitalWrite(MOTOR_IN1, 0);
            digitalWrite(MOTOR_IN2, 0);
            Serial.println("Gate Open");
            gateState = OPEN;
            gateMoving = false;
            delay(5000);
          }
          else
          {
            //Serial.println("Opening gate");
            // Move motor
            digitalWrite(MOTOR_IN1, LOW);
            digitalWrite(MOTOR_IN2, HIGH);
          }
        }
      }
    }
  }

void motorTest(){

  digitalWrite(LOCK, 1);
  int encoderPosition = abs(motorEncoder.getCount());
  // Print encoder position to the serial monitor
  Serial.print("Encoder position: ");
  Serial.println(encoderPosition);
  Serial.println(gateOpening);
  if (gateOpening == true)
  {
    if (encoderPosition > 2500) {
      //Stop the motor
      analogWrite(MOTOR_IN1, LOW);
      analogWrite(MOTOR_IN2, LOW);
      Serial.print("Gate Opened");
      gateOpening = false;
      delay(5000);
    }
    else
    {
      analogWrite(MOTOR_IN1, LOW);
      analogWrite(MOTOR_IN2, 255);
    }
  }
  else if (gateOpening == false) // gateClosing
  {
    if (encoderPosition < 50) {
      Serial.println("HERE2");
      //Stop the motor
      analogWrite(MOTOR_IN1, 0);
      analogWrite(MOTOR_IN2, 0);
      Serial.print("Gate Closed");
      gateOpening = true;
      delay(5000);
    }
    else
    {
      Serial.println("HERE1");
      analogWrite(MOTOR_IN1, 255);
      analogWrite(MOTOR_IN2, LOW);
    }
  }
  //  // ramp up forward
  //   Serial.println("HERE");
  //   digitalWrite(MOTOR_IN1, LOW);
  //   for (int i=0; i<255; i++) {
  //     analogWrite(MOTOR_IN2, 255);
  //     // motionLoop();
  //     // if (motionStateCurrent2 == HIGH)
  //     // {
  //     //   analogWrite(MOTOR_IN2, LOW);
  //     //   break;
  //     // }
  //     delay(10);
  //   }

  //   // forward full speed for one second
  //   delay(500);
    
  //   // // ramp down forward
  //   // for (int i=255; i>=0; i--) {
  //   //   analogWrite(MOTOR_IN2, i);
  //   //   //motionLoop();
  //   //   // if (motionStateCurrent2 == HIGH)
  //   //   // {
  //   //   //   analogWrite(MOTOR_IN2, LOW);
  //   //   //   break;
  //   //   // }
  //   //   delay(10);
  //   // }
  // delay(1000);
  //   // ramp up backward
  //   digitalWrite(MOTOR_IN2, LOW);
  //   for (int i=0; i<255; i++) {
  //     analogWrite(MOTOR_IN1, 255);
  //     //motionLoop();
  //     // if (motionStateCurrent2 == HIGH)
  //     // {
  //     //   analogWrite(MOTOR_IN1, LOW);
  //     //   break;
  //     // }
  //     delay(10);
  //   }

  //   // backward full speed for one second
  //   delay(500);

  //   // // ramp down backward
  //   // for (int i=255; i>=0; i--) {
  //   //   analogWrite(MOTOR_IN1, i);
  //   //   // motionLoop();
  //   //   // if (motionStateCurrent2 == HIGH)
  //   //   // {
  //   //   //   analogWrite(MOTOR_IN1, LOW);
  //   //   //   break;
  //   //   // }
  //   //   delay(10);
  //   // }
}


void measurementLoop()
{
  // Clears the FRONT trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Clears the REAR trigPin
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);

  // Sets the FRONT trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Returns the sound wave travel time in microseconds
  durationFront = pulseIn(echoPin, HIGH);

  // Sets the REAR trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);

  durationRear = pulseIn(echoPin2, HIGH);
  
  // Convert to inches
  distanceInchFront = durationFront * SOUND_SPEED_IN/2;
  distanceInchRear = durationRear * SOUND_SPEED_IN/2;
  
  if (distanceInchFront > 72){
    distanceInchFront = 72.0;
  }
  if (distanceInchRear > 60){
    distanceInchRear = 60.0;
  }
  // Store the values in the arrays

  frontValues[valueIndex] = distanceInchFront;
  rearValues[valueIndex] = distanceInchRear;

  // Increment the index, and reset to 0 if it exceeds the array size
  valueIndex = (valueIndex + 1) % NUM_VALUES;

  // Calculate the average distance
  avgFront = 0;
  avgRear = 0;

  for (int i = 0; i < NUM_VALUES; ++i) {
    avgFront += frontValues[i];
    avgRear += rearValues[i];
  }

  avgFront /= NUM_VALUES;
  avgRear /= NUM_VALUES;

  // Units: inch/sec
  
  avgFrontSpeed = 0.0;
  avgRearSpeed = 0.0;
  for (int i = 0; i < NUM_VALUES - 1; ++i) {
    avgFrontSpeed += abs(frontValues[i] - frontValues[i+1]);
    avgRearSpeed += abs(rearValues[i] - rearValues[i+1]);
  }

  avgFrontSpeed /= NUM_VALUES;
  avgRearSpeed /= NUM_VALUES;

  avgFrontSpeed /= 0.5;
  avgRearSpeed /= 0.5;  


  speedIndex = (speedIndex + 1) % NUM_VALUES2;
  frontSpeedMedian[speedIndex] = avgFrontSpeed;
  rearSpeedMedian[speedIndex] = avgRearSpeed;

  for (int i = 0; i < NUM_VALUES2; i++)
  {
    frontSpeedMedianSorted[i] = frontSpeedMedian[i];
    rearSpeedMedianSorted[i] = rearSpeedMedian[i];
  }
  ace_sorting::insertionSort(frontSpeedMedianSorted, NUM_VALUES2);
  ace_sorting::insertionSort(rearSpeedMedianSorted, NUM_VALUES2);

  Serial.print("Average Front Side Distance in inches: ");
  Serial.println(avgFront);

  Serial.print("Average Rear Side Distance in inches: ");
  Serial.println(avgRear);

  Serial.print("Average Front Side Speed in inch/second: ");
  Serial.println(frontSpeedMedianSorted[NUM_VALUES2 / 2]);

  Serial.print("Average Rear Side Speed in inch/second: ");
  Serial.println(rearSpeedMedianSorted[NUM_VALUES2 / 2]);

}

void motionLoop()
{
  motionStatePrevious = motionStateCurrent;             // store old state
  motionStateCurrent  = digitalRead(MOTION_SENSOR_PIN); // read new state
  int pinState = digitalRead(LED_PIN);
  if (motionStatePrevious == LOW && motionStateCurrent == HIGH) { // pin state change: LOW -> HIGH
    Serial.println("Front Gate Motion detected!");
    digitalWrite(LED_PIN, HIGH); // turn on
    motionSensor = true;
  } else if (motionStatePrevious == HIGH && motionStateCurrent == LOW) { // pin state change: HIGH -> LOW
    Serial.println("Front Gate Motion stopped!");
    digitalWrite(LED_PIN, LOW);  // turn off
    motionSensor = false;
  }

  motionStatePrevious2 = motionStateCurrent2;             // store old state
  motionStateCurrent2  = digitalRead(MOTION_SENSOR_PIN2); // read new state
  int pinState2 = digitalRead(LED_PIN2);
  if (motionStatePrevious2 == LOW && motionStateCurrent2 == HIGH) { // pin state change: LOW -> HIGH
    Serial.println("Rear Gate Motion detected!");
    digitalWrite(LED_PIN2, HIGH); // turn on
    motionSensor = true;
  } else if (motionStatePrevious2 == HIGH && motionStateCurrent2 == LOW) { // pin state change: HIGH -> LOW
    Serial.println("Rear Gate Motion stopped!");
    digitalWrite(LED_PIN2, LOW);  // turn off
    motionSensor = false;
  }
}

void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
  request->send(404, "text/plain", "File Not Found");
}

void onGateInputWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        std::istringstream ss(myData);
        std::string key;
        std::getline(ss, key, ',');
        //Serial.printf("Key [%s]", key.c_str());   
        if (key == "OPEN")
        {
          // OPEN GATE     
          Serial.println("OPEN GATE");
          mobileOpen = true;

        }
        else if (key == "CLOSE")
        {
          // CLOSE GATE
          Serial.println("CLOSE GATE");
          mobileClose = true;
        }
        else if (key == "STATUS")
        {
          Serial.println("STATUS");
          // Replace this with the actual variable or logic that holds the current gate status
          String currentStatus;
          if (gateMoving == true)
          {
            currentStatus = "Moving";
          }
          else
          {
            if (gateState == CLOSE)
            {
              currentStatus = "Close";
            }
            else
            {
              currentStatus = "Open";
            }
          }

          // Construct a JSON response
          String jsonResponse = "{\"type\":\"status\",\"status\":\"" + currentStatus + "\"}";

          // Send the JSON response back to the client
          server->text(client->id(), jsonResponse);
        }
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}