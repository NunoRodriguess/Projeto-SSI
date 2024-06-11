#!/bin/bash

# Function to check if directory exists
check_directory() {
    if [ -d "$1" ]; then
        echo "Directory $1 exists"
    else
        echo "Error: Directory $1 does not exist"
        exit 1
    fi
}

# Function to check if file exists
check_file() {
    if [ -f "$1" ]; then
        echo "File $1 exists"
    else
        echo "Error: File $1 does not exist"
        exit 1
    fi
}


# Get the username of the user who executed the script
CURRENT_USER=$(whoami)

# Generate usernames based on the current user
USER1=${CURRENT_USER}manager
USER2=${CURRENT_USER}sender
USER3=${CURRENT_USER}receiver

uid1=$(id -u "$USER1")
uid2=$(id -u "$USER2")
uid3=$(id -u "$USER3")

# Get the main group of the current user
MAIN_GROUP=$(id -gn $CURRENT_USER)

# Start Manager
echo "Login to $USER1 ..."
su -c "rm /home/$USER1/${USER1}_fifo && rm -rf /home/$USER1/mail" $USER1

su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user stop manager' $USER1
echo "Manager closed"


# Start Sender
echo "Login to $USER2 ..."
su -c "rm /tmp/${USER2}_fifo" $USER2

su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user stop sender' $USER2
echo "Sender closed"

# Start Receiver
echo "Login to $USER3 ..."
su -c "rm /tmp/${USER3}_fifo" $USER3

su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user stop receiver' $USER3
echo "Receiver closed"
