# ebin-scrot

This is a screenshot program. It can capture the entire screen, or a mouse
selection.

## Building

Dependencies:
- Xlib
- Xft
- Imlib2
- freetype2

Install dependecies with (arch based):

    pacman -S libx11 libxft imlib2 freetype2

The font dependencies are because of the suckless drw library

Build with

    make

and install with

    make install


## Running

Capture entire screen:

    ebin-scrot all

Capture selection. Click and drag to mark the area you want:

    ebin-scrot selection

I recommend binding both options to hotkeys.
