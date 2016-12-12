
/*
 * app logic
 *
 * dgorski - 11/22/2016
 *
 */
 
var VERSION = "0.95";

var currentView = 'home';
var previousView = 'home';

var showDoorTwo = false;
var timers = { };

var imgsrc = [ "garage-open.png", "garage-closed.png" ];

window.onload = function() {
  if(!localStorage.getItem('showDoorTwo')) {
    localStorage.setItem('showDoorTwo', 1); // show one door
  }
  // by default door two is hidden, if prefs say to show it, call the toggle function
  if(localStorage.getItem('showDoorTwo') > 1) {
    toggleDoorTwo();
  }

  // set up and destroy UI timers based on page visibility
  document.addEventListener("visibilitychange", function() { visibilityRefresh() });
  // call it once to set up the current state
  visibilityRefresh();
  
  // stash the remote config into local storage
  doGet("/api/config", function() {
    if(this.readyState != 4) return; // done done yet
    if(this.code != 200) return; // failed
    try {
      var cfg = JSON.parse(this.responseText);
      localStorage.setItem('deviceConfig', this.responseText);
    } catch (e) { }
  }, function() {} );
}

/**
 * enable and disable ajax data refreshes based on page visibility
 */
function visibilityRefresh() {
  if(document.visibilityState == "hidden") {
    clearInterval(timers["doorStates"]);
    // disable any timed background data gathering
  } else if(document.visibilityState == "visible") {
    // (re)enable timed background data gathering
    timers["doorStates"] = setInterval(refreshDoorStates, 8000);
    // and run one now
    //refreshDoorStates();
  }
}

/**
 * Show a view, hide the previous one.
 */
function openView(view) {
  document.getElementById(currentView).style.display = 'none';
  document.getElementById(view).style.display = 'block';
  previousView = currentView;
  currentView = view;
}

/**
 * open the home view
 */
function homeView() { openView('home'); }

/**
 * send an ajax GET request
 */
function doGet(uri, success_cb, error_cb) {
  if(success_cb === undefined) success_cb = function(){ };
  if(error_cb === undefined) error_cb = function(){
    openMessagePanel("Error", "<p>The operation failed, the device did not respond to our request</p>", true);
  };

  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = success_cb;
  xhttp.onerror = error_cb;

  xhttp.open("GET", uri, true);
  xhttp.send();
}

/**
 * send an ajax POST request
 */
function doPost(uri, data, success_cb, error_cb) {
  if(success_cb === undefined) success_cb = function(){ };
  if(error_cb === undefined) error_cb = function(){
    openMessagePanel("Error", "<p>The operation failed, the device did not respond to the request</p>", true);
  };

  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = success_cb;
  xhttp.onerror = error_cb;

  xhttp.open("POST", uri, true);
  if(data === undefined) {
    xhttp.send();
  } else {
    xhttp.send(data);
  }
}

/**
 * For ajax callbacks
 */
function openMessagePanel(title, message, homeAfter) {
  // by default, reset to the home view after displaying the message
  if(homeAfter === undefined) {
    setTimeout(homeView, 4500); // 4.5s
  }
  document.getElementById("messagesSpan").innerHTML = title;
  document.getElementById("messagesDiv").innerHTML = message;
  openView("messages");
}

/**
 * Parse a string as a JSON object, eat exceptions.
 * Always returns an object, if expr is not parsable, an empty object is returned.
 */
function parseJson(expr) {
  var result = {};
  try { result = JSON.parse(expr); } catch(e) { }
  return result;
}

/**
 * Get the current door states from the controller
 */
function refreshDoorStates() {
  //console.log("getting updated door states...");
  doGet("/api/doorStatus",
    function() {
      if(this.readyState != 4) return; // done done yet
      if(this.code != 200) return; // failed

      var states = JSON.parse(this.responseText);

      if(states.hasOwnProperty("doorOne")) {
        var state = states["doorOne"];
        var img = document.getElementById("doorOneDiv").getElementsByTagName("IMG")[0];
        if(state == "open") {
          img.src = imgsrc[0];
        } else if(state == "closed") {
          img.src = imgsrc[1];
        }
      }

      if(states.hasOwnProperty("doorTwo")) {
        var state = states["doorTwo"];
        var img = document.getElementById("doorTwoDiv").getElementsByTagName("IMG")[0];
        if(state == "open") {
          img.src = imgsrc[0];
        } else if(state == "closed") {
          img.src = imgsrc[1];
        }
      }
    },
    function(){} // ignore connection errors
  );
}

/**
 * Send a command to activate the first door
 */
function activateDoorOne() {
  console.log("activateDoorOne()");
  doPost("/api/door1");
}

/**
 * Send a command to activate the second door
 */
function activateDoorTwo() {
  console.log("activateDoorTwo()");
  doPost("/api/door2");
}

/**
 * Show or hide the second door view/control
 */
function toggleDoorTwo() {
  var door = document.getElementById("doorTwoDiv");
  var spacer = document.getElementById("doorSpacerDiv");
  var check = document.getElementById("doorTwoCheckMark");
  var cls = check.className;
  if(showDoorTwo) {
    console.log("toggleDoorTwo() - hiding door two");
    showDoorTwo = false;
    // hide second door div
    door.style.display = "none";
    spacer.style.display = "block";
    // change checkbox icon to -blank-
    check.className = cls.replace('mdi-checkbox-marked-outline', 'mdi-checkbox-blank-outline');
  } else {
    console.log("toggleDoorTwo() - showing door two");
    showDoorTwo = true;
    // show second door div
    door.style.display = "block";
    spacer.style.display = "none";
    // change checkbox icon to -marked-
    check.className = cls.replace('mdi-checkbox-blank-outline', 'mdi-checkbox-marked-outline');
  }
  localStorage.setItem('showDoorTwo', showDoorTwo ? 2 : 1);
}
  
/**
 * Delete config, reset wifi settings and restart controller.
 */
function factoryReset() {
  console.log("factoryReset()");
  doPost("/api/freset", undefined,
    function() {
      if(this.readyState != 4) return;
      openMessagePanel("Info", "<p>The device is resetting.</p>");
    }
  );
}

/**
 * Simply restart the controller
 */
function restartDevice() {
  console.log("restartDevice()");
  doPost("/api/restart", undefined,
    function() {
      if(this.readyState != 4) return;
      openMessagePanel("Info", "<p>The device is restarting.</p>");
    }
  );
}

/**
 * Send a firmware image update
 */
function fwUpdate() {
  console.log("fwUpdate");
  var fwForm = document.forms.fwForm;
  if(fwForm.value != "") {
    // TODO: use FormData object to send with XHR
    document.forms.fwForm.submit();
  }
}

/**
 * Send a filesystem image update
 */
function appUpdate() {
  console.log("appUpdate");
  var appForm = document.forms.appForm;
  if(appForm.value != "") {
    // TODO: use FormData object to send with XHR
    document.forms.appForm.submit();
  }
}

/**
 * Grab password form fields and submit config update
 */
function updatePassword() {
  var settings = { "web": { "username": "", "password": "" } };
  var form = document.forms.passwordForm;

  settings.web.username = form.elements.namedItem("username").value;
  settings.web.password = form.elements.namedItem("password").value;

  var f = function() { // completion callback
    if(this.readyState != 4) return;
    if(this.status == 200) {
      var res = JSON.parse(this.responseText);
      openMessagePanel("Info", "<p>The username and password were set successfully</p>");
    } else { // this is an error
      var res = parseJson(this.responseText);
      var error = "Unknown error";
      if(res.hasOwnProperty("error")) {
        error = res.error;
      }
      openMessagePanel("Error", "<p>The device returned an error: <i>" +
        error + "</i></p><p>The user/password update failed.</p>");
    }
  }

  if(settings.web.username != "" && settings.web.password != "") {
    doPost("/api/config", JSON.stringify(settings), f);
  } else {
    openMessagePanel("Error", "<p>The user or password field was blank!</p>");
  }
}

/**
 * Set the SSID and Password for WiFi Station mode
 */
function updateWifiSsid() {
  var settings = { "wifi": { "ssid": "", "password": "" } };
  var form = document.forms.wifiForm;

  settings.wifi.ssid = form.elements.namedItem("ssid").value;
  settings.wifi.password = form.elements.namedItem("password").value;

  var f = function() { // completion callback
    if(this.readyState != 4) return;
    if(this.status == 200) {
      var res = JSON.parse(this.responseText);
      openMessagePanel("Info", 
        "<p>The network name and password were set successfully</p>" +
        "<p>The device will join the new network at the next restart.</p>"
      );
    } else { // this is an error
      var res = parseJson(this.responseText);
      var error = "Unknown error";
      if(res.hasOwnProperty("error")) {
        error = res.error;
      }
      openMessagePanel("Error", "<p>The device returned an error: <i>" +
        error + "</i></p><p>The wifi configuration update failed.</p>");
    }
  }

  if(settings.wifi.ssid != "" && settings.wifi.password != "") {
    doPost("/api/config", JSON.stringify(settings), f);
  } else {
    openMessagePanel("Error", "<p>The user or password field was blank!</p>");
  }
}

/**
 * Ask the controller to scan wifi networks and return a list
 * TODO: add nice UI for scan data
 */
function wifiScan() {
  console.log("Requesting wifi network scan");
  doGet("/api/wifiScan", 
    function(){
      if(this.readyState != 4) return;
      var networks = JSON.parse(this.responseText);
      networks.sort(function(a, b){ return b.rssi - a.rssi; });
      console.log("wifi scan data: " + JSON.stringify(networks, null, 2));
    }
  );
}
