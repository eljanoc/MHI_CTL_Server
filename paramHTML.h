char ParamHtml[] PROGMEM = R"====(
<!DOCTYPE html>
<html lang="fr">

<head>
  <link rel="icon" href="data:,">
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width">
  <title>MHI Clim</title>
  <style>
    body {
      background-color: rgb(30, 61, 86);
      font-family: Arial;
    }
#logo {
    text-align:center;
    font-size:1rem;
    color:white;
    width:25px;
    height: 20px;
    border-style:solid;
    border-radius: 5px;
    background-color: rgb(78, 39, 185);
  }
   </style>
</head>
<body onload='LoadData();'>
   <table style="text-align:center;width:290px;margin:auto;margin-top:0px; color: white; font-size:2rem;">
    <tbody>
      <tr>
        <td><div id='titre' style="margin-left:23px;">Param&egrave;tres</div></td>
        <td><div id="logo" style="margin-left:15px;">&epsilon;lj</div></td>
      </tr>
    </tbody>
  </table>
  <div id="ver" style="text-align:center;color:white;font-size:0.9rem;margin-top:4px;"></div>
  
  <table style="text-align:left;width:300px;margin:auto;color: white;margin-top: 15px;">
    <tbody>
      <tr>
        <td style="text-align:justify" ;>SSID:</td>
        <td><label for="ssid1"></label>
            <input id="ssid3" style="font-weight: bold;background-color: rgb(189, 202, 238)" name="ssidName" size="13">
            <select id="ssid2" onchange="SetSSID()" style="width:24px;"></select>
        </td>        
        <!--td>
          <input type="text" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="ssid" name="ssid" size="20" />
        </td--!>
      </tr>
      <!--tr>
        <td style="text-align:justify" ;>SSID:</td>
          <td><label for="ssid"></label>
          <select name="ssid" onchange='SetSSID();' id='ssid' ,style="width:100px;">
          </select>
        </td>        
        <!--td>
          <input type="text" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="ssid" name="ssid" size="20" />
        </td--!>
      </tr--!>
      <tr>
        <td style="text-align:justify" ;>Password</td>
        <td>
          <input type="password" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="password" name="password" size="13" />
          <input type="checkbox" onclick="myFunction()">
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>static IP:</td>
        <td>
          <input type="text" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="staticIP" name="staticIP" size="13" />
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>Masque IP</td>
        <td>
          <input type="text" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="masqIP" name="masqIP" size="13" />
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>Gateway IP</td>
        <td>
          <input type="text" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="gatewayIP" name="gatewayIP" size="13" />
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>DNS IP</td>
        <td>
          <input type="text" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="dnsIP" name="dnsIP" size="13" />
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>MQTT actif</td>
        <td>
          <input type="checkbox" id="mqttEnabled"           name="mqttEnabled" onchange="showMqtt()" checked />
        </td>
      </tr>
      <tr class="mqttRow">
        <td style="text-align:justify" ;>MQTT IP</td>
        <td>
          <input type="text" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="mqttIP" name="mqttIP" size="13" />
        </td>
      </tr>
      <tr class="mqttRow">
        <td style="text-align:justify" ;>MQTT port</td>
        <td>
          <input type="text" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="mqttPort" name="mqttPort" size="4" />
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>WiFi Timeout</td>
        <td>
          <input type="text" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="wifiTimeout" name="wifiTimeout" size="2" />
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>Room</td>
        <td>
          <input type="text" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="room" name="room" size="13" />
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>Thing</td>
        <td>
          <input type="text" style="font-weight: bold;background-color: rgb(189, 202, 238)" id="thing" name="thing" size="13" />
        </td>
      </tr>
     </tbody>
  </table>
  <div style="text-align:left;margin:15px;color:red;font-size:1.5rem;margin-top:15px;" id="restart"></div>

  <table style="margin-top:10px;">
    <tr>
      <td><button style="margin-left:15px;"onclick='restart("restart")' ;>restart</button></td>
      <td><button style="margin-left:15px;"onclick='restart("reinit")' ;>reinit</button></td>
      <td><button style="margin-left:15px;"onclick='saveParam()' ;>save Param</button></td>
      <!--td><p style="margin-left:15px;text_color:white;"><a href="/" style="color:#0010FF;">Main Page</a></p></td--!>
      <td><button style="margin-left:15px;"onclick='loadPage()' ;>Main Page</button></td>
    </tr>
  </table>
  <div style="text-align:left;margin:15px;color:red;font-size:1.5rem;margin-top:15px;" id="restart"></div>
  <script src='ParamJS'></script>
</body>
</html>
)====";


const  char ParamJS[] PROGMEM= R"====(
var SIMUL = false;
var saveSSID='';
var savePass='';
var initialParameterValues = null;
var saveInProgress = false;

function getParameterValues() {
  return [
    document.getElementById('ssid3').value,
    document.getElementById('password').value,
    document.getElementById('staticIP').value,
    document.getElementById('masqIP').value,
    document.getElementById('gatewayIP').value,
    document.getElementById('dnsIP').value,
    document.getElementById('mqttIP').value,
    document.getElementById('mqttPort').value,
    document.getElementById('wifiTimeout').value,
    document.getElementById('room').value,
    document.getElementById('thing').value,
    document.getElementById('mqttEnabled').checked
  ];
}

function hasUnsavedChanges() {
  if (!initialParameterValues) return false;
  var currentValues = getParameterValues();
  for (var i = 0; i < currentValues.length; i++) {
    if (currentValues[i] !== initialParameterValues[i]) return true;
  }
  return false;
}

window.addEventListener('beforeunload', function (event) {
  if (!saveInProgress && hasUnsavedChanges()) {
    event.preventDefault();
    event.returnValue = '';
  }
});

function myFunction() {
  var x = document.getElementById("password");
  if (x.type === "password") {
    x.type = "text";
  } else {
    x.type = "password";
  }
} 

function loadPage ()
{
  document.location.assign("/");
}

function SetSSID() {
  var sel = document.getElementById('ssid2');
  if (!sel.value) return;
  document.getElementById('ssid3').value = sel.value;
  document.getElementById('password').value = (sel.value == saveSSID) ? savePass : '';
  sel.selectedIndex = 0;
}

function restart(mode) {
  if (window.confirm(mode + " : êtes vous sur ??")) {
    document.getElementById('restart').innerHTML = "Restarting...";
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        var retour = this.responseText;
        setTimeout('location.reload();', 2000);
      };
    }
    xhttp.open('POST', mode, true);
    xhttp.send();
    xhttp.onerror=function(error){
      location.reload();
    };
  }
}

/*document.getElementById('ssid3').addEventListener('click', function() {
  let inputElement = document.getElementById('myInput');
  if (!inputElement.value) {
    inputElement.value = saveSSID;
  }
});*/


function showMqtt() {
  var on = document.getElementById('mqttEnabled').checked;
  var rows = document.getElementsByClassName('mqttRow');
  for (var i = 0; i < rows.length; i++) rows[i].style.display = on ? '' : 'none';
}
function LoadData() {
  if (!SIMUL) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        var message = this.responseText.split('?');
        document.getElementById('staticIP').value=message[0];
        document.getElementById('masqIP').value=message[1];
        document.getElementById('gatewayIP').value=message[2];
        document.getElementById('dnsIP').value=message[3];
        document.getElementById('mqttIP').value=message[4];
        document.getElementById('mqttPort').value=message[5];
        document.getElementById('wifiTimeout').value=message[6];
        document.getElementById('room').value=message[7];
        document.getElementById('thing').value=message[8];
        document.getElementById('mqttEnabled').checked=(message[9]!='0');
        showMqtt();
      
        var dynamicSelect = document.getElementById("ssid2");
        dynamicSelect.innerHTML = "";
        // première option vide : la flèche reste visible sans afficher de réseau sélectionné
        dynamicSelect.appendChild(document.createElement("option"));
        for (let i = 10; i + 2 < message.length; i += 3) {
          let opt = document.createElement("option");
          opt.value = message[i];
          opt.textContent = message[i] + " (" + message[i + 2] + " dBm)";
          dynamicSelect.appendChild(opt);
          if (message[i+1]!="")
          {
            document.getElementById('password').value=message[i+1];
            saveSSID=message[i];
            savePass=message[i+1];
            document.getElementById("ssid3").placeholder=message[i];
          }
        }
        initialParameterValues = getParameterValues();
      }
    };
    xhttp.open('GET', 'param_data', true);
    xhttp.send();
    var xv = new XMLHttpRequest();
    xv.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        var v = this.responseText.split('?')[10] || '';
        document.getElementById('ver').innerHTML = v.split('&emsp;')[0];
      }
    };
    xv.open('GET', 'ajax_data', true);
    xv.send();
/*    xhttp.onerror=function(error){
      document.getElementById('restart').innerHTML = "Restarting...";
      location.reload();
    };*/
  }
}

function saveParam() {
  if (window.confirm("save Param : êtes vous sur ??")) {
    val=document.getElementById("ssid3").value;

    saveInProgress = true;
    document.getElementById('restart').innerHTML = "Enregistrement...";
    var S='NewParam?param='+
    document.getElementById('staticIP').value+'?'+
    document.getElementById('masqIP').value+'?'+
    document.getElementById('gatewayIP').value+'?'+
    document.getElementById('dnsIP').value+'?'+
    document.getElementById('mqttIP').value+'?'+
    document.getElementById('mqttPort').value+'?'+
    document.getElementById('wifiTimeout').value+'?'+
    document.getElementById('room').value+'?'+
    document.getElementById('thing').value+'?'+
    (document.getElementById('mqttEnabled').checked ? '1' : '0')+'?'+
    document.getElementById("ssid3").value+'?'+
    document.getElementById("password").value+'?';

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4) {
        if (this.status == 200) {
          initialParameterValues = getParameterValues();
          document.getElementById('restart').innerHTML = "Restarting...";
          setTimeout('location.reload();', 2000);
        } else {
          saveInProgress = false;
          document.getElementById('restart').innerHTML = "Erreur lors de l'enregistrement des paramètres.";
        }
      }
    };
    xhttp.open('POST', S, true);
    xhttp.send();
    xhttp.onerror=function(){
      saveInProgress = false;
      document.getElementById('restart').innerHTML = "Erreur lors de l'enregistrement des paramètres.";
    };
  }
}

)====";
