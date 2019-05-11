# Strong Live Variable Analysis in LLVM

## Liveness of a variable

A variable is said to be live at some point if it holds a value that may be needed in the future, or equivalently if its value may be read before the next time the variable is written to.

```sh
ln 1: y = a
ln 2: a = 9
```

In line 1 a is live as it uses it's previous defination, and if a is not used after it's re-defination in line 2. We can say a is not live after ln 2.
But if y is not live after ln 1 ie y = a is a dead statement and hence ln 1 should be considered as a use of a, which leads to a not being strongly live before ln 2.

## Setup

```sh
$ clone the repo
$ cd ~/where-ever-you-cloned-it/
$ export LLVM_HOME=your-llvm-8-home
$ cp /tests/your-fav-test.c test.c
$ bash ./test
```
