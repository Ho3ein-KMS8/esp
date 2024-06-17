#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>

// variables
String ssid = "";
int ssidLen;
String pass = "";
int passLen;
String port = "";
int portLen;
String localIp = "";
int localIpLen;
String gateway = "";
int gatewayLen;
String subnet = "";
int subnetLen;
String dns1 = "";
int dns1Len;
String dns2 = "";
int dns2Len;
String serverUsername = "";
int serverUsernameLen;
String serverPass = "";
int serverPassLen;
String serverLocalDomainName = "";
int serverLocalDomainNameLen;
String serverPort = "";
int serverPortLen;
String mqttClientId = "";
int mqttClientIdLen;
String mqttBrokerIp = "";
int mqttBrokerIpLen;
String mqttPort = "";
int mqttPortLen;
String mqttUsername = "";
int mqttUsernameLen;
String mqttPass = "";
int mqttPassLen;
String mqttPublishTopic = "";
int mqttPublishTopicLen;
String mqttSubscribeTopic = "";
int mqttSubscribeTopicLen;

String ssids = "";    // we save result of available wifi here
int counter = 0;      // used in loop function to set interval for readSsids function
bool dhcp = 0;        // 0: off   1: on
int start = 0;        // the beginning of range that for-loop begin
int end = 0;          // the end of range that for-loop goes
bool serverAuth = 0;  // used for server
bool mqttAuth = 0;    // used for mqtt
bool flag = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

IPAddress _localIp;
IPAddress _gateway;
IPAddress _subnet;
IPAddress _dns1;
IPAddress _dns2;
IPAddress _mqttBrokerIp;

// functions
void initLittleFS();
void initEEPROM();

void saveCredentials(String, String, String);
void readCredentials();
void showCredentials();
void clearCredentialsVariables();

void saveNetworkSettings(bool ,String ,String ,String ,String ,String);
void readNetworkSettings();
void showNetworkSettings();
void clearNetworkSettingsVariables(); 

void saveServerSettings(bool, String, String, String, String);
void readServerSettings();
void showServerSettings();
void clearServerSettingsVariables();

void saveMqttSettings(String, String, String, bool, String, String, String, String);
void readMqttSettings();
void showMqttSettings();
void clearMqttSettingsVariables();

void setupAP();
void createWebServer();
bool wifiConfig(IPAddress, IPAddress, IPAddress, IPAddress, IPAddress);
bool testWifi();
void showNetworkInfo();
void readSsids();
void mqttSetup();
void mqttCallback(char *, byte *, unsigned int);
void mqttConnect();

// create AsyncWebServer object
AsyncWebServer *server;

void setup() {
  Serial.begin(74880);
  pinMode(LED_BUILTIN, OUTPUT);

  initLittleFS();
  initEEPROM();

  Serial.print("\n\nStartup\n");
  Serial.print("\nDisconnecting previously connected WiFi\n");
  WiFi.disconnect();

  // saveCredentials("Irancell", "Password", "80");
  readCredentials();
  showCredentials();

  // saveNetworkSettings(0 ,"192.168.1.200", "192.168.1.1", "255.255.255.0", "8.8.8.8", "8.8.4.4");
  readNetworkSettings();
  showNetworkSettings();

  // saveServerSettings(0, "Ali", "12345", "testDomain", "300");
  readServerSettings();
  showServerSettings();

  // saveMqttSettings("client-1", "127.1.1.10", "250", 0, "username1", "123456", "device/temp", "device/led");
  readMqttSettings();
  showMqttSettings();

  if(testWifi()) {
    Serial.print("\nSuccesfully Connected!!!\n");
    
    _localIp.fromString(localIp);
    _gateway.fromString(gateway);
    _subnet.fromString(subnet);
    _dns1.fromString(dns1);
    _dns2.fromString(dns2);

    if(!dhcp && wifiConfig(_localIp, _gateway, _subnet, _dns1, _dns2)) {
      Serial.print("\nConfiguratin successfully.\n");
    }

    showNetworkInfo();

    mqttSetup();

    mqttConnect();
  }
  else {
    Serial.println("Turning the HotSpot On");

    setupAP();

    if(!MDNS.begin(serverLocalDomainName.c_str()))
      Serial.println("Error setting up MDNS responder!");
    Serial.println("mDNS responder started.");

    createWebServer();
  }
}

void loop() {
  counter++;
  if(counter == 1000000) {
    counter = 0;
    readSsids();
  }
  if(flag == true) {
    mqttConnect();
    mqttClient.loop();
  }

  // if ((WiFi.status() == WL_CONNECTED))
  // {
  //   for (int i = 0; i < 10; i++)
  //   {
  //     digitalWrite(LED_BUILTIN, HIGH);
  //     delay(1000);
  //     digitalWrite(LED_BUILTIN, LOW);
  //     delay(1000);
  //   }
  // }
}


void clearNetworkSettingsVariables() {
  localIp = "";
  gateway = "";
  subnet = "";
  dns1 = "";
  dns2 = "";
}

void clearCredentialsVariables() {
  ssid = "";
  pass = "";
  port = "";
}

void clearServerSettingsVariables() {
  serverUsername = "";
  serverPass = "";
  serverLocalDomainName = "";
  serverPort = "";
}

void clearMqttSettingsVariables() {
  mqttClientId = "";
  mqttBrokerIp = "";
  mqttPort = "";
  mqttUsername = "";  
  mqttPass = "";
  mqttPublishTopic = "";
  mqttSubscribeTopic = "";
}

void initLittleFS() {
  if(!LittleFS.begin())
    Serial.println("\n\nAn Error has occurred while mounting LittleFS");
}

void initEEPROM() {
  EEPROM.begin(1024);
  delay(10);
}

void saveCredentials(String tSsid, String tPass, String tPort) {
  readNetworkSettings();
  readServerSettings();
  readMqttSettings();

  ssidLen = tSsid.length();
  passLen = tPass.length();
  portLen = tPort.length();
  
  start = 0;
  end = start + ssidLen;
  for(int i = start; i < end; i++)
    EEPROM.write(i, int(tSsid[i]));

  start = end;
  end = start + passLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tPass[j]));

  start = end;
  end = start + portLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tPort[j]));
    

  EEPROM.write(900, ssidLen);
  EEPROM.write(901, passLen);
  EEPROM.write(902, portLen);

  saveNetworkSettings(dhcp, localIp, gateway, subnet, dns1, dns2);

  EEPROM.commit();
  
}

void readCredentials() {
  clearCredentialsVariables();

  ssidLen = EEPROM.read(900);  
  passLen = EEPROM.read(901);
  portLen = EEPROM.read(902);

  start = 0;
  end = start+ssidLen;
  for (int i = start; i < end; i++)
    ssid += char(EEPROM.read(i));

  start = end;
  end =  start + passLen;
  for (int i = start; i < end; i++)
    pass += char(EEPROM.read(i));
  
  start = end;
  end = start + portLen;
  for(int i = start; i < end; i++)
    port += char(EEPROM.read(i));
}

void showCredentials() {
  Serial.print("\n\nSaved Credentials\n");
  Serial.println("===========================");
  Serial.print("SSID: ");
  Serial.println(ssid);

  Serial.print("Pass: ");
  Serial.println(pass);

  Serial.print("Port: ");
  Serial.println(port);
}

void saveNetworkSettings(bool tDhcp, String tLocalIp, String tGateway, String tSubnet, String tDns1, String tDns2) {
  localIpLen = tLocalIp.length();
  gatewayLen = tGateway.length();
  subnetLen = tSubnet.length();
  dns1Len = tDns1.length();
  dns2Len = tDns2.length();

  start = ssidLen+passLen+portLen;
  end = start + 1;
  EEPROM.write(start, tDhcp);

  start = end;
  end = start+localIpLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tLocalIp[j]));

  start = end;
  end = start+gatewayLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tGateway[j]));

  start = end;
  end = start+subnetLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tSubnet[j]));

  start = end;
  end = start+dns1Len;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tDns1[j]));

  start = end;
  end = start+dns2Len;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tDns2[j]));

  EEPROM.write(903, localIpLen);
  EEPROM.write(904, gatewayLen);
  EEPROM.write(905, subnetLen);
  EEPROM.write(906, dns1Len);
  EEPROM.write(907, dns2Len);

  saveServerSettings(serverAuth, serverUsername, serverPass, serverLocalDomainName, serverPort);

  EEPROM.commit();
}

void readNetworkSettings() {
  clearNetworkSettingsVariables();

  localIpLen = EEPROM.read(903);
  gatewayLen = EEPROM.read(904);
  subnetLen = EEPROM.read(905);
  dns1Len = EEPROM.read(906);
  dns2Len = EEPROM.read(907);
  
  start = ssidLen+passLen+portLen;
  end = start+1;
  dhcp = bool(EEPROM.read(start));

  start = end;
  end = start + localIpLen;
  for (int i = start; i < end; ++i)
    localIp += char(EEPROM.read(i));

  start = end;
  end = start+gatewayLen;
  for (int i = start; i < end; ++i)
    gateway += char(EEPROM.read(i));
  
  start = end;
  end = start+subnetLen;
  for(int i = start; i < end; ++i)
    subnet += char(EEPROM.read(i));

  start = end;
  end = start+dns1Len;
  for(int i = start; i < end; ++i)
    dns1 += char(EEPROM.read(i));

  start = end;
  end = start+dns2Len;
  for(int i = start; i < end; ++i)
    dns2 += char(EEPROM.read(i));
}

void showNetworkSettings() {
  Serial.print("\n\nSaved Network Settings\n");
  Serial.println("===========================");
  Serial.print("DHCP: ");
  Serial.println(dhcp ? "on" : "off");

  Serial.print("Local IP: ");
  Serial.println(localIp);

  Serial.print("Gateway: ");
  Serial.println(gateway);

  Serial.print("Subnet: ");
  Serial.println(subnet);

  Serial.print("DNS1: ");
  Serial.println(dns1);

  Serial.print("DNS2: ");
  Serial.println(dns2);

  Serial.print("\n");
}

void saveServerSettings(bool tAuth, String tUsername, String tPassword, String tLocalDomainName, String tPort) {
  serverUsernameLen = tUsername.length();
  serverPassLen = tPassword.length();
  serverLocalDomainNameLen = tLocalDomainName.length();
  serverPortLen = tPort.length();

  start = ssidLen+passLen+portLen+localIpLen+gatewayLen+subnetLen+dns1Len+dns2Len+1;
  end = start + 1;
  EEPROM.write(start, tAuth);

  start = end;
  end = start+serverUsernameLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tUsername[j]));

  start = end;
  end = start+serverPassLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tPassword[j]));

  start = end;
  end = start+serverLocalDomainNameLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tLocalDomainName[j]));

  start = end;
  end = start+serverPortLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tPort[j]));

  EEPROM.write(908, serverUsernameLen);
  EEPROM.write(909, serverPassLen);
  EEPROM.write(910, serverLocalDomainNameLen);
  EEPROM.write(911, serverPortLen);

  saveMqttSettings(mqttClientId, mqttBrokerIp, mqttPort, mqttAuth, mqttUsername, mqttPass, mqttPublishTopic, mqttSubscribeTopic);

  EEPROM.commit();
}

void readServerSettings() {
  clearServerSettingsVariables();

  serverUsernameLen = EEPROM.read(908);
  serverPassLen = EEPROM.read(909);
  serverLocalDomainNameLen = EEPROM.read(910);
  serverPortLen = EEPROM.read(911);
  
  start = ssidLen+passLen+portLen+localIpLen+gatewayLen+subnetLen+dns1Len+dns2Len+1;
  end = start+1;
  serverAuth = bool(EEPROM.read(start));

  start = end;
  end = start + serverUsernameLen;
  for (int i = start; i < end; ++i)
    serverUsername += char(EEPROM.read(i));

  start = end;
  end = start+serverPassLen;
  for (int i = start; i < end; ++i)
    serverPass += char(EEPROM.read(i));
  
  start = end;
  end = start+serverLocalDomainNameLen;
  for(int i = start; i < end; ++i)
    serverLocalDomainName += char(EEPROM.read(i));

  start = end;
  end = start+serverPortLen;
  for(int i = start; i < end; ++i)
    serverPort += char(EEPROM.read(i));
}

void showServerSettings() {
  Serial.print("\n\nSaved Server Settings\n");
  Serial.println("===========================");
  Serial.print("Authentication: ");
  Serial.println(serverAuth ? "on" : "off");

  Serial.print("Username: ");
  Serial.println(serverUsername);

  Serial.print("Password: ");
  Serial.println(serverPass);

  Serial.print("Local Domain Name: ");
  Serial.println(serverLocalDomainName);

  Serial.print("Server Port: ");
  Serial.println(serverPort);

  Serial.print("\n");
}

void saveMqttSettings(String tClientId, String tBrokerIp, String tPort, bool tAuth, String tUsername, String tPassword, String tPublishTopic, String tSubscribeTopic) {
  mqttClientIdLen = tClientId.length();
  mqttBrokerIpLen = tBrokerIp.length();
  mqttPortLen = tPort.length();
  mqttUsernameLen = tUsername.length();
  mqttPassLen = tPassword.length();
  mqttPublishTopicLen = tPublishTopic.length();
  mqttSubscribeTopicLen = tSubscribeTopic.length();

  start = ssidLen+passLen+portLen+localIpLen+gatewayLen+subnetLen+dns1Len+dns2Len+serverUsernameLen+serverPassLen+serverLocalDomainNameLen+serverPortLen+2;
  end = start + mqttClientIdLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tClientId[j]));

  start = end;
  end = start+mqttBrokerIpLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tBrokerIp[j]));

  start = end;
  end = start+mqttPortLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tPort[j]));

  start = end;
  end = start+1;
  EEPROM.write(start, tAuth);

  start = end;
  end = start+mqttUsernameLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tUsername[j]));

  start = end;
  end = start+mqttPassLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tPassword[j]));

  start = end;
  end = start+mqttPublishTopicLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tPublishTopic[j]));

  start = end;
  end = start+mqttSubscribeTopicLen;
  for(int i = start, j = 0; i < end; i++, j++)
    EEPROM.write(i, int(tSubscribeTopic[j]));

  EEPROM.write(912, mqttClientIdLen);
  EEPROM.write(913, mqttBrokerIpLen);
  EEPROM.write(914, mqttPortLen);
  EEPROM.write(915, mqttUsernameLen);
  EEPROM.write(916, mqttPassLen);
  EEPROM.write(917, mqttPublishTopicLen);
  EEPROM.write(918, mqttSubscribeTopicLen);

  EEPROM.commit();
}

void readMqttSettings() {
  clearMqttSettingsVariables();

  mqttClientIdLen = EEPROM.read(912);
  mqttBrokerIpLen = EEPROM.read(913);
  mqttPortLen = EEPROM.read(914);
  mqttUsernameLen = EEPROM.read(915);
  mqttPassLen = EEPROM.read(916);
  mqttPublishTopicLen = EEPROM.read(917);
  mqttSubscribeTopicLen = EEPROM.read(918);
  
  start = ssidLen+passLen+portLen+localIpLen+gatewayLen+subnetLen+dns1Len+dns2Len+serverUsernameLen+serverPassLen+serverLocalDomainNameLen+serverPortLen+2;
  end = start+mqttClientIdLen;
  for (int i = start; i < end; ++i)
    mqttClientId += char(EEPROM.read(i));

  start = end;
  end = start+mqttBrokerIpLen;
  for (int i = start; i < end; ++i)
    mqttBrokerIp += char(EEPROM.read(i));
  
  start = end;
  end = start+mqttPortLen;
  for(int i = start; i < end; ++i)
    mqttPort += char(EEPROM.read(i));

  start = end;
  end = start+1;
  mqttAuth = bool(EEPROM.read(start));

  start = end;
  end = start+mqttUsernameLen;
  for(int i = start; i < end; ++i)
    mqttUsername += char(EEPROM.read(i));

  start = end;
  end = start+mqttPassLen;
  for(int i = start; i < end; ++i)
    mqttPass += char(EEPROM.read(i));

  start = end;
  end = start+mqttPublishTopicLen;
  for(int i = start; i < end; ++i)
    mqttPublishTopic += char(EEPROM.read(i));

  start = end;
  end = start+mqttSubscribeTopicLen;
  for(int i = start; i < end; ++i)
    mqttSubscribeTopic += char(EEPROM.read(i));
}

void showMqttSettings() {
  Serial.print("\n\nSaved MQTT Settings\n");
  Serial.println("===========================");

  Serial.print("Client ID: ");
  Serial.println(mqttClientId);

  Serial.print("Broker IP: ");
  Serial.println(mqttBrokerIp);

  Serial.print("MQTT Port: ");
  Serial.println(mqttPort);

  Serial.print("Authentication: ");
  Serial.println(mqttAuth ? "on" : "off");

  Serial.print("Username: ");
  Serial.println(mqttUsername);

  Serial.print("Password: ");
  Serial.println(mqttPass);

  Serial.print("Publish Topics: ");
  Serial.println(mqttPublishTopic);

  Serial.print("Subscribe Topics: ");
  Serial.println(mqttSubscribeTopic);

  Serial.print("\n");
}


bool testWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  Serial.print("Waiting for Wifi to connect '");
  Serial.print(ssid);
  Serial.println("'");
  
  int c = 0;
  Serial.println();
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
      return true;
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("\n\n");
  Serial.println("Connect timed out, opening AP");
  return false;
}

bool wifiConfig(IPAddress tLocalIp, IPAddress tGateway, IPAddress tSubnet, IPAddress tDns1, IPAddress tDns2) {
  WiFi.begin(ssid, pass);
  return WiFi.config(tLocalIp, tGateway, tSubnet, tDns1, tDns2) ? true : false;
}

void setupAP() {
  WiFi.softAP("ESP8266");
  Serial.print("\n\nAP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
}

void showNetworkInfo() {
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());

  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());

  Serial.print("DNS1: ");
  Serial.println(WiFi.dnsIP());

  Serial.print("DNS2: ");
  Serial.println(WiFi.dnsIP(1));
}

void readSsids() {
  // every 10 seconds this function calls from loop-function
  //  and reads available wifi and saves into ssids variable
  ssids = "";
  for(int i = 0; i < WiFi.scanNetworks(); i++) {
    ssids += WiFi.SSID(i);
    ssids += ": ";
    ssids += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    ssids += "\r\n";
  }
}

void mqttSetup() {
  _mqttBrokerIp.fromString(mqttBrokerIp);
  mqttClient.setServer(_mqttBrokerIp, mqttPort.toInt());
  mqttClient.setCallback(mqttCallback);
  flag = true;
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  String message;
  for (int i = 0; i < length; i++) {
      message = message + (char) payload[i];  // convert *byte to string
  }
  Serial.print(message);
  if (message == "on") { digitalWrite(LED_BUILTIN, 0); }   // LED on
  if (message == "off") { digitalWrite(LED_BUILTIN, 1); } // LED off
  Serial.println();
  Serial.println("-----------------------");
}

void mqttConnect() {
  if(!mqttClient.connected()) {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    Serial.println("Connecting to mosquitto broker.....");
    if (mqttClient.connect(client_id.c_str(), mqttUsername.c_str(), mqttPass.c_str())) {
      Serial.println("Public mqtt broker connected");
      // publish and subscribe
      mqttClient.publish(mqttPublishTopic.c_str(), "Hello Mosquitto.");
      mqttClient.subscribe(mqttSubscribeTopic.c_str());
    } else {
      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      Serial.print(" - ");
      delay(2000);
    }
  }
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Page Not found");
}

void createWebServer() 
{
  server = new AsyncWebServer(port.toInt());

  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(serverAuth && !request->authenticate(serverUsername.c_str(), serverPass.c_str()))
      request->requestAuthentication();
    request->send(LittleFS, "/index.html");
  });

  server->on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", ssids.c_str());
  });

  server->on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
    if(serverAuth && !request->authenticate(serverUsername.c_str(), serverPass.c_str()))
      request->requestAuthentication();
    request->send(LittleFS, "/wifi.html");
  });

  server->on("/network", HTTP_GET, [](AsyncWebServerRequest *request){
    if(serverAuth && !request->authenticate(serverUsername.c_str(), serverPass.c_str()))
      request->requestAuthentication();
    request->send(LittleFS, "/network-settings.html");    
  });

  server->on("/server", HTTP_GET, [](AsyncWebServerRequest *request){
    if(serverAuth && !request->authenticate(serverUsername.c_str(), serverPass.c_str()))
      request->requestAuthentication();
    request->send(LittleFS, "/server-settings.html");    
  });

  server->on("/mqtt", HTTP_GET, [](AsyncWebServerRequest *request){
    if(serverAuth && !request->authenticate(serverUsername.c_str(), serverPass.c_str()))
      request->requestAuthentication();
    request->send(LittleFS, "/mqtt-settings.html");    
  });

  server->on("/logout", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(401);
  });

  server->on("/get1", HTTP_POST, [](AsyncWebServerRequest *request){      
    int params = request->params();
    String creditionals [] = {"", "", ""};
    for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        creditionals[i] = p->value().c_str();
    }

    saveCredentials(creditionals[0], creditionals[1], creditionals[2]);
    readCredentials();
    showCredentials();

    request->send(LittleFS, "/index.html");

    ESP.restart();
  });

  server->on("/get2", HTTP_POST, [](AsyncWebServerRequest *request){      
    int params = request->params();
    String settings [] = {"", "", "", "", ""};

    if(params == 5) {
      dhcp = 0;

      for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        settings[i] = p->value().c_str();
      }

      _localIp.fromString(settings[0]);
      _gateway.fromString(settings[1]);
      _subnet.fromString(settings[2]);
      _dns1.fromString(settings[3]);
      _dns2.fromString(settings[4]);

      saveNetworkSettings(dhcp ,settings[0], settings[1], settings[2], settings[3], settings[4]);
    }
    else {
      dhcp = 1;
      saveNetworkSettings(dhcp ,"(IP unset)", "(IP unset)", "(IP unset)", "(IP unset)", "(IP unset)");
    }
    
    readNetworkSettings();
    showNetworkSettings();

    request->send(LittleFS, "/index.html"); 
  });

  server->on("/get3", HTTP_POST, [](AsyncWebServerRequest *request){
    int params = request->params();
    String settings [] = {"", "", "", "", ""};
    if(params == 5) {
      serverAuth = 1;
      for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        settings[i] = p->value().c_str();
      }
    } else {
      serverAuth = 0;
      settings[0] = "off";
      settings[1] = "";
      settings[2] = "";
      for(int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        settings[i+3] = p->value().c_str();
      }
    }

    saveServerSettings(serverAuth, settings[1], settings[2], settings[3], settings[4]);
    readServerSettings();
    showServerSettings();

    request->send(LittleFS, "/index.html"); 
  });

  server->on("/get4", HTTP_POST, [](AsyncWebServerRequest *request){
    int params = request->params();
    String settings [] = {"", "", "", "", "", "", "", ""};

    if(params == 8) {
      mqttAuth = 1;
      for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        settings[i] = p->value().c_str();
      }
    }
    else {
      for(int i = 0; i < params-2; i++) {
        AsyncWebParameter *p = request->getParam(i);
        settings[i] = p->value().c_str();
      }
      mqttAuth = 0;
      settings[3] = "off";
      settings[4] = "";
      settings[5] = "";
      for(int i = params-2; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        settings[i+3] = p->value().c_str();
      }
    }

    saveMqttSettings(settings[0], settings[1], settings[2], mqttAuth, settings[4], settings[5], settings[6], settings[7]);
    readMqttSettings();
    showMqttSettings();

    request->send(LittleFS, "/index.html");
  });

  server->onNotFound(notFound);

  server->begin(); 
}