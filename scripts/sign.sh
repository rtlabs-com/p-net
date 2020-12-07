#!/bin/bash

NAME=$1
PRODUCT=$2
DESCRIPTION=$3

usage()
{
    echo "Usage: $0 <company> <product> <product_description>"
    exit
}

replace()
{
    # Find matching filenames containing license
    files=$(git ls-files $@ | xargs grep -l -e "$MATCH1" -e "$MATCH2")

    # Replace header in files
    for f in $files; do
        echo "$HEADER" > $f.tmp
        sed '1,/\*\//d' $f >> $f.tmp
        mv $f.tmp $f
    done
}

[ "$#" -ne 3 ] && usage

MATCH1="This software is dual-licensed"
MATCH2="BEGIN PGP"

# Create and sign license
LICENSE=$(cat LICENSE.md.in | sed "s/@COMPANY@/$NAME/;s/@PRODUCT@/$PRODUCT/;s/@PRODUCT_DESCRIPTION@/$DESCRIPTION/" | gpg --output - --clearsign)

# Replace C headers
HEADER=$'/*\n'"$LICENSE"$'\n*/'
replace \*.[ch] \*.cpp \*.h.in

# Replace cmake headers
HEADER=$'#[[\n'"$LICENSE"$'\n]]#*/'
replace \*CMakeLists.txt \*.cmake

# Update LICENSE file
echo "$LICENSE" > LICENSE.md
