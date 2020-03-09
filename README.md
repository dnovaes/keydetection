# Bobito's App

Virtual Assistant Application for Tibia OtServers. This project is for learning purposes in Security Area involving:

- Code Injection
- Memory Reading
- Intercomunication between exe client and a external program without using APIs
- Automated Tasks (Bot alike)

App Requisites:
- Windows ONLY
- Client running in Dx9 version (Memory values are changed in different versions)

## Installation

1 - Install Nodejs from the official website **Node.js 12.12.0, NPM 6.11.3, Node-gyp 6.1.0** 
https://nodejs.org/download/release/v12.12.0/node-v12.12.0-x64.msi 

~~Node-gyp, Windows-build-tools for python2.7 and Microsoft Visual C++ DevLib using the command bellow.~~

Don't forget to execute CLI of the windows as admninistrator power. Do not use git bash or other unix substitute for windows to run this.

2- node-gyp and windows-build-tools
```
npm install -g node-gyp@6.1.0
npm install -g --add-python-to-path='true' --production windows-build-tools
```

After receiving the message of success. Like this one

![success-install](https://i.imgur.com/Z6ITFwb.png)

Last Version checked: **2.7.15 -> python-2.7.15.amd64.msi** 

Wanna check if package was installed globally by npm?\
```npm list -g windows-build-tools```

Wanna check if python is added in path?

The enviroment path variables at windows stays at the global variable "PATH". To add python variable to path, check first where is your python2.7 path, go to control painel of your windows and add the path at the end of PATH variable as the picture below shows.

![python27-path](https://i.imgur.com/gaVdnMA.png)

git clone this project and install dependencies
```
git clone https://github.com/dnovaes/keydetection.git
cd keydetection/
npm install
```

If needed you can set your own path to your node-gyp version\
```npm config set node_gyp "node .\node_modules\node-gyp\bin\node-gyp.js"```

## Running the App

Rebuild your native modules for electron to work with the proper version of node-gyp and start the application.
```
npm run electron-rebuild
npm start
```
or if the code above didnt work:
```
./node_modules/.bin/electron-rebuild.cmd
npm start
```
Note: If you're using Linux Bash for Windows, [see this guide](https://www.howtogeek.com/261575/how-to-run-graphical-linux-desktop-applications-from-windows-10s-bash-shell/) or use `node` from the command prompt.

## Issues

1 - if you still have any issues with python enviroment variables, check this thread: https://github.com/felixrieseberg/windows-build-tools/issues/56
Also a good could be reseting ur config python variables at npm and reinstall everything. To reset python config:

```
npm config set python ""
```

2 - 2 - If you are having issues with windows node modules, check their project README. Most of time it is probably due to different node versions between electron-rebuild node version and your system node version.\
https://github.com/electron/electron-rebuild

TDLR: Try to update ur electron-rebuild package (npm install --save-dev electron-rebuild) or reinstall a diff node version. Remember to always execute 
```
./node_modules/.bin/electron-rebuild.cmd
```
before ```npm start```

2 - if you are having trouble with running node-gyp alone with a js file (node test.js) maybe your node-gyp variable path is not set properly.

There is a couple of places where node-gyp might be installed, here are some:

```
C:\Program Files\nodejs\node_modules\npm\node_modules\npm-lifecycle\node-gyp-bin;
C:\Program Files\nodejs\node_modules\npm\bin\node-gyp-bin;
C:\Users\USERNAME\AppData\Roaming\npm\node_modules\node-gyp\bin\node-gyp.js
C:\Users\USERNAME\.electron-gyp\6.1.9\include\node
```

The last one is the correct path including already the headers used for build. But for them to appear in the specified folder you should type `npm run electron-rebuild` first. This will execute `node-gyp configure & node-gyp build` using the lib version of electron set in the package.json.

## Packaging and Compiling own version

Install electron-packager
```
npm install -g electron-packager@14.2.1
```

Go to source project folder and run:

```
if using windows 64bits on host:
electron-packager ./ BobitoBot --platform=win32 --arch=x64 --asar

if using windows 32bits on host
electron-packager ./ BobitoBot --platform=win32 --arch=ia32 --asar --prune=true
```

## Resources for Learning Electron

- [electron.atom.io/docs](http://electron.atom.io/docs) - all of Electron's documentation
- [electron.atom.io/community/#boilerplates](http://electron.atom.io/community/#boilerplates) - sample starter apps created by the community
- [electron/electron-quick-start](https://github.com/electron/electron-quick-start) - a very basic starter Electron app
- [electron/simple-samples](https://github.com/electron/simple-samples) - small applications with ideas for taking them further
- [electron/electron-api-demos](https://github.com/electron/electron-api-demos) - an Electron app that teaches you how to use Electron
- [hokein/electron-sample-apps](https://github.com/hokein/electron-sample-apps) - small demo apps for the various Electron APIs

## License

Copyright by dnovaes.
