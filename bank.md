# The `bank` program

The `bank` program takes one command line argument, as follows:

    % <path1>/bank <path2>/<init-fname>.bank

That is, it is called with a `<init-fname>.bank` that was created by
the `init` program. You can assume that it will NOT be called with any
other file than one that was created with your `init` program.

## Behavior of the `bank` program

 * If `<init-fname>.bank` cannot be opened, print
   "`Error opening bank initialization file`" and return value 64.

 * Otherwise, present the bank prompt "`BANK: `" and process bank
   commands, as described below.

### Bank commands
The bank should support the following commands:

 * `create-user <user-name> <pin> <balance>`
    * Inputs
       * `<user-name>`: a name that can consist only of upper and lower
         case characters (`[a-zA-Z]+`). (Valid user names are at most 250
	 characters.)
       * `<pin>`: a four-digit number `[0-9][0-9][0-9][0-9]`
       * `<balance>`: a non-negative integer `[0-9]+` at most what can be
         be represented with an `int`.
    * Behavior
       * ***TBD***
 * `deposit <user-name> <amt>`
    * Inputs
       * `<user-name>`: a name that can consist only of upper and lower
         case characters (`[a-zA-Z]+`). (Valid user names are at most 250
	 characters.)
       * `<amt>`: a non-negative integer `[0-9]+` at most what can be
         be represented with an `int`.
    * Behavior
       * ***TBD***
 * `balance <user-name>`
    * Inputs
       * `<user-name>`: a name that can consist only of upper and lower
         case characters (`[a-zA-Z]+`). (Valid user names are at most 250
	 characters.)
    * Behavior
       * ***TBD***
 * Any other commands are invalid (note that withdrawals are not supported
   at the bank), and should result in printing "`Invalid command`"

## Example transcript

