//hello.js
const addon = require("./build/Release/addon");
const mouse = require("./build/Release/mouse");
const keyboard = require("./build/Release/keyboard");
const sharexNode = require("./build/Release/sharex");
const focuscc = require("./build/Release/focus");
const bl = require("./build/Release/battlelist");
const AsyncLock = require('async-lock');
const {remote} = require('electron');

//global vars
//var win = remote.BrowserWindow.fromId(1);
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

remote.getCurrentWindow().on('close', (e) => {
  bl.finishDebugging(function(res){
    app.quit();
  });
  //win.hide();
});

//updateScreenCoords({});

var printlog = function(arg){
  var divConsole = document.getElementById("console");
  if(typeof(arg) == 'object'){
    console.log("object! it wont show on console.");
  }else{
    if(document.getElementById("console").innerText == ""){
      divConsole.innerHTML = arg;
    }else{
      divConsole.innerHTML = divConsole.innerHTML+"<br>"+arg;
    }
  }
  divConsole.scrollTop = divConsole.scrollHeight;
  console.log(arg);
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
        //win.minimize();

        //select game window
        sharexNode.getScreenResolution(function(res){
          updateScreenCoords(res);
          fBtnScreenCoords = 0;
        });
        //#5db31c = green
      });
    }
});

addon.registerHKF10Async(function(res){
  console.log("F10 key detected!!");
  prepareForFishing();
});

bl.getBattleList(function(res){
  let i;
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
      if((battlePokelist[res.addr]==0)||(battlePokelist[res.addr]==undefined)){
        for(i=0;i<searchPokeArr.length;i++){
          if(searchPokeArr[i] == res.name){
            console.log(res.addr, res.name);
            battlePokelist[res.addr] = {"addr": res.addr, "name": res.name, "type": res.type}
            break;
          }
        }
      }
      break;
  }
  //console.log(res.addr.toString(16).toUpperCase(), res.name, res.type)
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
    if(battlePokelist[iBl]!= 0){
      for(i=0; i<searchPokeArr.length; i++){
        if(battlePokelist[iBl].name == searchPokeArr[i]){
          if(fcheckChangeBlRestarted == 1){
            console.log("already fought and restarted function. finishing this call...");
          }else{
            //check if pokemon is near/close
            console.log(`checking if ${battlePokelist[iBl].name} is near`);
            const res = bl.isPkmNearSync(battlePokelist[iBl].addr);
            if(res.isNear){
              //:TODO add a check inside of isPkmNear to see if pkm was rly clicked, if not try again.
              //attack and just return after pokemon is dead
              ftargetFound=1;
              locktarget.acquire("locktarget", function(donetarget){
                lock.acquire("lockmouse", function(done){
                  if(battlePokelist[iBl]!=0){
                    console.log(`lockmouse attack acquired! ${battlePokelist[iBl].addr}`);
                  }else{
                    done();
                    donetarget();
                    console.log("fake target. was deleted during processing. will restart as lookForFighting in the doneTarget()");
                  }
                  //  console.log("lock2 fighting acquired!");
                  bl.attackPkm(battlePokelist[iBl].addr, function(){
                      done();
                  });
                }, function(err, ret){
                  // lockmouse released
                  console.log("lockmouse attack released! [pokemon selected]");
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
                fcheckChangeBlRestarted = 1;
                setTimeout(lookForFighting, 1000);
                /*
                console.log("lookforfighting finished [locktarget]");
                console.log(`is locktarget busy again? ${locktarget.isBusy()}`);
                if(!locktarget.isBusy()){
                }
                */
                return 1;
              });
              return 1; //async mode, this return will block the process to continue check for pokemon after found one
            }else{ //res.near else
              //it was dead by other player, disappeared of screen or dead by a area skill
              console.log("Pokemon NOT close");
              battlePokelist[iBl]=0;
              //in case program didnt delete it or it was dead by another player
              //faraway from player screen
            }
          }
        }
      }
    }
  }
  //no pokemons in the battlePokelist = restart check
  if((ftargetFound == 0) && (fcheckChangeBlRestarted == 0)){
    fLookForFighting = 0;
    console.log("lookforfighting finished [notargetfound]");
    checkChangeBattlelist();
  }
}

//sqm's in the screen: 15x11
function updateScreenCoords(res){

/*  res = {"x":0, "y":0, "w":0, "h":0}

  res.x = 324;
  res.y = 174;
  res.w = 402;
  res.h = 293;
*/

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

  bl.setScreenConfig(center, sqm, function(){
  });
}


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

      mouse.setCursorPos(coords, function(res){console.log(coords, "cursor at pos set");});
      bl.fish(function(res){console.log("fish clicked!");done();});
//      keyboard.pressKbKey("Fishing", function (res){}); //keyboard CTRL+Z 1
//      mouse.leftClick(function(res){ console.log("leftclicked!");done();});

    }, function(err, ret){
        console.log("lockmouse released");
    }); //end lock

    //wait for the change of color, press CTRL+Z when the fish appears
    console.log("Waiting for fish....");

    mouse.getColorFishing({
      "x": coords.x, "y": coords.y
    },function(res){
      if(!pause){
        keyboard.pressKbKey("Fishing", function (res){
          console.log("Fishing Rod Pulled Up!!");
          //end of fishing
          //IF pause is not requested, continue Fishing
          //restart fishing after some time (this time is necessary:
          //time: w8 for fishing sqm end animation of fished up to
          //"available to fish here"
          setTimeout(startFishing, 1000);
        }); //keyboard. CTRLZ 2
      }
    }); //mouse.getColorFishing
  }); //focuscc.focusWIndow
}
