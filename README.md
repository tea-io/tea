# tea
Tea is a simple network file system for mainly editing source code with support for LSP (Language Server Protocol) executed on the server side.

## Features
* **File System**: Tea provides a network file system, enabling you to edit files remotely on the server side.
* **Language Server Protocol (LSP)**: Tea supports LSP, executed on the server side, allowing for enhanced code editing features such as autocompletion, error checking, and more.
* **Security**: Tea uses TLS (Transport Layer Security) to ensure encrypted and secure communication between the client and server.

## Build
We provide two ways to build Tea: building from source code and using Nix.
The main challenge with building from source code is ensuring that you have the same version of Protocol Buffers (shared libraries) installed on your machines.
Therefore, our suggestion is:

* **Nix**: (Recommended) A safer option, but requires Nix to be installed on your machine.
* **Building from source code**: If you already have the same version of Protocol Buffers installed on your machines.

### Nix
#### Requirements

[Install Nix](https://nixos.org/download.html) on your machines and enable Nix Flakes:
```bash
echo experimental-features = nix-command flakes >> ~/.config/nix/nix.conf
```
#### Build and Install
Building and install filesystem and server.
```bash
nix develop --install
```
### Building from source code
#### Requirements
* Protocol Buffers C++ library and Protocol Buffers C++ compiler
* libfuse3 
* Makefile
* pkg-config
* g++

#### Build
```bash
make
```
To build the server or the filesystem separately, you can use the following targets: `server` or `filesystem`.

#### Install
```bash
make install
```
To install the server or the filesystem separately, you can use the following targets: `install-server` or `install-filesystem`.

## TLS preparation

To generate key/certificate pair for server:

```bash
make cert

```

To generate key and certificate signed by server for client:

```bash
make client-cert
```

## Usage
### Server
```bash
tea-server -c=server-certificate -k=server-key project-directory-path
```

### Filesystem
```bash
tea-fs -h=server-host -c=client-certificate -k=client-key mount-point
```
To unmount the filesystem, use the following command:
```bash
umount mount-point
```
For advanced configuration of the filesystem, see help:
```
 tea-fs --help

 _                __
| |_ ___  __ _   / _|___
| __/ _ \/ _` | | |_/ __|
| ||  __/ (_| | |  _\__ \
 \__\___|\__,_| |_| |___/

usage: tea-fs [options] <mountpoint>


File-system specific options:
    -h   --host=<s>      The host of server (required)
    -p   --port=<d>      The port of server (default: 5210)
    -n   --name=<s>      The display name of user (default: login name)
    --help               Print this help

FUSE options:
    -h   --help            print help
    -V   --version         print version
    -d   -o debug          enable debug output (implies -f)
    -f                     foreground operation
    -s                     disable multi-threaded operation
    -o clone_fd            use separate fuse device fd for each thread
                           (may improve performance)
    -o max_idle_threads    the maximum number of idle worker threads
                           allowed (default: -1)
    -o max_threads         the maximum number of worker threads
                           allowed (default: 10)
    -o kernel_cache        cache files in kernel
    -o [no]auto_cache      enable caching based on modification times (off)
    -o no_rofd_flush       disable flushing of read-only fd on close (off)
    -o umask=M             set file permissions (octal)
    -o uid=N               set file owner
    -o gid=N               set file group
    -o entry_timeout=T     cache timeout for names (1.0s)
    -o negative_timeout=T  cache timeout for deleted names (0.0s)
    -o attr_timeout=T      cache timeout for attributes (1.0s)
    -o ac_attr_timeout=T   auto cache timeout for attributes (attr_timeout)
    -o noforget            never forget cached inodes
    -o remember=T          remember cached inodes for T seconds (0s)
    -o modules=M1[:M2...]  names of modules to push onto filesystem stack
    -o allow_other         allow access by all users
    -o allow_root          allow access by root
    -o auto_unmount        auto unmount on process termination

Options for subdir module:
    -o subdir=DIR           prepend this directory to all paths (mandatory)
    -o [no]rellinks         transform absolute symlinks to relative

Options for iconv module:
    -o from_code=CHARSET   original encoding of file names (default: UTF-8)
    -o to_code=CHARSET     new encoding of the file names (default: UTF-8)
```

### LSP support
To be able to use LSP features the LSP servers have to be configured. You can
configure which LSP server will be started for the given language in the
`$XDG_CONFIG_HOME/tea/config.json` file. The configuration file should look like this:
```json
{
  "languageConfigs": {
    "cpp": "clangd",
    "c": "clangd",
    "latex": "texlab"
  }
}
```

#### Visual Studio Code
To use LSP features in the Visual Studio Code IDE, you need to install the
extension for [Tea Integration](https://github.com/tea-io/tea.vscode). If you
want to run the extension from source, you have to have the `pnpm` installed,
then clone the repository and run the following commands:
```bash
pnpm install
pnpm vsce package --no-dependencies
```
After that, you can install the extension from the generated `.vsix` file.

#### Neovim
To use LSP features in Neovim, you need to install a plugin for [Tea Integration](https://github.com/tea-io/tea.nvim).
To enable the plugin, you have to add the plugin location to the `runtimepath` in your `init.lua` file:
```lua
vim.opt.runtimepath:prepend(</path/to/tea-nvim>)
```
