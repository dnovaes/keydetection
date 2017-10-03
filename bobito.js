//hello.js
const addon = require("./build/Release/addon");
const mouse = require("./build/Release/mouse");
const keyboard = require("./build/Release/keyboard");
const sharexNode = require("./build/Release/sharex");
const focuscc = require("./build/Release/focus");
const bl = require("./build/Release/battlelist");
const AsyncLock = require('async-lock');
const {remote} = require('electron')

//global vars
var win = remote.BrowserWindow.fromId(1);
var pause = true;
var battlePokelist = {};
var searchPokeArr= ["Magikarp"];
var lock = new AsyncLock();
var locktarget = new AsyncLock();

var center = {
  "x": 0,
  "y": 0
}
var sqm = {
  "length": 0,
  "height": 0
}
var counterBattlelist = 168; //=0
var fBtnScreenCoords = 0;

//updateScreenCoords({});

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

addon.registerHKF10Async(function(res){
  console.log("F10 key detected!!");
  prepareForFishing();
});

bl.getBattleList(function(res){
  switch(res.type){
    case -100:
      res.type = "NPC";
      break;
    case 32:
    case 92:
      res.type = "PLAYER";
      break;
    case 112:
      res.type = "POKEMON";
      battlePokelist[res.addr] = {"addr": res.addr, "name": res.name, "type": res.type}
      break;
  }
  //console.log(res.addr.toString(16).toUpperCase(), res.name, res.type)
  console.log(res.addr, res.name, res.type);
});

var fLookForFighting = 0;
let fightCounter = 0
checkChangeBattlelist();

//###########################################
//              FUNCTIONS
//###########################################

function checkChangeBattlelist(){
  bl.checkChangeBattlelist(function(res){
      //if there is pokemon on battlelist, counterBattlelist changed and no lookForFighting. then call a new lookForFight function
      if((res.blCounter != counterBattlelist)&&(Object.keys(battlePokelist).length>0)
          &&(fLookForFighting==0)){
        counterBattlelist = res.blCounter;
        lookForFighting(); //lookForFighting recall changeBattlelist after finished
      }else{
        setTimeout(checkChangeBattlelist, 1000);
      }
  });
}

// search for a pokemon arround the player position to click on it (to fight), after it. recall
// can target only one pokemon per call
async function lookForFighting(){
  let i, ftargetFound=0, fcheckChangeBlRestarted=0;
  let currentBlist = Object.keys(battlePokelist);
  let lengthBl = currentBlist.length;
  let blCount = 0;

  fLookForFighting = 1;
  fightCounter+=1;
  console.log(`\ncalls to fight: ${fightCounter}`);
  console.log("lookForfighting started");

  for (const iBl of currentBlist){
    blCount+=1;
    for(i=0; i<searchPokeArr.length; i++){
      if((battlePokelist[iBl]!= 0) && (battlePokelist[iBl].name == searchPokeArr[0])){
        ftargetFound=1;
        if(fcheckChangeBlRestarted == 1){
          console.log("already fought and restarted function. finishing this call...");
        }else{
          //check if pokemon is near/close
          console.log(`checking if ${battlePokelist[iBl].name} is near`);
          const res = bl.isPkmNearSync(battlePokelist[iBl].addr);
          if(res.isNear){
            //:TODO add a check inside of isPkmNear to see if pkm was rly clicked, if not try again.
            //attack and just return after pokemon is dead

            locktarget.acquire("locktarget", function(donetarget){
              lock.acquire("lockmouse", function(done){
                if(battlePokelist[iBl]!=0){
                  console.log(`\nlockmouse attack acquired! ${battlePokelist[iBl].addr}`);
                }else{
                  done();
                  donetarget();
                  return 0;
                  console.log("testing return0");
                }
                //  console.log("lock2 fighting acquired!");
                bl.attackPkm(battlePokelist[iBl].addr, function(){
                    done();
                });
              }, function(err, ret){
                // lockmouse released
                console.log("lockmouse attack released!");
              });

              bl.waitForDeath(battlePokelist[iBl].addr, function(){
                console.log("pokemon is dead.");
                battlePokelist[iBl] = 0;
  //              if((fcheckChangeBlRestarted == 0)&&(blCount == lengthBl)){
                  //after pokmon dead, force a call to lookForfight to check if there is still
                  //other pokemons to target
  //              }
                donetarget();
              });
            }, function(err, ret){
              console.log("lookforfighting finished [locktarget]");
              console.log(`is locktarget busy again? ${locktarget.isBusy()}`);
              if(!locktarget.isBusy()){
                fcheckChangeBlRestarted = 1;
                setTimeout(lookForFighting, 1000);
              }
              return 1;
              console.log("testing return1");
            });

          }else{
            //console.log("Pokemon NOT close");
            //console.log(`blCount: ${blCount}, lengthBl: ${lengthBl}`);
            //in case program didnt delete it or it was dead by another player
            //faraway from player screen
            //if(res.isDead){
            //  delete battlePokelist[iBl];
            //}
          }
        }
      }
    }
    //when all the pokemons in battlePokelist isnt in list of target pokemons (searchPokeArr)
    //it doesnt enter in the if and goes out of loop without restarting checkChangeBattlelist.
    //its here when we check if this scenario happened
  }
  fLookForFighting = 0;
  //no pokemons in the battlePokelist = restart check
  if((ftargetFound == 0) && (fcheckChangeBlRestarted == 0)){
    checkChangeBattlelist();
    console.log("lookforfighting finished [notargetfound]");
  }
}

//sqm's in the screen: 15x11
function updateScreenCoords(res){

  res.x = parseInt(res.x);
  res.y = parseInt(res.y);
  res.w = parseInt(res.w);
  res.h = parseInt(res.h);

/*
  res.x = 904;
  res.y = 133;
  res.w = 514;
  res.h = 377;
*/

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

  bl.setScreenConfig(center, sqm, function(){

  });
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


  //select game window

  //TODO: add focusWindow in every keyboard action later
  focuscc.focusWindow(function(res){

    //move mouse 2 sqm of distance to top
    coords.x = center.x+(3*sqm.length);
    coords.y = center.y+(0*sqm.height);
    coords.y = coords.y+(sqm.height/3);

    lock.acquire("lockmouse", function(done){
      console.log("\nlockmouse fishing acquired!");
      console.log("Start the Fishing!!!");

      mouse.setCursorPos(coords, function(res){console.log("cursor at pos set");});
      keyboard.pressKbKey("Fishing", function (res){}); //keyboard CTRL+Z 1
      mouse.leftClick(function(res){ console.log("leftclicked!");done();});

    }, function(err, ret){
        console.log("lockmouse released");
    }); //end lock

    //wait for the change of color, press CTRL+Z when the fish appears
    console.log("Waiting for fish....");

    mouse.getColorFishing({
      "x": coords.x, "y": coords.y
    },function(res){
      keyboard.pressKbKey("Fishing", function (res){
        console.log("Fishing Rod Pulled Up!!");
        //end of fishing
        //IF pause is not requested, continue Fishing
        //recursevely call to Fish!
        if(!pause){
          //restart fishing after some time (this time is necessary:
          //time: w8 for fishing sqm end animation of fished up to
          //"available to fish here"
          setTimeout(startFishing, 1000);
        }
      }); //keyboard. CTRLZ 2
    }); //mouse.getColorFishing
  }); //focuscc.focusWIndow
}
