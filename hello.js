//hello.js
const addon = require("./build/Release/addon");

/*
var f_F10 = addon.registerHotKeyF10();
if(f_F10){
  console.log("F10 key detected!!");
}
*/

addon.registerHKF10Async(function(res){
  console.log("F10 key detected!!");
});

console.log("Hello.js is running");


