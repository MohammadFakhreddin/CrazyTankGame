# How to build

Create a directory a build directory instead the project folder and change directory to that folder then execute following command:

## For make, ninja, etc

For release build:
```
cmake -DCMAKE_BUILD_TYPE=Release ..
```
For debug build:
```
cmake -DCMAKE_BUILD_TYPE=Debug ..
```
## For visual studio
```
cmake ..
```

You can use visual studio (recommended) or ninja on windows and make on linux to run the project. 
