//hello.js
const addon = require("./build/Release/addon");
const mouse = require("./build/Release/mouse");
const keyboard = require("./build/Release/keyboard");

/*
var f_F10 = addon.registerHotKeyF10();
if(f_F10){
  console.log("F10 key detected!!");
}
*/
var pause = true;

var center = {
  "x": 0,
  "y": 0
}
var sqm = {
  "length": 0,
  "height": 0
}

//var posFishing;
addon.registerHKF10Async(function(res){
  console.log("F10 key detected!!");
  if(pause == true){
    console.log("Unpaused!");
    startFishing();
  }else{
    console.log("Pause Requested!");
  }
  pause = !pause;
});

console.log("Hello.js is running");

//sqm's in the screen: 15x11

addon.getCursorPosition(function(res){
  console.log("Mouse position detected: ");
  console.log(res);

  console.log("Calculating sice of each sqm in screen: ");

  res.SE.x = parseInt(res.SE.x);
  res.SE.y = parseInt(res.SE.y);
  res.NW.x = parseInt(res.NW.x);
  res.NW.y = parseInt(res.NW.y);

  //temp: my client
  res.SE.x = 1397;
  res.SE.y = 598;
  res.NW.x = 804;
  res.NW.y = 163;

  //length and height for each SQM
  sqm.length = (res.SE.x - res.NW.x)/15;
  sqm.height = (res.SE.y - res.NW.y)/11;

  console.log("Each sqm has "+sqm.length.toFixed(2)+" of length");
  console.log("Each sqm has "+sqm.height.toFixed(2)+" of height");

  console.log("Center: ");
  center.x = res.NW.x + (res.SE.x - res.NW.x)/2;
  center.y = res.NW.y + (res.SE.y - res.NW.y)/2;

  console.log(center);
});

function startFishing(){
  var coords = {"x":0, "y":0};

  console.log(" ");
  console.log("Start the Fishing!!!");

  //move mouse to center
  //mouse.setCursorPos(center, function(res){});

  //move mouse 2 sqm of distance to the right
  coords.x = center.x+(2*sqm.length);
  coords.y = center.y;
  mouse.setCursorPos(coords, function(res){});

  //press keys CTRL + Z
  keyboard.pressKbKey("Fishing", function (res){});

  //press LEFTCLICK of mouse
  mouse.leftClick(function(res){
      //console.log("Fishing");
  });

  //wait for the change of color, press LEFTCLICK of mouse again
  console.log("Waiting for fish....");
  mouse.getColorFishing({
    "x": coords.x, //center+2sqm to right
    "y": center.y+(sqm.height/4)+4
  },function(res){
    console.log("Fishing Rod Pulled Up!!");
    keyboard.pressKbKey("Fishing", function (res){
      //that is the last async function to execute.
      //IF pause is not requested, continue Fishing
      //recursevely call to Fish!
      if(!pause){
        setTimeout(startFishing, 1500);
      }
    });
  });
}
