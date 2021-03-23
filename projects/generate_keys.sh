#!/bin/bash
if test -f "private.key"; then
    rm private.key
fi
if test -f "public_key.c"; then
    rm public_key.c
fi

nrfutil keys generate private.key
nrfutil keys display --key pk --format code private.key --out_file public_key.c