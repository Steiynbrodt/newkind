# Building Elite: The New Kind on Windows 10

These steps target a clean Windows 10 install using the MSYS2 toolchain and the Allegro 5 monolith build that the game expects.

## Prerequisites
1. Install [MSYS2](https://www.msys2.org/) and open the **MSYS2 MinGW 64-bit** shell.
2. Update packages:
   ```bash
   pacman -Syu
   ```
   Restart the shell if prompted, then run `pacman -Syu` again until no more updates are offered.
3. Install the toolchain and Allegro 5 monolith package:
   ```bash
   pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-allegro
   ```

## Building
1. Ensure the MinGW bin directory is first on your `PATH` (the MSYS2 MinGW shell does this automatically).
2. From the repository root, run:
   ```bash
   mingw32-make
   ```
   The default `makefile` looks for Allegro headers and libraries under `C:/msys64/mingw64`. If you installed MSYS2 elsewhere, override the location:
   ```bash
   mingw32-make ALLEGRO_DIR=C:/path/to/your/msys64/mingw64
   ```
3. The build should produce `newkind.exe` in the repository root.

## Running
1. Make sure the `data/` directory and the `.wav` assets are in the same folder as `newkind.exe`.
2. Double-click `newkind.exe` from Explorer or run it from the MSYS2 shell:
   ```bash
   ./newkind.exe
   ```

If Allegro DLLs are missing when you run the game, add `C:/msys64/mingw64/bin` to your `PATH` or copy the required `allegro*.dll` files next to `newkind.exe`.
