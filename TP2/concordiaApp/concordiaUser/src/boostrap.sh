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

# Function to create user with correct permissions
create_user() {
    local username=$1
    local groupname=$2
    sudo useradd -m -s /bin/bash -g $groupname $username
    sudo usermod -a -G systemd-journal $username
    echo "Specify passwd for $username"
    sudo passwd $username
    echo "User $username created"
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

# Create Manager
if ! id -u $USER1 &>/dev/null; then
    create_user $USER1 $MAIN_GROUP
fi

# Create Sender
if ! id -u $USER2 &>/dev/null; then
    create_user $USER2 $MAIN_GROUP
fi

# Create Receiver
if ! id -u $USER3 &>/dev/null; then
    create_user $USER3 $MAIN_GROUP
fi

sudo usermod -aG "concordia" $CURRENT_USER
sudo usermod -aG "concordia" $USER1
sudo usermod -aG "concordia" $USER2
sudo usermod -aG "concordia" $USER3

# Start Manager
echo "Login to $USER1 ..."
sudo -u $USER1 mkdir -p /home/$USER1/.config/systemd/user/
check_directory "/home/$USER1/.config/systemd/user/"
sudo -u $USER1 cp /home/nuno/Desktop/concordiaApp/concordiaUser/src/manager.service /home/$USER1/.config/systemd/user/


sudo systemctl daemon-reload
check_file "/home/$USER1/.config/systemd/user/manager.service"
su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user start manager' $USER1
echo "Manager started"


# Start Sender
echo "Login to $USER2 ..."
sudo -u $USER2 mkdir -p /home/$USER2/.config/systemd/user/
check_directory "/home/$USER2/.config/systemd/user/"
sudo -u $USER2 cp /home/nuno/Desktop/concordiaApp/concordiaUser/src/sender.service /home/$USER2/.config/systemd/user/


sudo systemctl daemon-reload
check_file "/home/$USER2/.config/systemd/user/sender.service"
su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user start sender' $USER2
echo "Sender started"

# Start Receiver
echo "Login to $USER3 ..."
sudo -u $USER3 mkdir -p /home/$USER3/.config/systemd/user/
check_directory "/home/$USER3/.config/systemd/user/"
sudo -u $USER3 cp /home/nuno/Desktop/concordiaApp/concordiaUser/src/receiver.service /home/$USER3/.config/systemd/user/


sudo systemctl daemon-reload
check_file "/home/$USER3/.config/systemd/user/receiver.service"
su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user start receiver.service' $USER3
echo "Receiver started"
