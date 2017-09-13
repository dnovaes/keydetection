//hello.js
const addon = require("./build/Release/addon");

addon.focusWindow(function(res){
  console.log("Screen PXG focused.");
});
