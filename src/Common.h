/*! \file common.h
 *  \brief Enter description here.
 *  \author Georgi Gerganov
 */

#pragma once

const char * kPageIndex = R"PageIndex(
<html>
    <head>
      <script language="javascript" type="text/javascript">
          var output;
          var output_number;

          function init()
          {
              output = document.getElementById("output");
              output_number = document.getElementById("output_number");

              testWebSocket();
          }

          function testWebSocket()
          {
              var wsUri = "ws://localhost:3000/data/test";

              websocket = new WebSocket(wsUri);
              websocket.onopen = function(evt) { onOpen(evt) };
              websocket.onclose = function(evt) { onClose(evt) };
              websocket.onmessage = function(evt) { onMessage(evt); };
              websocket.onerror = function(evt) { onError(evt) };
          }

          function onOpen(evt)
          {
              writeToScreen("CONNECTED", output);
              doSend("WebSocket rocks");
          }

          function onClose(evt)
          {
              writeToScreen("DISCONNECTED", output);
          }

          function onMessage(evt)
          {
              if (typeof evt.data === 'string' && evt.data.substring(0, 6) === "Random") {
                  writeToScreen('<span style="color: green;">RESPONSE: ' + evt.data+'</span>', output_number);
              } else {
                  writeToScreen('<span style="color: blue;">RESPONSE: ' + evt.data+'</span>', output);
              }
          }

          function onError(evt)
          {
              writeToScreen('<span style="color: red;">ERROR:</span> ' + evt.data, output);
          }

          function doSend(message)
          {
              writeToScreen("SENT: " + message, output);
              websocket.send(message);
          }

          function writeToScreen(message, o)
          {
              pre = document.createElement("p");
              pre.style.wordWrap = "break-word";
              pre.innerHTML = message;

              if (o.childNodes.length > 5) {
                  o.removeChild(o.childNodes[0]);
              }
              o.appendChild(pre);
          }

          window.addEventListener("load", init, false);

function vr_send() {
    doSend(document.getElementById("data_to_send").value);
}

function vr_send_binary() {
    const array = new Float32Array(5);

    for (var i = 0; i < array.length; ++i) {
        array[i] = i / 2;
    }
    doSend(array);
}
</script>
</head>

<body>
<h2>WebSocket Test</h2>

<input type="text" id="data_to_send" value="hello"></input>
<button id="sender" onClick="vr_send()">Send</button>
<button id="sender_binary" onClick="vr_send_binary()">Send Binary</button>

<div id="output"></div>
<div id="output_number"></div>
</body>
</html>
)PageIndex";
