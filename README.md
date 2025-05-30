
[![Run FileOp CI](https://github.com/ZF-Group/FileOp/actions/workflows/CI.yml/badge.svg?branch=main)](https://github.com/ZF-Group/FileOp/actions/workflows/CI.yml)

# FileOp

Tool for general file operations under Windows with support of file names longer than MAX_PATH (260 characters).

## Background

Windows command line tools only support paths with a maximum length of 260 characters.
As workaround you need to subst the directory to a drive letter and delete the sub
tree from there.

## Usage

All supported commands are compiled into a single executable `FileOp.exe`. To get a list of commands
and the available options you can execute `FileOp.exe --help`.

## Development

For development a workspace for `Visual Studio Code` is configured together with a cross compiler
running under docker.

## Directory layout

| dir       | description                         |
| --------- | ----------------------------------- |
| `build`   | *ignored*: Storage of build results |
| `scripts` | Storage for source files            |
| `src`     | Storage for source files            |

### Build

The project uses CMake and ninja for building the executable. The CMake configuration step is executed by
calling [scripts/cmake.configure.sh](./scripts/cmake.configure.sh) and the build by calling
[scripts/cmake.build.sh](./scripts/cmake.build.sh). In the status bar of the IDE there are buttons to
execute the tools.

### Test

The test are also written in CMake and executed by calling [scripts/cmake.test.sh](./scripts/cmake.test.sh).
