//hello.js
const addon = require("./build/Release/addon");

addon.focusWindow(function(res){
  console.log(res);
  console.log("Screen Resolution is now set.");
});
