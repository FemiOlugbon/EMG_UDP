#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi_internal.h>
#include <AsyncUDP.h>


// Pin definitions
#define MYOWARE_PIN_1  34   // myoware 1 signal pin
#define MYOWARE_PIN_2  35   // myoware 1 signal pin
#define LED_PIN 2

#define BUFFER_SIZE 256



// LED blinking speed (frequency = 1/(INTERVAL_MS * 10^-3)
#define INTERVAL_MS 250    

// Sampling rate 
// 100 us is equivalent to 1kHz sampling frequency.
#define SAMPLING_INTERVAL 200     // this value is defined in micro seconds

// Set WiFi Speed
#define WIFI_DATA_RATE WIFI_PHY_RATE_54M // WIFI_PHY_RATE_11M_S

// Set BOARD ID
#define BOARD_ID 1

// Uncomment next line to print debug data to the terminal
#define DEBUG

// struct to hold emg data
typedef struct __attribute__((__packed__)) EMGData
{
  uint8_t header[2] = {0xAA, BOARD_ID};
  uint32_t counter = 0; 
  uint16_t emg[2];  // Raw ADC readings
  uint8_t  gesture = 1;
} EMGData;

// EMG data sent
EMGData myoWare;
// LED state
boolean on = false;

// data from each sender
uint16_t idx = 0;
uint8_t *data;
unsigned long lastSentTime = 0;
unsigned long lastSampleTime = 0;

// WiFi SSID and Password
// Change this to match your HotSpot name and password
const char *ssid = "Wearable_Systems";
const char *password = "47789517";

// udp connection
AsyncUDP udp;

// data transmission routine
void onDataSent(const uint8_t *outgoingData, int len)
{
  // Check if it is a valid data size
  if ( len == sizeof(EMGData))
  {
    // Check if buffer is full
    if (idx + len < BUFFER_SIZE)
    {
      memcpy(data + idx, outgoingData, len);
      idx += len;
    }
    else // if buffer is full, send to MATLAB
    {
      udp.write(data, idx);
      memcpy(data, outgoingData, len);
      idx = len;
      #ifdef DEBUG
        Serial.println("Data Transmitted!");
      #endif
    }
    lastSentTime = millis();
  }
}

// Debug Data
void printData(EMGData incomingData)
{
  
  Serial.print("Counter:  ");Serial.print(incomingData.counter);Serial.print("  ");Serial.print("Myoware_1:  "); Serial.print(incomingData.emg[0]);Serial.print("  ");Serial.print("Myoware_2:  "); Serial.print(incomingData.emg[1]);Serial.print("\n");
}

// Data sampling routine
void dataSampling(void *parameter)
{
  while (true)
  {
    if (millis() - lastSampleTime >= SAMPLING_INTERVAL)
    {
      // Increment the last sample time
      lastSampleTime += SAMPLING_INTERVAL;
      
      // Sample here
      myoWare.counter++;
      myoWare.emg[0] = analogRead(MYOWARE_PIN_1);
      myoWare.emg[1] = analogRead(MYOWARE_PIN_2);

      // Send the data out
      onDataSent((uint8_t *) &myoWare, sizeof(myoWare));
      #ifdef DEBUG
      printData(myoWare);
      #endif
    }
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  // set LED pin as output
  pinMode(LED_PIN, OUTPUT);

  // initialize serial for debugging
  Serial.begin(115200);

  // set the device as a station
  WiFi.mode(WIFI_STA);

  // stop WiFi to change configuration
  esp_wifi_stop();
  esp_wifi_deinit();

  // disable AMPDU to set a fix rate
  wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT(); // use the default config ...
  my_config.ampdu_tx_enable = 0;                             // ... and modify only what we want.
  esp_wifi_init(&my_config);                                 // set the new config

  // set the WiFi rate
  esp_wifi_internal_set_fix_rate(ESP_IF_WIFI_STA, true, WIFI_DATA_RATE);

  // disable power save mode
  esp_wifi_set_ps(WIFI_PS_NONE);

  // start WiFi
  esp_wifi_start();

  // start Wifi connection
  WiFi.begin(ssid, password);
  // wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    on = !on;
    digitalWrite(LED_PIN, on ? LOW : HIGH);
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }

  // keep the LED on
  on = true;
  digitalWrite(LED_PIN, HIGH);

  // print the WiFi information
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  Serial.println(WiFi.macAddress());

  // initialize ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // start udp
  udp.connect(IPAddress(192, 168, 137, 1), 12345);
  data = (uint8_t *)malloc(BUFFER_SIZE);

  // Create a tasks to handle data transmission
  xTaskCreate(dataSampling, "dataSampling", 5000, NULL, 1, NULL);
}

void loop() {

  // Turn on esp32 onboard LED to signify that transmission is on
  // toggle the LED if sending data, otherwise keep it on
  if (millis() - lastSentTime <= 5000)
  {
    // toggle led every INTERVAL_MS milliseconds
    on = !on;
    digitalWrite(LED_PIN, on ? LOW : HIGH);
  }
  else
  {
    on = true;
    digitalWrite(LED_PIN, HIGH);
  }
  delay(INTERVAL_MS);
}