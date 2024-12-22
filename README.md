# chip8-emu
A CHIP-8 emulator developed for educational purposes

## TODOs
 1. [ ] Test with [CHIP-8-OOB-Test](https://github.com/Estus-Dev/CHIP-8-OOB-Test/)
 2. [ ] Fix the keyboard input
   - [ ] XSet message
   - [ ] Better keyboard timer

 - [x] A "debug" version that only displays the current cpu context, without no graphics and no sound
    - With this version, I hope to run a simple "Sum of first n natural numbers" program
    - This version will only implement single-stepping
 - [ ] Check for terminal size (see ioctl())
 - [x] Run the [Octojam 1 Title](https://johnearnest.github.io/chip8Archive/play.html?p=octojam1title)
 - [x] Implement a flash or beep
 - [x] Optimize the DRW instruction
 - [ ] Make a assembler and a disassembler
 - [ ] Add the "safe mode" (no writing to stack...)
 - [ ] Add the extended instruction set
 - [ ] Add more resolutions
 - [ ] Make a better debugger
