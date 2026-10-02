# CRESCENT TUI
<p align="center">
  <img
    src="https://github.com/user-attachments/assets/d75c7b03-e96b-4d69-83a2-6ef6d00d0650"
    alt="Crescent TUI screenshot"
    width="1000"
  />
</p>

### This is the crescent terminal version made based on the public api provided by ascpixi and this project is being submitted to a great ysws crescent itself

<br>

<table align="center">
  <tr>
    <td align="center" valign="middle">
      <img
        src="https://github.com/user-attachments/assets/7f8a4173-404a-4a63-9e7e-f5fae6ba2d1a"
        alt="Crescent logo"
        width="230"
      />
    </td>
    <td align="center" valign="middle">
      <img
        src="https://github.com/user-attachments/assets/b1d820c0-1b0d-4bfa-a578-1d8a91b83b1f"
        alt="Low-level card"
        width="270"
      />
    </td>
    <td align="center" valign="middle">
      <img
        src="https://github.com/user-attachments/assets/9bab890d-f9e9-4765-813c-fc8397025eab"
        alt="Crescent mascot"
        width="210"
      />
    </td>
  </tr>
</table>

# [AI DECLARATION]: used for UI and bug fixing and no over use 

## Requirements

### Linux
- GCC or another C11-compatible compiler
- GNU Make
- `ncurses`
- `libcurl`
- Git

### Windows
- Windows x64
- For the prebuilt version, no compiler or libraries are required. The required DLLs are included in the Windows package.
- For building from source:
  - MinGW-w64 GCC
  - PDCurses
  - libcurl
  - GNU Make

### API Key

Crescent requires a **Crescent API key**.

The application can read it from:

```bash
CRESCENT_API_KEY
```

or from:

```text
~/.config/crescent-tui/config
```

with:

```text
api_key=YOUR_API_KEY
region=US
```

If no key is configured, Crescent prompts for one on startup.

---

## Installation

### Linux - Prebuilt

The repository includes a prebuilt x86-64 Linux binary.

```bash
git clone https://github.com/ba-sheer/crescenttt.git
cd crescenttt

tar -xzf release/crescent-tui-linux-x64.tar.gz
cd linux-x64

chmod +x crescent-tui
./crescent-tui
```

To install it system-wide:

```bash
sudo install -Dm755 crescent-tui /usr/local/bin/crescent-tui
```

Then:

```bash
crescent-tui
```

### Linux - Build from Source

#### Fedora

```bash
sudo dnf install gcc make ncurses-devel libcurl-devel
```

#### Debian / Ubuntu

```bash
sudo apt install gcc make libncurses-dev libcurl4-openssl-dev
```

#### Arch Linux

```bash
sudo pacman -S gcc make ncurses curl
```

Then build:

```bash
git clone https://github.com/ba-sheer/crescenttt.git
cd crescenttt

make
./crescent-tui
```

To install:

```bash
sudo make install
```

This installs the binary to:

```text
/usr/local/bin/crescent-tui
```

### Windows - Prebuilt

The repository includes a complete Windows x64 package with the required DLLs.

```powershell
git clone https://github.com/ba-sheer/crescenttt.git
cd crescenttt
```

Extract:

```text
release/crescent-tui-windows-x64.zip
```

Then run:

```powershell
crescent-tui.exe
```

**Keep the DLL files in the same directory as `crescent-tui.exe`.**

### Windows - Build from Source

Using an MSYS2 MinGW-w64 environment, install the required toolchain and libraries, then:

```
git clone https://github.com/ba-sheer/crescenttt.git
cd crescenttt

make windows
```
### Build Commands

```
make              # Build for Linux
make windows      # Build for Windows
make clean        # Remove Linux build files
make clean-windows
sudo make install # Install Linux binary
```
