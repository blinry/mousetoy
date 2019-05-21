# Create a second master input device, and attach an existing USB mouse to it.

MASTER_ID=$(xinput|grep "second"|grep -o "id=[0-9]\+"|grep -o "[0-9]\+"|tr "\n" " ")

if [ -z "$MASTER_ID" ]; then
    xinput create-master second
    MASTER_ID=$(xinput|grep "second"|grep -o "id=[0-9]\+"|grep -o "[0-9]\+"|tr "\n" " ")
    echo fu
fi

NEW_ID=$(xinput|grep "USB Optical Mouse"|grep -o "id=[0-9]\+"|grep -o "[0-9]\+"|tr "\n" " ")

xinput reattach $NEW_ID $MASTER_ID

xinput
