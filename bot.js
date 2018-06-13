
const bl = require("../build/Release/battlelist");
const sharexNode = require("../build/Release/sharex");
const keyboard = require("../build/Release/keyboard");
const focuscc = require("../build/Release/focus");
const mouse = require("../build/Release/mouse");
const AsyncLock = require('async-lock');

//global vars
//var win = remote.BrowserWindow.fromId(1);
var pause = true;
var battlePokelist = {};
var searchPokeArr= ["Magikarp", "Rattata"];
var lock = new AsyncLock();
var locktarget = new AsyncLock();
var moduleAddr = 0;
var pid = 0;
var fRevive = 0;
var fPause = false;

var center = {
  "x": 0,
  "y": 0
}
var sqm = {
  "length": 0,
  "height": 0
}
var pos = {
  pkmSlot: { "x":1432, "y":139 },
  reviveSlot: { "x":1464, "y":139 }
}
var counterBattlelist = 168; // === 0
var fBtnScreenCoords = 0;

var printlog = function(arg){
  var divConsole = document.getElementById("console");
  if(typeof(arg) == 'object'){
    console.log("object found on printlog! it wont show on console.");
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

var audio = new Audio('../assets/audio.mp3');
/*
focuscc.focusWindow(function(res){
  bl.revivePkm();
});
*/

//first function to run. It gets moduleaddress and pid of the program too
bl.getBattleList(function(res){
  let i;

  if(res.fAction == 2){
    switch(res.type){
      case 72:
        res.type = "NPC";
        break;
      case 104: //myself
      case 8:
        res.type = "PLAYER";
        break;
      //case 112:
      case 76:
        res.type = "POKEMON";
        if((battlePokelist[res.addr] === 0)||(battlePokelist[res.addr] === undefined)){
          for(i=0;i<searchPokeArr.length;i++){
            if(searchPokeArr[i] == res.name){
              battlePokelist[res.addr] = {
                "addr": res.addr,
                "name": res.name,
                "type": res.type,
                "lookType": res.lookType,
                "counterBl": res.counterBl
              }
            }
            if((res.name === "Machamp")&&(res.lookType != 121)){
              //https://i.imgur.com/hDB7e46.png/ shiny machamp looktype 26
              audio.play();
              printlog(`\nShiny Machamp!\n`);
            }
            break;
          }
        }
        break;
    }
    printlog(`0x${res.addr.toString(16).toUpperCase()}  ${res.name} ${res.type} ${res.lookType}`)
  }else{
    moduleAddr = res.moduleAddr;
    pid = res.pid;
  }
});

/*
mouse.getCursorPosbyClick(function(res){
    console.log(`res.x: ${res.x} res.y: ${res.y}`);
});
*/




// ============  Global Bot Hotkeys =====================
//f10 key = pause
bl.registerHKF10Async(function(res){
  fPause = !fPause;
  console.log(`F10 key detected!! Status: ${fPause}`);
  //bl.testread();
});
/*
addon.registerHKF10Async(function(res){
  console.log("F10 key detected!!");
  if(center.x != 0 & center.y != 0){
    prepareForFishing();
  }else{
    alert(`Configure a tela do seu pokemon pelo bot antes de iniciar`);
  }
});
*/

//delete key
bl.registerHkRevivePkm(function(res){
  if(fRevive === 1){
    console.log("REVIVE key detected!!");
    focuscc.focusWindow(function(res){
      bl.revivePkm(pos.pkmSlot, pos.reviveSlot);
    });
  }
});


// ============  Featured Buttons =====================
divFishing = document.getElementById("fishing");
divFishing.addEventListener("click", function(){
  if(center.x != 0 & center.y != 0){
    divFishing.style.background = 'linear-gradient(red, #774247)';
    divFishing.style.border = '1px solid white';
    setTimeout(function(){
      divFishing.style.border = 'none';
    }, 100);
    prepareForFishing();
  }else{
    alert(`Configure a tela do seu pokemon pelo bot antes de iniciar`);
  }
});

divRevive = document.getElementById("revive");
divRevive.addEventListener("click", function(){
  if(fRevive === 0){
    focuscc.focusWindow(function(res){
      fRevive=2; //cursorPosbyClick is waiting for a response

      //getting pkm slot pos
      mouse.getCursorPosbyClick(function(res){
        console.log(`PkmSlot) res.x: ${res.x} res.y: ${res.y}`);
        pos.pkmSlot.x = res.x;
        pos.pkmSlot.y = res.y;
        divRevive.style.background = 'linear-gradient(#774247, #f7e53f)';
        fRevive=1;
      });

      /*
      //getting revive slot pos
      mouse.getCursorPosbyClick(function(res){
        console.log(`ReviveSlot) res.x: ${res.x} res.y: ${res.y}`);
        pos.reviveSlot.x = res.x;
        pos.reviveSlot.y = res.y;
      });
      */
    });
  }else if (fRevive === 1){
    divRevive.style.background = 'linear-gradient(red, #774247)';
    fRevive=0;
  }
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

var fLookForFighting = 0;
var fightCounter = 0

/*
setInterval(function(){
  checkChangeBattlelist();
}, 1100);
*/

btCheckpoint = document.querySelector("#checkpoint");
btCheckpoint.addEventListener("click", function(){
  let newCommand = document.createElement("li");
  newCommand.classList.add("sel-el");
  newCommand.dataset.cmdType = "check";
  newCommand.addEventListener("click", removeCommandEl);
  bl.getPlayerPos(function(res){
    newCommand.innerHTML = `<div class="sel-text">${res.posx},${res.posy},${res.posz}</div><div class="remove-el"></div>`;
    listCommands.appendChild(newCommand);
  });
});

// ============  Fishing  =====================

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
  let fishStatus = 2; //normal pos
  let fPkmSummoned;

  //select game window
  //TODO: add focusWindow in every keyboard action later
  focuscc.focusWindow(function(res){

    //test if player's pokemon is out of pokebal.. if not use revive and summon it //sync function
    fPkmSummoned = bl.isPlayerPkmSummoned();
    console.log(`is pokemon summoned? ${fPkmSummoned.status}`);
    if(fPkmSummoned.status == "1" && fRevive === 1){
      //bl.revivePkm(pos.pkmSlot, pos.reviveSlot);
      bl.revivePkm(pos.pkmSlot, {x: 0, y: 0});
    }

    //move mouse 2 sqm of distance to top
    coords.x = center.x+(3*sqm.length);
    coords.y = center.y+(0*sqm.height);
    coords.y = coords.y+(sqm.height/3);
//    coords.y = coords.y+(2*sqm.height/5);

    lock.acquire("lockmouse", function(done){
      console.log("\nlockmouse fishing acquired!");
      console.log("Start the Fishing!!!");

      mouse.setCursorPos(coords, function(res){console.log(coords, "cursor at pos set");});
      bl.fish(function(res){
        console.log("fish clicked!");
        fishStatus = res.fishStatus;
        console.log("fishStatus: ", res.fishStatus);
        done();
      });

      //test if player's pokemon is out of pokebal.. if not use revive and summon it //sync function
      fPkmSummoned = bl.isPlayerPkmSummoned();
      if(fPkmSummoned.status == "1" && fRevive === 1){
        bl.revivePkm(pos.pkmSlot, pos.reviveSlot);
      }

    }, function(err, ret){
      console.log("lockmouse released");
      //test if player's pokemon is out of pokebal.. if not use revive and summon it //sync function
      fPkmSummoned = bl.isPlayerPkmSummoned();
      if(fPkmSummoned.status == "1" && fRevive === 1){
        bl.revivePkm(pos.pkmSlot, pos.reviveSlot);
      }

      //if player started fishing (3), wait for pokemon to get the fish, if not, restart fishing
      if(fishStatus != 2){
        //wait for the change of color, press CTRL+Z when the fish appears
        console.log("Waiting for fish to fish....");

        let fish;

        mouse.getColorFishing({
          "x": coords.x, "y": coords.y
        }, pid, moduleAddr, function(res){
          if(!pause){
            keyboard.pressKbKey("Fishing", function (res){
              console.log("Fishing Rod Pulled Up!!");
              //end of fishing

              //test if player's pokemon is out of pokebal.. if not use revive and summon it //sync function
              fPkmSummoned = bl.isPlayerPkmSummoned();
              if(fPkmSummoned.status == "1" && fRevive === 1){
                bl.revivePkm(pos.pkmSlot, pos.reviveSlot);
              }
              //IF pause is not requested, continue Fishing
              //restart fishing after some time (this time is necessary:
              //time: w8 for fishing sqm end animation of fished up to
              //"available to fish here"
              setTimeout(startFishing, 1000);
            }); //keyboard. CTRLZ 2
          }

        }); //mouse.getColorFishing

      }else{
        //test if player's pokemon is out of pokebal.. if not use revive and summon it //sync function
        fPkmSummoned = bl.isPlayerPkmSummoned();
        if(fPkmSummoned.status == "1" && fRevive === 1){
          bl.revivePkm(pos.pkmSlot, pos.reviveSlot);
        }

        console.log("fish restarting");
        setTimeout(startFishing, 1000);
      }
    }); //end lock

  }); //focuscc.focusWIndow
}

// ================================


// ============  Battlling =====================

function checkChangeBattlelist(){

  console.log(battlePokelist);
  console.log(`${Object.keys(battlePokelist).length>0} ${fLookForFighting}`);

  if((Object.keys(battlePokelist).length>0)&&(fLookForFighting == 0)){
    fLookForFighting = 1;
    console.log("TIME TO FIGHT");
    lookForFighting(); //lookForFighting recall changeBattlelist after finished
  }
}

// search for a pokemon arround the player position to click on it (to fight), after it. recall
// can target only one pokemon per call
async function lookForFighting(){
  let i, ftargetFound=0, fcheckChangeBlRestarted=0;
  let currentBlist = Object.keys(battlePokelist);
  let lengthBl = currentBlist.length;
  let blCount = 0;

  fightCounter+=1;
  console.log(`\ncalls to fight: ${fightCounter}`);
  console.log("--> lookForfighting started");

  for (const iBl of currentBlist){
    blCount+=1;
    if(battlePokelist[iBl]!= 0){
      //check if pokemon in battlePokelist is a target of searchPokeArr
      //if it is: check if its near to attack
      //if not: remove it
      for(i=0; i<searchPokeArr.length; i++){
        if(battlePokelist[iBl].name == searchPokeArr[i]){
          if(fcheckChangeBlRestarted == 1){
            console.log("already fought and restarted function. finishing this call...");
          }else{
            //check if pokemon is near/close
            console.log(`1- checking if ${battlePokelist[iBl].name} is near`);
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
                  console.log("2-fake target. was deleted during processing. will restart as lookForFighting in the doneTarget()");
                }
                //  console.log("lock2 fighting acquired!");
                bl.attackPkm(battlePokelist[iBl].addr, function(res){
                    done();
                });
              }, function(err, ret){
                // lockmouse released
                console.log("2- lockmouse attack released! [pokemon selected]");
              });

              bl.waitForDeath(battlePokelist[iBl].addr, function(res){
                console.log("3- pokemon is dead.");
                battlePokelist[iBl] = 0;
  //              if((fcheckChangeBlRestarted == 0)&&(blCount == lengthBl)){
                  //after pokmon dead, force a call to lookForfight to check if there is still
                  //other pokemons to target
  //              }
                donetarget();
              });
              }, function(err, ret){
                fcheckChangeBlRestarted = 1;
                /*
                console.log("lookforfighting finished [locktarget]");
                console.log(`is locktarget busy again? ${locktarget.isBusy()}`);
                if(!locktarget.isBusy()){
                }
                */
              });
            }else{ //res.near else
              //it was dead by other player, disappeared of screen or dead by a area skill
              console.log("Pokemon NOT close");
              battlePokelist[iBl]=0;
              //in case program didnt delete it or it was dead by another player
              //faraway from player screen
            }
          }

        //else check if reach last element of array to remove it from battlelist
        }else if(i == (searchPokeArr.length-1)){
          delete battlePokelist[iBl];
        }
      }
    }
  }

  //no pokemons in the battlePokelist = restart check
  if((ftargetFound == 0) && (fcheckChangeBlRestarted == 0)){
    //no target found: target not in the battle list
    console.log("lookforfighting finished [notargetfound]");
  }

  fLookForFighting = 0;
}

// ============  Auxiliar Functions =====================

//sqm's in the screen: 15x11
function updateScreenCoords(res){
  console.log(res);

  //res = { x: 970, y: 136, w: 442, h: 324 }

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

  console.log("Center: "+center.x+"x"+center.y);

  console.log("Screen Coords captured");

  bl.setScreenConfig(center, sqm, function(){
  });
}