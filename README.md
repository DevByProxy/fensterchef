# Fensterchef – The X11 Tiling Window Manager for Linux

Fensterchef is a lightweight, lightning-fast window manager for Linux, focused on manual tiling.

🔹 Manual Tiling: Arrange your windows exactly how you want — no rigid grids or enforced layouts. </br>
🔹 Lightweight & Fast: Minimal overhead ensures smooth performance, even on low-end hardware. </br>
🔹 Highly Customizable: Configure Fensterchef easily with a simple configuration file. </br>
🔹 Keyboard-Centric: Navigate your workspace effortlessly with intuitive shortcuts. </br>
🔹 **Modern C++**: Recently modernized to C++17 with Object-Oriented Programming principles. </br>

## Gallery

![fensterchef](./images/fensterchef.png)
![fensterchef](./images/fensterchef2.png)

## Installation

Get started immediately! Open a terminal and clone the repository:
```sh
git clone https://github.com/JulianBMW/fensterchef.git
```
Then simply type the following and enter your password.
```sh
sudo make install
```

Now you have the **fensterchef** executable (`/usr/bin/fensterchef`)
and the manual page (`/usr/share/man/man1/fensterchef.1.gz`).

If you are using a login manager, you can simply put this at the end of your `~/.xsession`:
```
mkdir -p ~/.local/share/fensterchef
exec /usr/bin/fensterchef -dinfo 2>~/.local/share/fensterchef
```
Alternatively put it this into the `~/.xinitrc`.

*How to get fensterchef to run exactly varies on your environment.*

## Development

### Modern C++ Architecture

Fensterchef has been modernized to use C++17 with Object-Oriented Programming principles. Key improvements include:

- **RAII**: Automatic resource management through constructors/destructors
- **Type Safety**: Strong typing with `enum class`, `nullptr`, and `constexpr`
- **OOP**: Core components (Window, Frame, Monitor) are now proper C++ classes
- **Memory Safety**: Replaced manual memory management with C++ `new`/`delete`

For detailed information about the modernization, see [MODERNIZATION.md](MODERNIZATION.md).

## Bugs

Report any issues directly to us over the Github issues tab.

An issue should start with the version, the rest is up to you. Try to add steps
to reproduce and the relevant excerpts from the log.
