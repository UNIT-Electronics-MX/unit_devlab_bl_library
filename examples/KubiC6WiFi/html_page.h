#pragma once

const char HTML_PAGE[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang="en">

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width, initial-scale=1.0">

<title>KubiDeskPet</title>

<style>

*{
  margin:0;
  padding:0;
  box-sizing:border-box;
}

body{

  font-family: Arial, Helvetica, sans-serif;

  background:
    linear-gradient(
      180deg,
      #0b1020 0%,
      #111827 100%
    );

  color:white;

  min-height:100vh;

  padding:20px;
}

/* =======================================
HEADER
======================================= */

.header{

  text-align:center;

  margin-bottom:30px;
}

.logo{

  font-size:42px;

  font-weight:900;

  background:linear-gradient(
    90deg,
    #00d4ff,
    #6c63ff
  );

  -webkit-background-clip:text;

  -webkit-text-fill-color:transparent;

  margin-bottom:10px;
}

.subtitle{

  color:#9ca3af;

  font-size:15px;
}

/* =======================================
CARD
======================================= */

.card{

  background:rgba(255,255,255,0.05);

  border:1px solid rgba(255,255,255,0.08);

  border-radius:24px;

  padding:20px;

  margin-bottom:20px;

  backdrop-filter:blur(10px);

  box-shadow:
    0 8px 30px rgba(0,0,0,0.3);
}

.card h2{

  margin-bottom:16px;

  font-size:20px;

  color:#e5e7eb;
}

/* =======================================
GRID
======================================= */

.grid{

  display:grid;

  grid-template-columns:
    repeat(auto-fit,minmax(130px,1fr));

  gap:14px;
}

/* =======================================
BUTTONS
======================================= */

button{

  border:none;

  border-radius:18px;

  padding:16px;

  font-size:16px;

  font-weight:bold;

  cursor:pointer;

  color:white;

  transition:0.2s;

  box-shadow:
    0 4px 15px rgba(0,0,0,0.3);
}

button:hover{

  transform:translateY(-2px);

  opacity:0.92;
}

button:active{

  transform:scale(0.98);
}

.happy{
  background:linear-gradient(135deg,#00c853,#64dd17);
}

.sad{
  background:linear-gradient(135deg,#546e7a,#78909c);
}

.angry{
  background:linear-gradient(135deg,#ff1744,#ff5252);
}

.sleepy{
  background:linear-gradient(135deg,#5e35b1,#7e57c2);
}

.vomit{
  background:linear-gradient(135deg,#43a047,#c0ca33);
}

.surprised{
  background:linear-gradient(135deg,#ff9100,#ffca28);
}

.clock{
  background:linear-gradient(135deg,#1565c0,#42a5f5);
}

/* =======================================
INPUT
======================================= */

.inputBox{

  width:100%;

  padding:16px;

  border:none;

  border-radius:16px;

  margin-bottom:14px;

  background:#1f2937;

  color:white;

  font-size:16px;

  outline:none;
}

.inputBox::placeholder{

  color:#9ca3af;
}

/* =======================================
SWITCH
======================================= */

.switchContainer{

  display:flex;

  align-items:center;

  justify-content:space-between;

  background:#1f2937;

  padding:18px;

  border-radius:18px;
}

.switchContainer span{

  font-size:18px;

  font-weight:bold;
}

.switch{

  position:relative;

  display:inline-block;

  width:64px;

  height:34px;
}

.switch input{

  opacity:0;

  width:0;

  height:0;
}

.slider{

  position:absolute;

  cursor:pointer;

  top:0;
  left:0;
  right:0;
  bottom:0;

  background:#ef5350;

  transition:.3s;

  border-radius:34px;
}

.slider:before{

  position:absolute;

  content:"";

  height:26px;

  width:26px;

  left:4px;

  bottom:4px;

  background:white;

  transition:.3s;

  border-radius:50%;
}

input:checked + .slider{

  background:#00c853;
}

input:checked + .slider:before{

  transform:translateX(30px);
}

/* =======================================
FOOTER
======================================= */

.footer{

  text-align:center;

  margin-top:30px;

  color:#6b7280;

  font-size:13px;
}

</style>

</head>

<body>

<!-- ===================================== -->
<!-- HEADER -->
<!-- ===================================== -->

<div class="header">

  <div class="logo">
    KubiDeskPet
  </div>

  <div class="subtitle">
    UNIT Electronics · Smart Desktop Robot
  </div>

</div>

<!-- ===================================== -->
<!-- EMOTIONS -->
<!-- ===================================== -->

<div class="card">

  <h2>
    Emotions
  </h2>

  <div class="grid">

    <button
      class="happy"
      onclick="emotion('happy')">
      Happy
    </button>

    <button
      class="sad"
      onclick="emotion('sad')">
      Sad
    </button>

    <button
      class="angry"
      onclick="emotion('angry')">
      Angry
    </button>

    <button
      class="sleepy"
      onclick="emotion('sleepy')">
      Sleepy
    </button>

    <button
      class="vomit"
      onclick="emotion('vomit')">
      Vomit
    </button>

    <button
      class="surprised"
      onclick="emotion('surprised')">
      Surprise
    </button>

  </div>

</div>

<!-- ===================================== -->
<!-- CONTROLS -->
<!-- ===================================== -->

<div class="card">

  <h2>
    Controls
  </h2>

  <div class="grid">

    <button
      class="clock"
      onclick="showClock()">
      Clock
    </button>

  </div>

  <br>

  <div class="switchContainer">

    <span>
      Auto Mode
    </span>

    <label class="switch">

      <input
        type="checkbox"
        id="autoSwitch"
        checked
        onchange="toggleAuto()"
      >

      <span class="slider"></span>

    </label>

  </div>

</div>

<!-- ===================================== -->
<!-- MESSAGE -->
<!-- ===================================== -->

<div class="card">

  <h2>
    Message
  </h2>

  <input
    class="inputBox"
    type="text"
    id="msg"
    placeholder="Write message..."
  >

  <button
    class="clock"
    style="width:100%;"
    onclick="sendMessage()">

    Send Message

  </button>

</div>

<div class="footer">
  Powered by UNIT Electronics
</div>

<script>

// =======================================
// EMOTION
// =======================================

function emotion(name){

  fetch('/emotion?name=' + name);
}

// =======================================
// CLOCK
// =======================================

function showClock(){

  fetch('/clock');
}

// =======================================
// AUTO MODE
// =======================================

function toggleAuto(){

  let enabled =
    document.getElementById("autoSwitch").checked;

  let state =
    enabled ? "on" : "off";

  fetch('/auto?state=' + state);
}

// =======================================
// MESSAGE
// =======================================

function sendMessage(){

  let text =
    document.getElementById("msg").value;

  fetch(
    "/message?text=" +
    encodeURIComponent(text)
  );
}

// =======================================
// SYNC TIME
// =======================================

function syncTime(){

  const epoch =
    Math.floor(Date.now()/1000);

  fetch('/setTime?epoch=' + epoch);
}

syncTime();

</script>

</body>
</html>

)rawliteral";