#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define USE_SERIAL Serial

static const char ssid[] = "front_gate";
static const char password[] = "";
MDNSResponder mdns;

static void writeEsc(bool);

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>Front Gate Controller</title>
<style>
"body { background-color: #74A5EE; font-family: Arial, Helvetica, Sans-Serif; Color: #FF6F00; }"
"h1 {text-align:center; Color: #FF6F00;}"
p {text-align:center;}
</style>
<script>
var websock;
function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) { console.log('websock open'); };
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
    console.log(evt);
    var e = document.getElementById('ledstatus');
    if (evt.data === 'gateOpen') {
      e.style.color = 'red';
    }
    else if (evt.data === 'gateClose') {
      e.style.color = 'green';
    }
    else {
      console.log('unknown event');
    }
  };
}
function buttonclick(e) {
  websock.send(e.id);
}
</script>
</head>
<body onload="javascript:start();">
<table cellpadding="10%" cellspacing="10%" align="center" bgcolor="#74A5EE" width="100%" style="text-align: center; ">
<tr>
<td><span align="centre"><h1>W & J Randall</h1></span></td>
</tr>
<tr>
<td><span align="centre"><h1>Gate Controller</h1></span></td>
</tr>
<tr>
<td><div class="centre" id="ledstatus"><h1>Gate</h1></div></td>
</tr>
<tr>
<td><div class="centre"><button id="gateOpen"  type="button" onclick="buttonclick(this);"><h1>-OPEN-</h1></button></div></td>
</tr>
<tr>
<td><centre><button id="gateClose" type="button" onclick="buttonclick(this);"><h1>-CLOSE-</h1></button></centre></td>
</tr>
</table>
</body>
</html>
)rawliteral";


const int in1 = 4;//Esc in1 pin
const int in2 = 5;//Esc in2 pin
const int closedLimitSwitch = 14;// limit switch for closed position
const int openLimitSwitch = 12;//Limit switch for open position
bool gateStatus;// Current gate status

// Commands sent through Web Socket
const char OPEN[] = "gateOpen";
const char CLOSE[] = "gateClose";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  USE_SERIAL.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch (type) {
  case WStype_DISCONNECTED:
    USE_SERIAL.printf("[%u] Disconnected!\r\n", num);
    break;
  case WStype_CONNECTED:
  {
               IPAddress ip = webSocket.remoteIP(num);
               USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
               // Send the current LED status
               if (gateStatus) {
                 webSocket.sendTXT(num, OPEN, strlen(OPEN));
               }
               else {
                 webSocket.sendTXT(num, CLOSE, strlen(CLOSE));
               }
  }
    break;
  case WStype_TEXT:
    USE_SERIAL.printf("[%u] get Text: %s\r\n", num, payload);

    if (strcmp(OPEN, (const char *)payload) == 0) {
      writeEsc(true);
    }
    else if (strcmp(CLOSE, (const char *)payload) == 0) {
      writeEsc(false);
    }
    else {
      USE_SERIAL.println("Unknown command");
    }
    // send data to all connected clients
    webSocket.broadcastTXT(payload, length);
    break;
  case WStype_BIN:
    USE_SERIAL.printf("[%u] get binary length: %u\r\n", num, length);
    hexdump(payload, length);

    // echo data back to browser
    webSocket.sendBIN(num, payload, length);
    break;
  default:
    USE_SERIAL.printf("Invalid WStype [%d]\r\n", type);
    break;
  }
}

void handleRoot()
{
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

static void writeEsc(bool openGate)
{
  gateStatus = openGate;
  if (openGate) {
    if (digitalRead(openLimitSwitch)==1){
      digitalWrite(in1, 0);
      digitalWrite(in2, 1);
    }
    else {
    digitalWrite(in1, 0);
    digitalWrite(in2, 0);

  }
  }
  else {
    if (digitalRead(closedLimitSwitch)==1){
     digitalWrite(in1, 1);
     digitalWrite(in2, 0);
    }
    else {
      digitalWrite(in2, 0);
      digitalWrite(in1, 0);
    }
  }
}

void setup()
{
  pinMode(in1, OUTPUT);
  digitalWrite(in1, 0);
  pinMode(in2, OUTPUT);
  digitalWrite(in2, 0);
  writeEsc(false);
  pinMode(closedLimitSwitch, INPUT_PULLUP);
  pinMode(openLimitSwitch, INPUT_PULLUP);


  USE_SERIAL.begin(115200);

  //Serial.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

//  WiFiMulti.addAP(ssid, password);
//
//  while (WiFiMulti.run() != WL_CONNECTED) {
//    Serial.print(".");
//    delay(100);
//  }

  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  USE_SERIAL.print("AP IP address: ");
  USE_SERIAL.println(myIP);

  USE_SERIAL.println("");
  USE_SERIAL.print("Connected to ");
  USE_SERIAL.println(ssid);
  USE_SERIAL.print("IP address: ");
  USE_SERIAL.println(WiFi.localIP());

  if (mdns.begin("espWebSock", WiFi.localIP())) {
    USE_SERIAL.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);
  }
  else {
    USE_SERIAL.println("MDNS.begin failed");
  }
  USE_SERIAL.print("Connect to http://espWebSock.local or http://");
  USE_SERIAL.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  webSocket.loop();
  server.handleClient();
}

