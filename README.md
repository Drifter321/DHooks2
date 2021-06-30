# DHooks 2
A SourceMod extension to hook any game function from within a SourcePawn plugin.
This fork adds support to dynamically detour any functions instead of only virtual functions.

Check out the [release post](https://forums.alliedmods.net/showthread.php?p=2588686#post2588686) for details on how to use this in a plugin.

## Installation
1. Grab the [latest release package](https://github.com/peace-maker/DHooks2/releases/latest).
2. Extract the package and upload it to your gameserver.
3. ???
4. Profit.

## Build instructions
To build the extension yourself, run the following commands in a shell.
To build on Windows, you have to have Visual Studio 2015+ installed. The dependency setup part is identical to the [building instructions of SourceMod](https://wiki.alliedmods.net/Building_SourceMod) itself.

```bash
# On e.g. Debian 10
sudo apt install build-essential clang gcc-multilib g++-multilib git python3 python3-pip
cd somefolderforbuildenvironment

git clone --recursive https://github.com/alliedmodders/sourcemod.git
# We don't use any HL2SDKs, so limit the download to 1 SDK.
sourcemod/tools/checkout-deps.sh -s csgo
cd sourcemod
# Switch to SourceMod 1.10
git checkout --recurse-submodules 1.10-dev
cd ..

git clone https://github.com/peace-maker/DHooks2
cd DHooks2
mkdir build
cd build
CC=clang CXX=clang++ python ../configure.py --enable-optimize
ambuild
```

The extension archive structure will be in the `build/package` folder.
