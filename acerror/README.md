# acerror

A small pygame popup that displays an error message. It is meant to be
launched by `ace` when something in the config file cannot be understood,
so the user gets a visible warning instead of a silent failure.

## Usage

```
acerror "message to display"
```

If no message is given, it falls back to "Unknown error". The window
closes on `Enter`, `Esc`, or the window close button.

## Building

```
make
```

This produces an executable file named `acerror` in this directory.

## Dependencies

- python3
- pygame

Install pygame with:

```
pip install pygame
```

or, on Arch Linux:

```
sudo pacman -S python-pygame
```
