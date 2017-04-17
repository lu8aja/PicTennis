# PicTennis - Tennis for Two implemented with a PIC

![Oscilloscope screen](docs/screen.jpg?raw=true)

PIC implementation of the seminal game "[Tennis for Two](https://en.wikipedia.org/wiki/Tennis_for_Two)" from
1958.

The project is written in C with the C18 compiler from Microchip and features a
very simple PIC18F258, with 2 DACs with R2R resistors to produce an XY
signal to be fed to the oscilloscope. To play you need appropriate controls aside of the board
consisting of a potentiometer and a switch for each user.

The game comes with 4 modes:
* Mode 0: Display mode - The PIC plays alone automatically hitting randomly the ball
* Mode 1: 2P Original - No rules enforced, remember the game was with analog discrete components
* Mode 2: 2P with Rules - A more usable version enforces certain rules to avoid locks and so on
* Mode 3: 1P with Rules - You play against the PIC

The game features a debug mode, to toggle it you need to fulfill ALL of these conditions:
* Be in MODE 1
* Have a new ball on the LEFT side
* Both players' angle be at 0 (This can be tricky as the angle can be negative)
* Both players hit the button

The idea behind such esoteric combination is to precisely be hard to hit on normal usage.
The debug mode will display the variables as 7 segment numbers at the top of the oscilloscope screen.

Enjoy!



Disclaimer 0: (Yeah I am a 0-based coder) This was put up in a single night rush to get it
done in time for a presentation, don't be too harsh judging the code ;)

Disclaimer 1: The PCB was made using a universal board, so no schematics... sorry.
If I happen to open the case to fix it some day I will make one. In the meanwhile, you should be able
to deduce what is connected to what from the top configs.

Disclaimer 2: I may have borrowed code from many places, if you notice I blatantly stole your code,
 lmk and I will put a note thanking you for it ;p
