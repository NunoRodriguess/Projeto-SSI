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

# Function to set environment variables for user
set_environment_variables() {
     export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus"
     loginctl enable-linger $USER
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
echo "Configuring to $USER1 ..."
su -c "cp /home/nuno/Desktop/concordiaApp/concordiaUser/src/manager.service ~/.config/systemd/user/" $USER1

echo "Starting $USER1 ..."
su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user start manager' $USER1
echo "Manager started"

# Start Sender
echo "Configuring to $USER2 ..."
su -c "cp /home/nuno/Desktop/concordiaApp/concordiaUser/src/sender.service ~/.config/systemd/user/" $USER2

echo "Starting $USER2 ..."
su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user start sender' $USER2
echo "Sender started"

# Start Receiver
echo "Configuring to $USER3 ..."
su -c "cp /home/nuno/Desktop/concordiaApp/concordiaUser/src/receiver.service ~/.config/systemd/user/" $USER3

echo "Starting $USER3 ..."
su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user start receiver' $USER3
echo "Receiver started"