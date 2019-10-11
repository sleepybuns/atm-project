# Building the System

This document describes the various programs you will need to write for
this project. We have provided stub code for much of it, along with
some helper utility code (a hash table and a list). You are welcome to
use these, or to write your own, but the way that the code
communicates (the ATM and bank route their messages through the
router) must remain the same.

We also describe outputs for the program: make sure
that these match *exactly* (the outputs do not have periods at the
end, for instance) so that we can use automatic graders.

## Building the programs: what and how

From your submission directory, there should be a Makefile such that
when we run `make` it creates the following programs:

 * `bin/atm`
 * `bin/bank`
 * `bin/init`
 * `bin/router`

The `atm`, `bank`, and `router` programs must be implemented in
C. `init` can be a script or a C program. The stub code that we
provide contains the networking operations you will need. The router,
bank, and ATM all run on different ports on `localhost` (so you need
not even be connected to the Internet for this to work).

Your submissions must build with stack guard turned off
(`-fno-stack-protector`) and executable stacks (`-z execstack`) -- as
already specified in the Makefile provided. As you all know, this is
bad practice, and you should not do this outside of this class! But
it will require you to be all the more on your toes--and should make the
break-it phase of the project even more fun.

## Invoking the programs

The `init` and `router` programs can be run in either order, but both
must be run before `bank`, which in turn must be run before `atm`. The
order of these will be clear from what they create.

 * The `init` program is described in [init.md](init.md).
 * The `bank` program is described in [bank.md](bank.md).
 * The `atm` program is described in [atm.md](atm.md).

## Threat model

## Deliverables

## Grading
