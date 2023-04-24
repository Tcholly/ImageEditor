# ImageEditor
This image editor works by cropping the image based on rows and columns, you can then combine diffrent pieces to make a bigger one and finally you can save the single pieces.

## Dependencies
This software uses:
- [Difu](https://github.com/Tcholly/Difu/tree/ECS)
    - [Raylib](https://www.raylib.com/)
    - [entt](https://github.com/skypjack/entt)
    - [fmt](https://github.com/fmtlib/fmt)
- [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended)

## Quick Start
This project uses [premake5](https://premake.github.io/) as the build system and this [ResourceManager](https://github.com/Tcholly/ResourceManager) to create Global variables

To build you should first edit the premake.lua file to have the correct path for the ResourceManager executable and the difu library. Then run the following commands:
```cmd
$ premake5 gmake2
$ make
$ ./bin/ImageEditor/Debug/ImageEditor
```
And you're ready to go.
For linux it should work fine, for other operating system you just need to change the dependencies to your os specific lib file.

## TODO-list
- [ ] ??
