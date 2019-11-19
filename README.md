# Networks Project 04 -- Pong

Authors:
* Blake Trossen (btrossen)
* Horacio Lopez (hlopez1)

## Getting Started
### Compilation
To compile, perform the following commands:
```
$ make
```

### Running
Below are the execution formats of the generated executable for each respective player:
#### Player 1 (Host)
```
$ ./netpong --host PORT
```
#### Player 2 (Challenger)
```
./netpong HOSTNAME PORT
```

### Example
Below is an example of how to run the generated executable for each player:
#### Player 1 (Host)
```
$ ./netpong --host 41045
```
#### Player 2 (Challenger)
```
./netpong student00.cse.nd.edu 41045
```
Note that the above example assumes that the two players are on different hosts. If player 1 and player 2 are on the same host, then different ports must be used.
