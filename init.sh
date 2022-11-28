#! /bin/bash

if [ "$#" -ne 1 ] 
then
    echo "Usage:  init <filename>"
    exit 62
fi

if [ -f "$1.bank" ] || [ -f "$1.atm" ]
then
    echo "Error: one of the files already exists"
    exit 63
fi

openssl rand 32 > "$1.bank"
if [ $? -ne 0 ]
then
    echo "Error creating initialization files"
    exit 64
fi

cp "$1.bank" "$1.atm"
if [ $? -ne 0 ]
then
    echo "Error creating initialization files"
    exit 64
fi

echo "Successfully initialized bank state"
exit 0