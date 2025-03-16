# genpass

genpass is a very simple password generator written in C.
It reads data from */dev/(u)random* and outputs it in a human-readable form.

# Features

- Generating multiple passwords at once.
- Specifying password length.
- Writing to a file directly.
- Specifying the character set to use:
  - Uppercase letters
  - Lowercase letters
  - Special characters
  - Digits
  - Any combination of the above
  - 64 characters (for no modulo bias)
- No rejection sampling.

# Installation

Run the following as root:
```bash
make clean install
```
