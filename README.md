# Keydetection

program that detects key even if program isnt focused [temp]. Uses nodejs, C and electron to connect
both. (Windows ONLY)

# Installation

Install nodejs from the official website (6.11+, with npm of course)
Install as global, node-gyp and windows-build-tools for python2.7 and Microsoft Visual C++ DevLib
```
npm install -g node-gyp
npm install -g --production windows-build-tools
```

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

## Resources for Learning Electron

- [electron.atom.io/docs](http://electron.atom.io/docs) - all of Electron's documentation
- [electron.atom.io/community/#boilerplates](http://electron.atom.io/community/#boilerplates) - sample starter apps created by the community
- [electron/electron-quick-start](https://github.com/electron/electron-quick-start) - a very basic starter Electron app
- [electron/simple-samples](https://github.com/electron/simple-samples) - small applications with ideas for taking them further
- [electron/electron-api-demos](https://github.com/electron/electron-api-demos) - an Electron app that teaches you how to use Electron
- [hokein/electron-sample-apps](https://github.com/hokein/electron-sample-apps) - small demo apps for the various Electron APIs

## License

[CC0 1.0 (Public Domain)](LICENSE.md)
