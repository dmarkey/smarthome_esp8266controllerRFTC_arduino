#include <IRremoteESP8266.h>
#include <Event.h>
#include <Timer.h>
#include <DHT.h>
#include <RestClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Client.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define DHTPIN 0     // what digital pin we're connected to


#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

#define page "<html> <body> <form action='.' method='post'>  SSID: <input type='text' name='ssid'><br>  Password: <input type='password' name='password'><br>  <input type='submit' value='Submit'></form></body></html>"


const char* mqtt_server = "dmarkey.com";

const char* ssid = "";
const char* password = "";
//MDNSResponder mdns;

DHT dht(DHTPIN, DHTTYPE);
Timer t;

ESP8266WebServer server(80);
WiFiClient espClient;
WiFiClient webClient;
RestClient restClient("dmarkey.com", 8080);
PubSubClient client(espClient);


short rc_pin = 2;

int RECV_PIN = 3;


IRrecv irrecv(RECV_PIN);

#define ch1on "10110000000000010001"
#define ch1off "10110000000000000000"
#define ch2on "10110000000010010011"
#define ch2off "10110000000010000010"
#define ch3on "10110000000001010000"
#define ch3off "10110000000001000001"
#define ch4on "10110000000011010010"
#define ch4off "10110000000011000011"
#define chMon "10110000000011110000"
#define chMoff "10110000000011100001"
#define dimmUp1 "10110000000000001010"
#define dimmDn1 "10110000000000011011"
#define dimmUp2 "10110000000010001000"
#define dimmDn2 "10110000000010011001"
#define dimmUp3 "10110000000001001011"
#define dimmDn3 "10110000000001011010"
#define dimmUp4 "10110000000011001001"
#define dimmDn4 "10110000000011011000"



boolean sendCode(char code[]) { //empfange den Code in Form eines Char[]
  sendSymbol('x');
  for (short z = 0; z < 7; z++) { //wiederhole den Code 7x
    for (short i = 0; i < 20; i++) { //ein Code besteht aus 20bits
      sendSymbol(code[i]);
    }
    sendSymbol('x'); //da der code immer mit x/sync abschliesst, brauchen wir den nicht im code und haengen es autisch immer hinten ran.
  }
  return true;
}
void sendSymbol(char i) { //Diese Funktion soll 0,1 oder x senden koennen. Wir speichern die gewuenschte Ausgabe in der Variabel i
  switch (i) { //nun gucken wir was i ist
    case '0':
      { //Der Code fuer '0'
        digitalWrite(rc_pin, LOW);
        delayMicroseconds(600);
        digitalWrite(rc_pin, HIGH);
        delayMicroseconds(1400);
        return;
      }
    case '1':
      { //Der Code fuer '1'
        digitalWrite(rc_pin, LOW);
        delayMicroseconds(1300);
        digitalWrite(rc_pin, HIGH);
        delayMicroseconds(700);
        return;
      }
    case 'x':
      { //Der Code fuer x(sync)
        digitalWrite(rc_pin, LOW);
        delay(81);
        digitalWrite(rc_pin, HIGH);
        delayMicroseconds(800);
      }

  }
}


int connected = false;

const int led = 13;

#define RF_DATA_PIN    2



#define MAX_CODE_CYCLES 4

#define SHORT_DELAY       380
#define NORMAL_DELAY      500
#define SIGNAL_DELAY     1500

#define SYNC_DELAY       2650
#define EXTRASHORT_DELAY 3000
#define EXTRA_DELAY     10000

enum {
  PLUG_A = 0,
  PLUG_B,
  PLUG_C,
  PLUG_D,
  PLUG_MASTER,
  PLUG_LAST
};

unsigned long signals[PLUG_LAST][2][MAX_CODE_CYCLES] = {
  { /*A*/
    {0b101111000001000101011100, 0b101100010110110110101100, 0b101110101110010001101100, 0b101101000101010100011100},
    {0b101101010010011101111100, 0b101111100011110000101100, 0b101111110111001110001100, 0b101110111000101110111100}
  },
  { /*B*/
    {0b101101110100001000110101, 0b101101101010100111100101, 0b101110011101111000000101, 0b101100100000100011110101},
    {0b101111011001101011010101, 0b101100111011111101000101, 0b101110001100011010010101, 0b101100001111000011000101}
  },
  { /*C*/
    {0b101101010010011101111110, 0b101111100011110000101110, 0b101111110111001110001110, 0b101110111000101110111110},
    {0b101110101110010001101110, 0b101101000101010100011110, 0b101111000001000101011110, 0b101100010110110110101110}
  },
  { /*D*/
    {0b101111011001101011010111, 0b101100111011111101000111, 0b101100001111000011000111, 0b101110001100011010010111},
    {0b101101110100001000110111, 0b101100100000100011110111, 0b101101101010100111100111, 0b101110011101111000000111}
  },
  { /*MASTER*/
    {0b101111100011110000100010, 0b101110111000101110110010, 0b101101010010011101110010, 0b101111110111001110000010},
    {0b101111000001000101010010, 0b101101000101010100010010, 0b101110101110010001100010, 0b101100010110110110100010}
  },
};

boolean       onOff;
unsigned char plug;
unsigned char swap;


void sendSync() {
  digitalWrite(RF_DATA_PIN, HIGH);
  delayMicroseconds(SHORT_DELAY);
  digitalWrite(RF_DATA_PIN, LOW);
  delayMicroseconds(SYNC_DELAY - SHORT_DELAY);
}

void sendValue(boolean value, unsigned int base_delay) {
  unsigned long d = value ? SIGNAL_DELAY - base_delay : base_delay;
  digitalWrite(RF_DATA_PIN, HIGH);
  delayMicroseconds(d);
  digitalWrite(RF_DATA_PIN, LOW);
  delayMicroseconds(SIGNAL_DELAY - d);
}

void longSync() {
  digitalWrite(RF_DATA_PIN, HIGH);
  delayMicroseconds(EXTRASHORT_DELAY);
  digitalWrite(RF_DATA_PIN, LOW);
  delayMicroseconds(EXTRA_DELAY - EXTRASHORT_DELAY);
}

void ActivatePlug(unsigned char PlugNo, boolean On) {
  Serial.print("Activating Plug ");
  Serial.println(PlugNo);
  delayMicroseconds(1000);
  if ( PlugNo < PLUG_LAST ) {

    digitalWrite(RF_DATA_PIN, LOW);
    delayMicroseconds(1000);

    unsigned long signal = signals[PlugNo][On][swap];

    swap++;
    swap %= MAX_CODE_CYCLES;

    for (unsigned char i = 0; i < 4; i++) { // repeat 1st signal sequence 4 times
      sendSync();
      for (unsigned char k = 0; k < 24; k++) { //as 24 long and short signals, this loop sends each one and if it is long, it takes it away from total delay so that there is a short between it and the next signal and viceversa
        sendValue(bitRead(signal, 23 - k), SHORT_DELAY);
      }
    }
    for (unsigned char i = 0; i < 4; i++) { // repeat 2nd signal sequence 4 times with NORMAL DELAY
      longSync();
      for (unsigned char k = 0; k < 24; k++) {
        sendValue(bitRead(signal, 23 - k), NORMAL_DELAY);
      }
    }

  }
  Serial.print("Done: ");
  Serial.println(PlugNo);
}






String mqttNameCmd() {
  String name;
  int id = ESP.getChipId();
  name = "SmartController-";
  name  += id;
  return name;
}


String commandTopicCmd() {
  String topic;
  int id = ESP.getChipId();
  topic = "/smart_plug_work/SmartPlug-";
  topic  = topic + id;
  return topic;
}



String commandTopic = commandTopicCmd();

String mqttName = mqttNameCmd();



String eeprom_read() {
  String tempStr;
  for (int i = 0; i < 512; i++) {
    byte temp = EEPROM.read(i);
    if (temp != '\0') {
      tempStr += (char)temp;
    }
  }
  return tempStr;
}


void eeprom_write(String buf) {
  for (int i = 0; i < buf.length(); i++) {
    EEPROM.write(i, buf[i]);
  }
  EEPROM.write(buf.length() + 1, '\0');
  EEPROM.end();
}

void handleRoot() {
  if (server.method() == HTTP_POST) {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["ssid"] = server.arg("ssid");
    root["password"] = server.arg("password");
    char buffer[256];
    root.printTo(buffer, sizeof(buffer));
    String strBuf(buffer);
    eeprom_write(strBuf);
    Serial.print(strBuf);
    server.send(200, "text/plain", "Restarting");
    delay(1000);
    ESP.restart();
  }

  server.send(200, "text/html", page);
}


void handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);

}


void beaconFunc() {
  restClient.setClient(&webClient);
  String post_data;
  post_data = "model=SmartController-8RFT&controller_id=";
  post_data += ESP.getChipId();
  post_data += "\r\n";
  //espClient2.connect("dmarkey.com", 80);
  int resp = restClient.post("/controller_ping_create/", post_data.c_str(), NULL);

  Serial.print("REST Response:");
  Serial.println(resp);

  //queue_web_request("http://dmarkey.com:8080/controller_ping_create/", post_data);
  //hc.setPostBody(post_data);
  //hc.downloadString("http://dmarkey.com:8080/controller_ping_create/", processBeaconResponse);
}


void setTaskStatus(char * task_id, int status) {

  Serial.println(task_id);
  char post_data[128];

  DynamicJsonBuffer outJsonBuffer;


  JsonObject& root = outJsonBuffer.createObject();
  root["controller_id"] = ESP.getChipId();
  root["task_id"] = task_id;
  root["status"] = status;

  root.printTo(post_data, sizeof(post_data));
  client.publish("/admin/task_status", post_data, strlen(post_data));
  client.loop();
  //queue_web_request("http://dmarkey.com:8080/controller_task_status/", post_data, "application/json");
}


void processSwitchcmd(JsonObject& obj) {

  int switch_num = obj["socketnumber"];



  bool state = obj["state"];


  if (switch_num < 5) {
    if (state) {

      switch (switch_num) {
        case 1:
          {
            sendCode(ch1on);
            Serial.println("command ch1on executed");
            break;
          }
        case 2:
          {
            sendCode(ch2on);
            Serial.println("command ch2on executed");
            break;
          }
        case 3:
          {
            sendCode(ch3on);
            Serial.println("command ch3on executed");
            break;
          }
        case 4:
          {
            sendCode(ch4on);
            Serial.println("command ch4on executed");
            break;
          }
      }

    }
    else {


      switch (switch_num) {
        case 1:
          {
            sendCode(ch1off);
            Serial.println("command ch1on executed");
            break;
          }
        case 2:
          {
            sendCode(ch2off);
            Serial.println("command ch2on executed");
            break;
          }
        case 3:
          {
            sendCode(ch3off);
            Serial.println("command ch3on executed");
            break;
          }
        case 4:
          {
            sendCode(ch4off);
            Serial.println("command ch4on executed");
            break;
          }
      }

    }
    return;
  }


  ActivatePlug(switch_num - 5, state);


}

void callback(char* topic, byte* payload, unsigned int length)
{

  Serial.println(millis());

  Serial.println("Incoming command");
  Serial.println((char *)payload);
  Serial.println(ESP.getFreeHeap(), DEC);
  char task_id[40];

  if (String(topic) == commandTopic) {
    Serial.println(ESP.getFreeHeap(), DEC);
    DynamicJsonBuffer inJsonBuffer;

    JsonObject& root = inJsonBuffer.parseObject((char *)payload);
    if (!root.success()) {
      Serial.print("Problem parsing this message:");
      Serial.println((char *)payload);
      return;
    }

    const char * command = root["name"];
    snprintf(task_id, 40, "%s", (const char *)root["task_id"]);
    Serial.println("Setting task status");

    if (strcmp(command, "sockettoggle") != -1) {
      processSwitchcmd(root);
      setTaskStatus(task_id, 3);
    }

  }

}

void readTemp() {
  DynamicJsonBuffer tempJsonBuffer;
  char post_data[128];
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.println(t);



  JsonObject& root = tempJsonBuffer.createObject();
  root["controller_id"] = ESP.getChipId();
  root["event"] = "Temp";
  root["route"] = "Temperature";
  root["value"] = t;
  root["hum"] = h;

  root.printTo(post_data, sizeof(post_data));
  client.publish("/admin/events", post_data, strlen(post_data));
  client.loop();
  delay(10);

}



void setup(void) {
  EEPROM.begin(512);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting..");
  pinMode(RECV_PIN, OUTPUT);
  pinMode(RF_DATA_PIN, OUTPUT);

  irrecv.enableIRIn();
  StaticJsonBuffer<1000> json;
  JsonObject& root = json.parseObject(eeprom_read());


  if (root.success()) {
    ssid = (const char*)root["ssid"];
    password = (const char*)root["password"];
    Serial.println("Good WIFI Details.");
  } else {
    Serial.println("WIFI Details missing");
  }
  Serial.print("SSID:");
  Serial.println(ssid);
  Serial.print("Password:");
  Serial.println(password);
  WiFi.begin(ssid, password);

  for (int i = 0; i < 5; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WIFI CONNECTED!");
      connected = true;
      break;
    }
    delay(5000);
  }

  if (!connected) {
    fallbackMode();
  }
  else {
    beaconFunc();
    client.setServer(mqtt_server, 8000);
    client.setCallback(callback);
    t.every(10000, readTemp);

  }

}

void fallbackMode() {
  String my_ssid = "SmartHome-";
  my_ssid += ESP.getChipId();
  WiFi.softAP(my_ssid.c_str(), "", false);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.print(myIP);
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();


}

int attempt = 0;


void sendMQTTBeacon() {
  char buf[200];
  DynamicJsonBuffer BeaconBuff;
  JsonObject& root = BeaconBuff.createObject();
  root["controller_id"] = ESP.getChipId();
  root["event"] = "BEACON";
  root["route"] = "All";
  root.printTo(buf, sizeof(buf));
  client.publish("/admin/events", buf, strlen(buf));
  client.loop();

}

void reconnect() {
  DynamicJsonBuffer willJsonBuffer;
  // Loop until we're reconnected
  while (!client.connected()) {

    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    char buf[200];
    JsonObject& root = willJsonBuffer.createObject();
    root["controller_id"] = ESP.getChipId();
    root["event"] = "disconnect";
    root["route"] = "ALL";
    root.printTo(buf, sizeof(buf));
    if (client.connect(mqttName.c_str(), "/admin/events", false, 0, buf)) {
      Serial.println("connected");
      attempt = 0;
      // Once connected, publish an announcement...
      client.subscribe(commandTopic.c_str());
      Serial.print("Subscribed to ");
      Serial.println(commandTopic);
      sendMQTTBeacon();
    } else {
      attempt++;
      if (attempt > 5) {
        connected = false;
        fallbackMode();
      }
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

const char *  encoding (decode_results *results)
{
  switch (results->decode_type) {
    default:
    case UNKNOWN:      return NULL;       break ;
    case NEC:          return "NEC";           break ;
    case SONY:         return "SONY";          break ;
    case RC5:          return "RC5";           break ;
    case RC6:          return "RC6";           break ;
    case DISH:         return "DISH";          break ;
    case SHARP:        return "SHARP";         break ;
    case JVC:          return "JVC";           break ;
    case SANYO:        return "SANYO";         break ;
    case MITSUBISHI:   return "MITSUBISHI";    break ;
    case SAMSUNG:      return "SAMSUNG";       break ;
    case LG:           return "LG";            break ;
    case WHYNTER:      return "WHYNTER";       break ;
    case AIWA_RC_T501: return "AIWA_RC_T501";  break ;
    case PANASONIC:    return "PANASONIC";     break ;
  }
}


void  dumpInfo (decode_results *results)
{

  char buf[200];
  DynamicJsonBuffer IRJsonBuffer;
  JsonObject& root = IRJsonBuffer.createObject();
  root["controller_id"] = ESP.getChipId();
  root["event"] = "IRIN";
  root["route"] = "RemoteControl";
  const char * encoding_str = encoding(results);
  if (encoding_str == NULL || results->value == -1) {

    return;

  }
  root["encoding"] = encoding_str;
  root["code"] = results->value;
  root.printTo(buf, sizeof(buf));
  client.publish("/admin/events", buf, strlen(buf));
  client.loop();
}



void loop(void) {

  //Serial.println("Looping!");
  if (!connected) {
    server.handleClient();
  }
  else {

    if (!client.connected()) {
      reconnect();
    }
    decode_results  results;        // Somewhere to store the results
    if (irrecv.decode(&results)) {  // Grab an IR code
      dumpInfo(&results);           // Output the results
      irrecv.resume();              // Prepare for the next value
    }
    client.loop();
    delay(10);
    t.update();

  }
}

