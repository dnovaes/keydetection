# Bobito's App

Virtual Assistant Application for a specific game (Tibia Platform) for Learning Purposes in Security Area involving:
- Code Injection
- Memory Reading
- Intercomunication between client and a external program.

App Requisites:
- Windows ONLY
- Cliente Dx9 version

# Installation

Install nodejs from the official website (6.11+, with npm of course)
Install as global, node-gyp and windows-build-tools for python2.7 and Microsoft Visual C++ DevLib
```
npm install -g node-gyp
npm install -g --add-python-to-path --production windows-build-tools
```
if have already python installed in your pc, remove '--add-python-to-path' from last command before execute.

git clone this project and install dependencies
```
git clone https://github.com/dnovaes/keydetection.git
cd keydetection/
npm install
```

Now the magic as the last step, rebuild your native modules for electron to work even with your other
native modules (C++ in this case). And start the application.
```
./node_modules/.bin/electron-rebuild.cmd
npm start
```

Note: If you're using Linux Bash for Windows, [see this guide](https://www.howtogeek.com/261575/how-to-run-graphical-linux-desktop-applications-from-windows-10s-bash-shell/) or use `node` from the command prompt.

# NOTES

- if you are going to run a node file and not use the electron app, you have to compile the binaries of the
native modules using the code below
```
node-gyp configure
node-gyp build
```

if you change your resolution or the game resolution, you need to reconfigure the screen coordinates by clicking in the
monitor button again.

# Packaging and Compiling own version

Install electron-packager. I build my version using NPM 5.4.1 > 5.6.0.
```
npm install electron-packager -g
```

Go to source project folder and run:

```
electron-packager ./ BobitoBot --platform=win32 --arch=x64 --asar=true
electron-packager ./ BobitoBot --platform=win32 --arch=ia32 --asar=true --prune=true
```

## Resources for Learning Electron

- [electron.atom.io/docs](http://electron.atom.io/docs) - all of Electron's documentation
- [electron.atom.io/community/#boilerplates](http://electron.atom.io/community/#boilerplates) - sample starter apps created by the community
- [electron/electron-quick-start](https://github.com/electron/electron-quick-start) - a very basic starter Electron app
- [electron/simple-samples](https://github.com/electron/simple-samples) - small applications with ideas for taking them further
- [electron/electron-api-demos](https://github.com/electron/electron-api-demos) - an Electron app that teaches you how to use Electron
- [hokein/electron-sample-apps](https://github.com/hokein/electron-sample-apps) - small demo apps for the various Electron APIs

## License

[CC0 1.0 (Public Domain)](LICENSE.md)
