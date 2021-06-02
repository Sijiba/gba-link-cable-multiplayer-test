cd gba
make clean
make
cd ..
mkdir data
mv -f gba/gba_mb.gba data/gba_mb.gba
start "No$GBA" "C:\Users\sjber\Saved Games\Emulation\Emulators\No$gba debugger\NO$GBA.EXE" "C:\Users\sjber\Code\GitHub\gba-link-cable-dumper\data\gba_mb.gba"
