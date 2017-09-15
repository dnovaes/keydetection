//hello.js
const addon = require("./build/Release/addon");
const mouse = require("./build/Release/mouse");
const keyboard = require("./build/Release/keyboard");
const sharexNode = require("./build/Release/sharex");
const focuscc = require("./build/Release/focus");
const {remote} = require('electron')

var win = remote.BrowserWindow.fromId(1);
var pause = true;

var center = {
  "x": 0,
  "y": 0
}
var sqm = {
  "length": 0,
  "height": 0
}

var fBtnScreenCoords = 0;
var printlog = console.log;

var console = {}
console.log = function(arg){
  var divConsole = document.getElementById("console");
  if(typeof(arg) == 'object'){
    printlog("object! it wont show on console.");
  }else{
    if(document.getElementById("console").innerText == ""){
      divConsole.innerHTML = arg;
    }else{
      divConsole.innerHTML = divConsole.innerHTML+"<br>"+arg;
    }
  }
  divConsole.scrollTop = divConsole.scrollHeight;
  printlog(arg);
}

divFishing = document.getElementById("fishing");
divFishing.addEventListener("click", function(){
    divFishing.style.background = 'linear-gradient(red, #774247)';
    divFishing.style.border = '1px solid white';
    setTimeout(function(){
      divFishing.style.border = 'none';
    }, 100);
    prepareForFishing();
});

divScreenCoords = document.getElementById("screenCoords");
divScreenCoords.addEventListener("click", function(){
    if(!fBtnScreenCoords){
      fBtnScreenCoords = 1;

      focuscc.focusWindow(function(res){
        win.minimize();

        while(win.isMinimized() == false){console.log("not minimized! looping\n");}

        
          //select game window
          //TODO: make a function later that checks if bot window is foreground before taking a screenshot
          sharexNode.getScreenResolution(function(res){
            console.log(res);
            updateScreenCoords(res);
            //#5db31c = green
            fBtnScreenCoords = 0;
          });
      });
    }
});

//var posFishing;
addon.registerHKF10Async(function(res){
  console.log("F10 key detected!!");
  prepareForFishing();
});

//sqm's in the screen: 15x11
function updateScreenCoords(res){
  res.x = parseInt(res.x);
  res.y = parseInt(res.y);
  res.w = parseInt(res.w);
  res.h = parseInt(res.h);

  var coordxEl = document.querySelector("input[name='coordx']");
  coordxEl.value = res.x;

  var coordyEl = document.querySelector("input[name='coordy']");
  coordyEl.value = res.y;

  var coordwEl = document.querySelector("input[name='coordw']");
  coordwEl.value = res.w;

  var coordhEl = document.querySelector("input[name='coordh']");
  coordhEl.value = res.h;

  //length and height for each SQM
  sqm.length = parseInt((res.w/15).toFixed(2));
  sqm.height = parseInt((res.h/11).toFixed(2));

  
  console.log("Sqm: "+sqm.length+"x"+sqm.height);
  
  center.x = res.x + res.w/2;
  center.y = res.y + res.h/2;
  
  console.log("Screen Coords captured");
}

//sqm's in the screen: 15x11
/*
addon.getCursorPosition(function(res){
  console.log("Mouse position detected: ");
  console.log(res);

  console.log("Calculating size of each sqm in screen: ");

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
*/

function prepareForFishing(){
  if(pause == true){
    console.log("Unpaused!");
    divFishing.style.background = 'linear-gradient(#774247, #f7e53f)';
    startFishing();
  }else{
    console.log("Pause Requested!");
    divFishing.style.background = 'linear-gradient(red, #774247)';
  }
  pause = !pause;
}

function startFishing(){
  var coords = {"x":0, "y":0};

  console.log(" ");
  console.log("Start the Fishing!!!");

  //select game window

//  ls.stdout.on('data', function (data) {
  focuscc.focusWindow(function(res){

    //move mouse to center
    //mouse.setCursorPos(center, function(res){});

    //move mouse 2 sqm of distance to the right
    coords.x = center.x+(2*sqm.length);
    coords.y = center.y;
    mouse.setCursorPos(coords, function(res){

      //press keys CTRL + Z
      keyboard.pressKbKey("Fishing", function (res){
        //press LEFTCLICK of mouse
        mouse.leftClick(function(res){});

        //wait for the change of color, press LEFTCLICK of mouse again
        console.log("Waiting for fish....");
        mouse.getColorFishing({
          "x": coords.x, //center+2sqm to right
          "y": center.y+(sqm.height/3)
        },function(res){
          console.log("Fishing Rod Pulled Up!!");
          keyboard.pressKbKey("Fishing", function (res){
            //that is the last async function to execute.
            //IF pause is not requested, continue Fishing
            //recursevely call to Fish!
            if(!pause){
              //restart fishing after some time
              setTimeout(startFishing, 1500);
            }
          });
        });
      });

    });


  });

}
