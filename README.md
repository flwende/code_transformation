# An Automated Proxy-Type Generator and Source-to-Source Code Transformation Tool 
A standalone tool that draws on (Clang) LibTooling for source code analysis and proxy-type generation for structured data types.

## Build
Create directories `obj` and `bin` and run the Makefile: `make clean && make`

### Dependencies
LibTooling (Clang version 8.0 or newer)

## Execute
Important: in case of something goes wrong, make a backup of your source code beforehand!

The generator tool processes all source files given as arguments.
If any of these files introduces library dependencies you need to prepend the include path of your environment, e.g. for our `example` program

```
$> WORKING_DIR=`pwd`
$> export CPLUS_INCLUDE_PATH=$WORKING_DIR/example/include:$CPLUS_INCLUDE_PATH
```

All modifed source files will be written into a separate output directory, which should be specified through the environment variable `CODE_TRAFO_OUTPUT_PATH`

```
$> export CODE_TRAFO_OUTPUT_PATH=$WORKING_DIR/example/new_files
```

Run the tool

```
$> ./bin/test_proxy_gen.x example/main.cpp
```

All modified source files can be found in `CODE_TRAFO_OUTPUT_PATH`.
