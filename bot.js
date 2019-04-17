
const bl = require("../build/Release/battlelist");
const keyboard = require("../build/Release/keyboard");
const focuscc = require("../build/Release/focus");
const mouse = require("../build/Release/mouse");
const AsyncLock = require('async-lock');

//global vars
//var win = remote.BrowserWindow.fromId(1);
var battlePokelist = {};
var searchPokeArr= ["Magikarp", "Rattata"];
var lock = new AsyncLock();
var locktarget = new AsyncLock();
var moduleAddr = 0;
var pid = 0;
var fRevive = 0;
var fPause = false;
var lastEntAppeared = {}

var center = {
  "x": 0,
  "y": 0
}
var sqm = {
  "length": 0,
  "height": 0
}
var pos = {
  pkmSlot: { "x":1200, "y":182 },
  reviveSlot: { "x":1180, "y":182 }
}
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
var gmAudio = new Audio('../assets/Pkm RedBlueYellow - Evil Trainer Encounter.mp3');
var playerAudio = new Audio('../assets/Pkm RedBlue Wild Battle Music.mp3');

//first function to run. It gets moduleaddress and pid of the program too
bl.getBattleList(function(res){
  let i;

  if(res.fAction == 2){
    let creatureName = res.name;

    console.log(res);
    //Support Member 
    if(res.name.includes("Support")){
      gmAudio.play();
      printlog(`\n\nSupport Member!\n\n`);
    }else{

      switch(res.type){
        case 152:
          res.type = "NPC";
          break;
        case 7: //myself
        case 56:
          res.type = "PLAYER";
          playerAudio.play();
          break;
        //case 112:
        case 244:
          res.type = "POKEMON";
          if((battlePokelist[res.addr] === 0)||(battlePokelist[res.addr] === undefined)){
            for(i=0;i<searchPokeArr.length;i++){
              if(searchPokeArr[i] == res.name){
                battlePokelist[res.addr] = {
                  "addr": res.addr,
                  "name": res.name,
                  "type": res.type,
                  "lookType": res.lookType,
                  "counterNumber": res.counterNumber
                }
              }

              if((creatureName === "Machamp")&&(res.lookType != 121)){
                //https://i.imgur.com/hDB7e46.png/ shiny machamp looktype 26
                audio.play();
                printlog(`\nShiny Machamp!\n`);
              }else if((creatureName === "Farfetch'd") && (res.lookType == 8)){
                audio.play();
                printlog(`\nElite Farfetch'd!\n`);
              }else if((creatureName === "Gengar") && (res.lookType != 244)){
                audio.play();
                printlog(`\nShiny Gengar!\n`);
              }else if((creatureName === "Pidgeot") && (res.lookType != 80)){
                audio.play();
                printlog(`\nShiny Pidgeot!\n`);
              }else if(creatureName === "Feebas"){
                audio.play();
                printlog(`\nFeebas!\n`);
              }
            }
          }
          break;
      }
    }

    lastEntAppeared.name = res.name;
    lastEntAppeared.addr = res.addr;
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
//'=' key = pause, previously the f10 key
bl.registerHKF10Async(function(res){
  fPause = !fPause;
  console.log(`Pause key detected!! (JS) Status: ${fPause}`);
  //bl.testread();
});

//delete key
bl.registerHkRevivePkm(function(res){
  /*if(fRevive === 1){
    console.log("REVIVE key detected!!");
    focuscc.focusWindow(function(res){
      bl.revivePkm();
    });
  }*/
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
    alert(`Antes de iniciar, configure a coordenadas da tela do seu cliente pelo bot`);
  }
});

//default Pc Lukas
bl.registerPkmSlot(pos.pkmSlot);

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
        bl.registerPkmSlot(pos.pkmSlot);
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

        //get game screen coords
        /*
        sharexNode.getScreenResolution(function(res){
          updateScreenCoords(res);
          fBtnScreenCoords = 0;
        });
        */
        bl.getScreenGamePos(function(res){
          //res = { x: 970, y: 136, w: 442, h: 324 }
          //updateScreenCoords(res);
          console.log(`getScreenGamePos finished`);
          if(res){
            updateScreenCoords(res);
            divScreenCoords.style.background = 'linear-gradient(#774247, #f7e53f)';
          }
          //#5db31c = green
        });
      });
    }else{
      updateScreenCoords({x:0, y:0, w:0, h:0});
      divScreenCoords.style.background = 'linear-gradient(red, #774247)';
      fBtnScreenCoords = 0;
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
  bl.getPlayerPos(function(res){
    newCommand.innerHTML = `<div class="sel-text">${res.posx},${res.posy},${res.posz}</div><div class="remove-el"></div>`;
    newCommand.addEventListener("click", highlightCommand);
    newCommand.children[1].addEventListener("click", removeCommandEl);
    addCommandToList(newCommand);
  });
});

// ============  Fishing  =====================

function prepareForFishing(){
  if(fPause == false){
    console.log("start Fishing!");
    divFishing.style.background = 'linear-gradient(#774247, #f7e53f)';
    startFishing();
  }else{
    console.log("Fishing Pause l-244!");
    divFishing.style.background = 'linear-gradient(red, #774247)';
  }
}

function startFishing(){
  var coords = {"x":0, "y":0};
  let fishStatus = 2; //normal pos
  let fPkmSummoned;

  //select game window
  //TODO: add focusWindow in every keyboard action later
  focuscc.focusWindow(function(res){

    //test if player's pokemon is out of pokebal.. if not use revive and summon it //sync function
    /*
    fPkmSummoned = bl.isPlayerPkmSummoned();
    console.log(`is pokemon summoned? ${fPkmSummoned.status}`);
    if(fPkmSummoned.status == "1" && fRevive === 1){
      //bl.revivePkm(pos.pkmSlot, pos.reviveSlot);
      bl.revivePkm();
    }
    */

    //move mouse 2 sqm distance to the top from the caracter
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

      /*
      //test if player's pokemon is out of pokebal.. if not use revive and summon it //sync function
      fPkmSummoned = bl.isPlayerPkmSummoned();
      if(fPkmSummoned.status == "1" && fRevive === 1){
        bl.revivePkm();
      }
      */
    }, function(err, ret){
      console.log("lockmouse released");
      //test if player's pokemon is out of pokebal.. if not use revive and summon it //sync function
      fPkmSummoned = bl.isPlayerPkmSummoned();
      if(fPkmSummoned.status == "1" && fRevive === 1){
        bl.revivePkm();
      }

      //if player started fishing (3), wait for pokemon to get the fish, if not, restart fishing
      if(fishStatus != 2){
        //wait for the change of color, press CTRL+Z when the fish appears
        console.log("Waiting for fish to fish....");

        let fish;

        mouse.getColorFishing({
          "x": coords.x, "y": coords.y
        }, pid, moduleAddr, function(res){
          if(!fPause){
            keyboard.pressKbKey("Fishing", function (res){
              console.log("Fishing Rod Pulled Up!!");
              //end of fishing

              //test if player's pokemon is out of pokebal. if not, use revive and summon it //sync function
              fPkmSummoned = bl.isPlayerPkmSummoned();
              if(fPkmSummoned.status == "1" && fRevive === 1){
                bl.revivePkm();
              }
              //IF pause is not requested, continue Fishing
              //restart fishing after some time (this time is necessary:
              //time: w8 for fishing sqm end animation of fished up to
              //"available to fish here"
              setTimeout(startFishing, 1000);
            }); //keyboard. CTRLZ 2
          }else{
            console.log("Fishing finished");
            divFishing.style.background = 'linear-gradient(red, #774247)';
          }

        }); //mouse.getColorFishing

      }else{
        //test if player's pokemon is out of pokebal.. if not use revive and summon it //sync function
        /*
        fPkmSummoned = bl.isPlayerPkmSummoned();
        if(fPkmSummoned.status == "1" && fRevive === 1){
          bl.revivePkm();
        }
        */

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
            let res = bl.isPkmNearSync(battlePokelist[iBl].addr);
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

function sleep(ms){
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function sleepFor(ms){
  console.log(`sleep For started ${ms}`);
  await sleep(ms);
  console.log(`sleepFor finished ${ms}`);
  if(fPause){
    sleepFor(ms);
  }
}