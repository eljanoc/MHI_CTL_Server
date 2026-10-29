char MainHtml[] PROGMEM = R"====(
<!DOCTYPE html>
<html lang="fr">

<head>
  <link rel="icon" href="data:,">
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width">
  <title>Une page pour papa</title>
  <style>
    body {
      background-color: rgb(30, 61, 86);
      font-family: Arial;
    }
    #LED{position:absolute;top:4px;left:4px;width:0px;height:0px;border:5px solid red;border-radius:5px;}
    .nice-btn {
      background-color: #4CAF50;
      /* Green */
      border: none;
      color: white;
      padding: 15px 15px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 16px;
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
<body onload='LoadData();LoadTraceData();'>
  <div id='LED'></div>
  <table style="text-align:center;width:290px;margin:auto;margin-top:0px; color: white; font-size:2rem;">
    <tbody>
      <tr>
        <td><div id='titre' style="margin-left:23px;"></div></td>
        <td><div id="logo" style="margin-left:15px;">&epsilon;lj</div></td>
      </tr>
    </tbody>
  </table>
  
  <table style="text-align:center;width:200px;margin:auto;color: white;margin-top: 15px;">
    <tbody>
      <tr>
        <td style="text-align:justify" ;>Temp. int.</td>
        <td>
          <div id='TRoom'>
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>MQTT</td>
        <td>
          <div id='MQTT'>
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>Temp. ext.</td>
        <td>
          <div id='TExt'>
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>Puissance</td>
        <td>
          <div id='Puissance'>
        </td>
      </tr>
     </tbody>
  </table>

  <table style="text-align:center;width:200px;margin:auto;color: white;margin-top: 15px;">
    <tbody>
      <tr>
        <td style="text-align:justify" ;>Mode</td>
        <td>
          <div style="color:white;backgroundColor:black;" id='statusMode'>
        </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>Consigne</td>
        <td>
          <div id='statusConsigne' °C</td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>Fan</td>
        <td>
          <div id='statusFan' </td>
      </tr>
      <tr>
        <td style="text-align:justify" ;>Volet</td>
        <td>
          <div id='statusVolet' </td>
      </tr>
    </tbody>
  </table>
  <div style="text-align:left;margin:15px;color:red;font-size:1.5rem;margin-top:15px;" id="restart"></div>
  <form style="text-align:center;margin-top:20px;">
    <input type="button" onclick='onoff();' id="buttonPower"
      style="font-size:2rem;font-weight: bold;background-color:#4CAF50;width:200px;height:60px;text-align:center;color: white"
      value="On/off" </button>>
  </form>

  <table style="text-align: center;margin:auto; color: white; font-size: 1rem; margin-top: 10px;max-width: 100%;">
    <tbody>
      <tr style="height:40px;">
        <td style="text-align:justify" ;>Set Mode</td>
        <td><label for="set-mode"></label>
          <select name="Mode" onchange='SendValues(id,value);' id="set-mode" ,style="width:100px;">
            <option value="1">Auto</option>
            <option value="2">Dry</option>
            <option value="3">Cool</option>
            <option value="4">Fan</option>
            <option value="5">Heat</option>
          </select>
        </td>
      </tr>
      <tr style="height:40px;">
        <td style="text-align:justify" ;>Set température</td>
        <td><label for="set-temp"></label>
          <select name="Tsetpoint" onchange='SendValues(id,value);' id='set-temp' ,style="width:100px;">
            <option value="1">18°C</option>
            <option value="2">19°C</option>
            <option value="3">20°C</option>
            <option value="4">21°C</option>
            <option value="5">22°C</option>
            <option value="6">23°C</option>
            <option value="7">24°C</option>
            <option value="8">25°C</option>
            <option value="9">26°C</option>
            <option value="10">27°C</option>
            <option value="11">28°C</option>
            <option value="12">29°C</option>
          </select>
        </td>
      </tr>
      <tr style="height:40px;">
        <td style="text-align:justify" ;>Set Fan</td>
        <td><label for="set-fan"></label>
          <select name="Fan" onchange='SendValues(id,value);' id="set-fan" ,style="width:100px;">
            <option value="1">Auto</option>
            <option value="2">1</option>
            <option value="3">2</option>
            <option value="4">3</option>
            <option value="5">4</option>
          </select>
        </td>
      </tr>
      <tr style="height:40px;">
        <td style="text-align:justify" ;>Set Volet</td>
        <td><label for="set-vanes"></label>
          <select name="Vanes" onchange='SendValues(id,value);' id="set-vanes" ,style="width:100px;">
            <option value="1">Swing</option>
            <option value="2">1</option>
            <option value="3">2</option>
            <option value="4">3</option>
            <option value="5">4</option>
          </select>
        </td>
      </tr>
    </tbody>
  </table>
  <section style="margin:15px auto;max-width:420px;color:white;">
    <h3 style="margin-bottom:8px;">Détection des trames <input type="checkbox" id="traceCheck" onclick="toggleTrace();" style="margin-left:8px;"></h3>
    <span id="traceStatus" style="display:none;"></span>
    <pre id="traceFrames" style="display:none;box-sizing:border-box;max-height:220px;overflow:auto;padding:8px;background:#172b3a;color:#c9f7c9;font-size:0.75rem;white-space:pre-wrap;word-break:break-all;">Aucune trame capturée</pre>
  </section>
  <div style="text-align:left;margin:15px;color:white;font-size:0.7rem;margin-top:15px;" id="version"></div>
  <table style="margin-top:10px;">
    <tr>
      <td><button style="margin-left:20px;"onclick='restart("restart")' ;>restart</button></td>
      <td><button style="margin-left:15px;"onclick='restart("reinit")' ;>reinit</button></td>
      <!--td><p style="margin-left:15px;text_color:white;"><a href="param.html" style="color:#0010FF;">Param&egrave;tres</a></p></td--!>
      <td><button style="margin-left:15px;"onclick='paramPage()' ;>parametres</button></td>
    </tr>
  </table>
  <div style="text-align:left;margin-left:8px;font-size:0.8rem;color: rgb(233, 75, 75);margin-top:10px;" id="Warning"></div>
  <table style="text-align: center;margin:auto;font-size: 1rem; margin-top: 10px;max-width: 100%;">
    <td><a style="font-size:1.2rem;background-color:#235161;width:100px;height:40px;
      text-align:center;color: white;" id='urlSalon'>Salon</a>
    </td>
    <td><a style="font-size:1.2rem;margin-left: 20px; background-color:#235161;width:100px;height:40px;
    text-align:center;color: white;" id='urlChNO'>ChNO</a>
    </td>
    <td><a style="font-size:1.2rem;margin-left: 20px;background-color:#235161;width:100px;height:40px;
  text-align:center;color: white;" id='urlCuisine'>Cuisine</a>
    </td>
  </table>
  <script src='MainJS'></script>
</body>
</html>
)====";


const  char MainJS[] PROGMEM= R"====(
var SIMUL = false;

function paramPage ()
{
  document.location.assign("/param.html");
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
      document.getElementById('restart').innerHTML = "Restarting...";
      location.reload();
    };
  }
}
function parametres() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      var retour = this.responseText;
      document.getElementById('restart').innerHTML = "Restarting...";
      setTimeout('location.reload();', 2000);
    };
  }
  xhttp.open('POST', "Param", true);
  xhttp.send();
  xhttp.onerror=function(error){
    document.getElementById('restart').innerHTML = "Restarting...";
    location.reload();
  };
}
var simulMHI = "Deb?Salon?Cool?22°C?Auto?3?23.5°C?22.5°C?0??Fin";
var MHImode = "Cool", MHIvanes = "2", MHIfan = "1", MHItemp = "21°C";
function loadSimulMHI(id, value) {
  if (id == "set-mode")
    MHImode = value;
  else if (id == "set-vanes")
    MHIvanes = value;
  else if (id == "set-fan")
    MHIfan = value;
  else if (id == "set-temp")
    MHItemp = value + "°C";
  simulMHI = "Deb?Salon?" + MHImode + "?" + MHItemp + "?" + MHIfan + "?" + MHIvanes +
    "?23.5°C?17.5°C?0?Warning:ça marche?Fin";
}

function onoff() {
  document.getElementById('buttonPower').style.backgroundColor = "#dfe2dd"; //set gris waiting state from webServer

  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      var retour = this.responseText;
    }
  };
  if (!document.getElementById('buttonPower').checked)
    xhttp.open('POST', "SetValue?Power=On", true);
  else
    xhttp.open('POST', "SetValue?Power=Off", true);
  xhttp.send();
}

function blackFrame(dac) {
  dac.style.backgroundColor = "black";
}

function greenFrame(topic, message) {
  doc = document.getElementById(topic);
  if (doc.innerHTML.localeCompare(message)) {
    doc.style.backgroundColor = "green";
    doc.innerHTML = message;
    setTimeout(function (dac) { dac.style.backgroundColor = "black"; }.bind(null, doc), 500);
  }
  if (topic == "statusMode")
    document.getElementById('buttonPower').value = message;
}

function loadValues(MHI) {
  var message = MHI.split('?');

  document.getElementById('titre').innerHTML = "Climatiseur&nbsp;"+message[1];
  greenFrame('statusMode', message[2]);
  var index = getIndexFromOptionString('set-mode', message[2]);
  greenFrame('statusConsigne', message[3]);
  index = getIndexFromOptionString('set-temp', message[3]);
  greenFrame('statusFan', message[4]);
  index = getIndexFromOptionString('set-fan', message[4]);
  greenFrame('statusVolet', message[5]);
  index = getIndexFromOptionString('set-vanes', message[5]);
  document.getElementById('TRoom').innerHTML = message[6];//greenFrame('TRoom', message[6]);
  document.getElementById('TExt').innerHTML = message[7];//greenFrame('TExt', message[7]);
  document.getElementById('Warning').innerHTML = message[9];
  document.getElementById('version').innerHTML = message[10];
  document.getElementById('Puissance').innerHTML = message[11];
document.getElementById('MQTT').innerHTML = message[15];
  if (message[12].length!=0)
    document.getElementById('urlSalon').href="http://"+message[12];
  if (message[13].length!=0)
    document.getElementById('urlChNO').href="http://"+message[13];
  if (message[14].length!=0)
    document.getElementById('urlCuisine').href="http://"+message[14];

  var doc = document.getElementById('buttonPower');
  if (message[8] == "1") {
    doc.checked = true;
    doc.style.backgroundColor = "#d42a2a";//rouge     
    document.getElementById('buttonPower').value = message[2] + '(on)';
  }
  else {
    doc.checked = false;
    doc.style.backgroundColor = "#739464";//vert
    document.getElementById('buttonPower').value = message[2] + '(off)';
  }
}

function LoadData() {
//  <div name="login" id="1234567890"></div><div name="passwd" id="passw1234567890"></div>
//  var id=document.getElementById('login').innerHTML;
//  var psw=document.getElementById('passwd').innerHTML;

  document.getElementById('LED').style='display:block;';
  if (!SIMUL) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        var MHI = this.responseText;
        loadValues(MHI);
        document.getElementById('LED').style='display:none;';
        setTimeout('LoadData();', 2000);
      }
    };
    xhttp.open('GET', 'ajax_data', true);
    xhttp.send();
    xhttp.onerror=function(error){
      document.getElementById('restart').innerHTML = "Restarting...";
      location.reload();
    };
  }
  else {
    loadValues(simulMHI);

    /*            document.getElementById('LED').style='display:none;';*/
    setTimeout('LoadData();', 1000);
  }
}

var traceWanted = false;
var traceWantedUntil = 0;

function LoadTraceData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
      var enabled = Date.now() < traceWantedUntil ? traceWanted : data.enabled;
      document.getElementById('traceCheck').checked = enabled;
      var status = document.getElementById('traceStatus');
      var frames = document.getElementById('traceFrames');
      status.style.display = enabled ? '' : 'none';
      frames.style.display = enabled ? '' : 'none';
      status.innerHTML = "Capture active — trames perdues : " + data.dropped;
      frames.textContent = data.frames.length
        ? data.frames.map(function (frame, index) {
            return (index + 1) + " RX: " + frame + (data.miso ? "\n   TX: " + data.miso[index] : "");
          }).join("\n")
        : "Aucune trame capturée";
    }
    if (this.readyState == 4)
      setTimeout(LoadTraceData, 1000);
  };
  xhttp.open('GET', 'trace_data', true);
  xhttp.send();
}

function toggleTrace() {
  var enable = document.getElementById('traceCheck').checked;
  traceWanted = enable;
  traceWantedUntil = Date.now() + 4000;
  document.getElementById('traceStatus').style.display = enable ? '' : 'none';
  document.getElementById('traceFrames').style.display = enable ? '' : 'none';
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status != 200) {
      document.getElementById('traceStatus').style.display = '';
      document.getElementById('traceStatus').innerHTML = "Erreur de commande (" + this.status + ")";
    }
  };
  xhttp.open('POST', 'trace?enable=' + (enable ? "1" : "0"), true);
  xhttp.send();
}

function int2ip(V) {
  var ipInt = parseInt(V);
  return ((ipInt >>> 24) + '.' + (ipInt >> 16 & 255) + '.' + (ipInt >> 8 & 255) + '.' + (ipInt & 255));
}

function SendValues(id, value) {
  var doc = document.getElementById(id);
  var name = doc.name;
  var option = doc.innerHTML;
  //  <option value="1">Swing</option> <option value="2">1</option> <option value="3">2</option> <option value="4">3</option> <option value="5">4</option>

  var ss = "<option value=\"" + value + "\">";
  var start = option.indexOf(ss);
  if (start != -1) {
    start += ss.length;
    var end = option.indexOf("<", start);
    var textValue = option.slice(start, end);
    if (name == "Tsetpoint")
      textValue = textValue.slice(0, 2);
  }
  var S = "SetValue?" + name + "=" + textValue;
  if (SIMUL)
    loadSimulMHI(id, textValue);

  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      var retour = this.responseText;
    }
  };
  xhttp.open('POST', S, true);
  xhttp.send();
}

function onMode(id, value) {
  doc = document.getElementById('buttonPower').value = value;
  SendValues(id, value);
}

//  <option value="1">Swing</option> <option value="2">1</option> <option value="3">2</option> <option value="4">3</option> <option value="5">4</option>
function getIndexFromOptionString(id, OptStr) {
  var doc = document.getElementById(id);
  var option = doc.innerHTML;
  var Str = option.split("<option value=\"");
  var element;
  for (var i = 0; i < Str.length; i++) {
    element = Str[i].split("\">");
    if (element.length == 2) {
      if (element[1].indexOf(OptStr + '<') == 0) {
        doc.value = element[0];
        return element[0];
      }
    }
  }
  return -1;
}
)====";
