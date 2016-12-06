
var currentView = 'home';
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
    refreshDoorStates();
  }
}

/**
 * Show a view, hide the previous one.  Views are referenced by contianing DIV ID
 */
function openView(view) {
  document.getElementById(currentView).style.display = 'none';
  document.getElementById(view).style.display = 'block';
  currentView = view;
}

/**
 * send an ajax GET request
 */
function doGet(uri, success_cb, error_cb) {
  if(success_cb === undefined) success_cb = function(){ };
  if(error_cb === undefined) error_cb = function(){
    openMessagePanel("The operation failed, the device did not respond to our request");
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
    openMessagePanel("The operation failed, the device did not respond to the request");
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
function openMessagePanel(message) {
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
  doGet("/api/doorStatus", updateDoorStates);
}

/**
 * Update the main page images based on door states (open/closed)
 */
function updateDoorStates() {
  if(this.readyState != 4) return; // done done yet

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
}

/**
 * Send a command to activate the first door
 */
function activateDoorOne() {
  console.log("activateDoorOne()");
  doPost("/api/door1"); // don't need response
}

/**
 * Send a command to activate the second door
 */
function activateDoorTwo() {
  console.log("activateDoorTwo()");
  doPost("/api/door2"); // don't need response
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
    function() { openMessagePanel("The device is resetting."); }
  );
}

/**
 * Simply restart the controller
 */
function restartDevice() {
  console.log("restartDevice()");
  doPost("/api/restart", undefined,
    function() { openMessagePanel("The device is restarting."); }
  );
}

/**
 * Send a firmware image update
 */
function fwUpdate() {
  console.log("fwUpdate");
  var fwForm = document.forms.fwForm;
  if(fwForm.value != "") {
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
    document.forms.appForm.submit();
  }
}

function updatePassword() {
  var settings = { "web": { "username": "", "password": "" } };
  var form = document.forms.passwordForm;
  var u = form.elements.namedItem("username").value;
  var p = form.elements.namedItem("password").value;

  var f = function() { // completion callback
    if (this.readyState == 4 && this.status == 200) {
      var res = JSON.parse(this.responseText);
    } else if(this.readyState == 4) { // this is an error
      var res = parseJson(this.responseText);
      var error = "Unknown error";
      if(res.hasOwnProperty("error")) {
        error = res.error;
      }
      openMessagePanel("<p>The device returned an error: <i>" + 
        error + "</i></p><p>The password update failed.</p>");
    }
  }
  var e = function() { // error callback (connection failure)
    openMessagePanel("<p>Failed to contact the device, the password update failed.</p>");
  };

  if(u != "" && p != "") {
    settings.web.username = u;
    settings.web.password = p;
    doPost("/api/updateConfig", JSON.stringify(settings), f, e);
  }
}

/**
 * Ask the controller to scan wifi networks and return a list
 */
function wifiScan() {
  console.log("Requesting wifi network scan");
  doGet("/api/wifiScan", wifiScanResult);
}

/**
 * Process the result of a wifi scan
 */
function wifiScanResult(result) {
  var networks = JSON.parse(result.responseText);
  console.log("updating door states: " + JSON.stringify(networks));
}
