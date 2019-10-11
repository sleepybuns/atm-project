# ATM Design and Implementation

This is a two-part project, the first of which is a *team* effort,
while the second is an *individual* effort. Because the second part of
the project will make use of the results of the first part, there will
not be extensions granted. The second part is due at the end of the
semester, so extensions cannot be granted.

In this project you will first design and implement a prototype
ATM/Bank system. Then you will get a chance to (try to) attack other
teams' designs! We have tried to make these instructions as explicit
as possible. Read them carefully; *if anything is unclear, please ask
for clarification well in advance.*

## Overview

 1. You may work in teams of at most four people. Sign up for your
    teams on ELMS under "People">"Groups".

 2. You will design and implement three programs: an *ATM*, a *bank*,
    and an *init* program that initializes state for them. (You may
    also find it useful to create various auxiliary files defining
    classes and functionalities that can be used by the ATM and Bank
    classes.)

 3. You will be provided with stub code for the ATM, the bank, and a
    *router* that will route messages between the ATM and the bank. The
    stub code will allow the ATM and the router to communicate with each
    other, and the router and the bank to communicate with each other.
    The router will be configured to simply pass messages back-and-forth
    between the ATM and the bank. (Looking ahead, the router will provide
    a way to carry out passive or active attacks on the "communication
    channel" between the bank and the ATM.)

 4. You will design a protocol allowing a user to withdraw money from
    the ATM.  Requirements include:
    
     * The ATM *card* of user `XXX` will be represented by a file
       called `XXX.card`.
     * The user's PIN must be a 4-digit number.
     * User balances will be maintained by the bank, not by the ATM.
     * You need **not** support multiple ATMs connecting to the bank
       simultaneously.
     * You also do **not** need to maintain state between restarting
       the bank (e.g., all user balances can be maintained in memory).
    
    Of course, as part of the design process you will want to consider
    security...

 5. You will then implement your protocol. Most of your work should
    involve the ATM, bank, and init programs, with no (or absolutely
    minimal) modifications to the router.

## Part 1 -- Basic Functionality

This section describes the various programs you will need to write for
this project. We have provided stub code for much of it, along with
some helper utility code (a hash table and a list). You are welcome to
use these, or to write your own, but the way that the code
communicates (the ATM and bank route their messages through the
router) must remain the same.

Also, in thie section, we describe outputs for the program: make sure
that these match *exactly* (the outputs do not have periods at the
end, for instance) so that we can use automatic graders.

### Building the programs: what and how

From your submission directory, there should eb a Makefile such that
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

### Invoking the programs

The `init` and `router` programs can be run in either order, but both
must be run before `bank`, which in turn must be run before `atm`. The
order of these will be clear from what they create.

 * The `init` program is described in [init.md](init.md).
 * The `bank` program is described in [bank.md](bank.md).
 * The `atm` program is described in [atm.md](atm.md).

### Threat model

### Part 1 Deliverables

## Part 2 -- Attacking Other Teams' Implementations

### Part 2 Deliverables

## Grading

### Part 1

### Part 2