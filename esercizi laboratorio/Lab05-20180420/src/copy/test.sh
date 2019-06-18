#!/bin/bash
FILES="test-100K.raw test-127.raw test-128.raw test-129.raw"
PROG="copy"

if [ -f $PROG ];
then
    for FILE in ${FILES[@]}
    do
        echo "Running test on $FILE..."
        ./$PROG input/$FILE $FILE.copy
        cmp input/$FILE $FILE.copy
        rm $FILE.copy
    done
    echo "Done!"
else
    echo "Did you forget to compile copy.c as \"$PROG\"? :-)"
fi
