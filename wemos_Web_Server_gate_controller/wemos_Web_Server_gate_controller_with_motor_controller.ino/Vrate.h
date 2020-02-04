
static const char PROGMEM Vrate_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">

<title>Seeder Controller</title>
<style>
body { background-color: lightgreen; font-family: Arial, Helvetica, Sans-Serif; Color: black; }
h1 { color: yellow;
text-align: centre;
}
p { font-family: verdana;
font-size: 20px;
}
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
    var e = document.getElementById('motorStatus');
    if (evt.data === 'motoron') {
      e.style.color = 'red';
    }
    else if (evt.data === 'motoroff') {
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
<h1>Seeder Controller</h1>
<div id="motorStatus"><b><h2>MOTOR</h2></b></div>
<button id="motoron"  type="button" onclick="buttonclick(this);"><h1>-On-</h1></button> 
<button id="motoroff" type="button" onclick="buttonclick(this);"><h1>-Off-</h1></button>
<div id="averageRpm"><b><h2>RPM</h2></b></div>

</body>
</html>
)rawliteral";

//----Current Motor Status------

bool motorStatus;

//-------- Commands sent through Web Socket----

const char motoron[] = "motoron";
const char motoroff[] = "motoroff";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // Send the current LED status
        if (motorStatus) {
          webSocket.sendTXT(num, motoron, strlen(motoron));
        }
        else {
          webSocket.sendTXT(num, motoroff, strlen(motoroff));
        }
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\r\n", num, payload);

      if (strcmp(motoron, (const char *)payload) == 0) {
       motorStatus = true;
       }
      else if (strcmp(motoroff, (const char *)payload) == 0) {
       motorStatus = false;
             
      }
      else {
        Serial.println("Unknown command");
      }
      // send data to all connected clients
      webSocket.broadcastTXT(payload, length);
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

