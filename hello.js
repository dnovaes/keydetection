//hello.js
const addon = require("./build/Release/addon");

/*
var f_F10 = addon.registerHotKeyF10();
if(f_F10){
  console.log("F10 key detected!!");
}
*/

//var posFishing;
addon.registerHKF10Async(function(res){
  console.log("F10 key detected!!");
  //addon.startFishing(posFishing);
});


console.log("Hello.js is running");

//sqm's in the screen: 15x11

addon.getCursorPosition(function(res){
  console.log("Mouse position detected: ");
  console.log(res);
});


