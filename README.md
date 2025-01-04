# chip8-emu
A CHIP-8 emulator developed for educational purposes

## Notes
 - [Timendus/chip8-test-suite](https://github.com/Timendus/chip8-test-suite): Passes all tests except
   - Quirks Test: Display wait
   - Keypad Test: ``Fx0A`` GETKEY (displays NOT HALTING)
   - Scrolling Test: ``0x00Cn`` (SCD) instruction doesn't behave like it should under __legacy__ mode
 - ``0x00FF`` (HIGH) and ``0x00FE`` (LOW) instructions don't behave like described [here](https://github.com/Chromatophore/HP48-Superchip/blob/master/investigations/quirk_display.md)
 - Flag registars aren't persistent, like described [here](https://johnearnest.github.io/Octo/docs/SuperChip.html)


## TODOs
 - [ ] Quirk menagment


 - [x] A "debug" version that only displays the current cpu context, without no graphics and no sound
    - With this version, I hope to run a simple "Sum of first n natural numbers" program
    - This version will only implement single-stepping
 - [ ] Check for terminal size (see ioctl())
 - [ ] Better border (sometimes display clips the border)
 - [x] Run the [Octojam 1 Title](https://johnearnest.github.io/chip8Archive/play.html?p=octojam1title)
 - [x] Implement a flash or beep
 - [x] Optimize the DRW instruction
 - [x] Test with [Timendus/chip8-test-suite](https://github.com/Timendus/chip8-test-suite)
 - [x] Fix the keyboard input
   - [x] XSet message
   - [x] Better keyboard timer
 - [x] Add the extended instruction set
 - [ ] Make a assembler and a disassembler
 - [ ] Add the "safe mode" (no writing to stack...)
 - [ ] Add more resolutions
 - [ ] Make a better debugger
 - [ ] Add adjustable clock speed
