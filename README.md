<div align="center">
  <img src="./assets/flightrunner.gif" alt="flightrunner" width="600px" />
  <p><em>chip8-emu running <a href="https://johnearnest.github.io/chip8Archive/play.html?p=flightrunner">Flight Runner</em></p>
</div>

# chip8-emu
A ncurses CHIP-8 emulator developed for educational purposes.

## Notes
 - **KEYBOARD INPUT IS TERRIBLE**
 - Implements a flash, not a buzzer
 - Modern ``0x00Cn`` (SCD) instruction (see [Scroll Test](https://github.com/Timendus/chip8-test-suite#scrolling-test))
 - ``0x00FF`` (HIGH) and ``0x00FE`` (LOW) instructions don't behave like described [here](https://github.com/Chromatophore/HP48-Superchip/blob/master/investigations/quirk_display.md)
 - Flag registars aren't persistent, like described [here](https://johnearnest.github.io/Octo/docs/SuperChip.html)
 - [Timendus/chip8-test-suite](https://github.com/Timendus/chip8-test-suite): Passes all tests except
   - Quirks Test: Display wait is always none
   - Keypad Test: ``Fx0A`` GETKEY (displays NOT HALTING)

## Installation and usage
The only depencency for the project is `ncurses`.
 1. Download the .tar.gz file from the [release page](https://github.com/miloje357/chip8-emu/releases).
 2. After extracting it and entering the project's directory, run these commands
 ```sh
mkdir build && cd build
../configure
make
 ```
 3. Now you have a `chip8_emu` binary in the `build` directory. Download any chip8 rom and run it with:
 ```sh
./chip8_emu <rom file>
 ```
 Also, a dissasembler is included:
 ```sh
./chip8_dasm <rom file>
 ```
 You may also, clone the repo, run `autoreconf` and do steps 2. and 3. as described above:
```sh
git clone https://github.com/miloje357/chip8-emu/
cd chip8-emu
autoreconf -i
```

## Bugs
 - [ ] [Black Rainbow](https://johnearnest.github.io/chip8Archive/play.html?p=blackrainbow) Sometimes teleports to homescreen when entering a room

## Acknowledgements
 - [wernsey/chip8](https://github.com/wernsey/chip8)
 - Documentation and other helpful sites:
   - [Cowgod's Chip-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
   - [Awesome CHIP-8](https://chip-8.github.io/links/)
   - [HP48-Superchip](https://github.com/Chromatophore/HP48-Superchip)
 - Testing: [chip8-test-suite](https://github.com/Timendus/chip8-test-suite)
 - ROMs:
   - [CHIP-8 Archive](https://johnearnest.github.io/chip8Archive/)
   - [CHIP-8 Roms](https://github.com/kripod/chip8-roms/)

