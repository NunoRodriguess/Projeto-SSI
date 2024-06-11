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

# Create Manager
if ! id -u $USER1 &>/dev/null; then
    create_user $USER1 $MAIN_GROUP
fi

# Create Receiver
if ! id -u $USER3 &>/dev/null; then
    create_user $USER3 $MAIN_GROUP
fi

# Create Sender
if ! id -u $USER2 &>/dev/null; then
    create_user $USER2 $MAIN_GROUP
fi

sudo usermod -aG "concordia" $CURRENT_USER

# Start Manager
echo "Login to $USER1 ..."
sudo -u $USER1 mkdir -p /home/$USER1/.config/systemd/user/
check_directory "/home/$USER1/.config/systemd/user/"
sudo -u $USER1 cp /home/nuno/Desktop/concordiaApp/concordiaUser/src/manager.service /home/$USER1/.config/systemd/user/
sudo -u "$USER1" rm "/home/$USER1/${USER1}_fifo"
sudo -u "$USER1" rm -rf "/home/$USER1/mail"

sudo systemctl daemon-reload
check_file "/home/$USER1/.config/systemd/user/manager.service"
su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user stop manager' $USER1
echo "Manager closed"


# Start Sender
echo "Login to $USER2 ..."
sudo -u $USER2 mkdir -p /home/$USER2/.config/systemd/user/
check_directory "/home/$USER2/.config/systemd/user/"
sudo -u $USER2 cp /home/nuno/Desktop/concordiaApp/concordiaUser/src/sender.service /home/$USER2/.config/systemd/user/
sudo -u "$USER2" rm "/tmp/${USER2}_fifo"

sudo systemctl daemon-reload
check_file "/home/$USER2/.config/systemd/user/sender.service"
su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user stop sender' $USER2
echo "Sender closed"

# Start Receiver
echo "Login to $USER3 ..."
sudo -u $USER3 mkdir -p /home/$USER3/.config/systemd/user/
check_directory "/home/$USER3/.config/systemd/user/"
sudo -u $USER3 cp /home/nuno/Desktop/concordiaApp/concordiaUser/src/receiver.service /home/$USER3/.config/systemd/user/
sudo -u "$USER3" rm "/tmp/${USER3}_fifo"

sudo systemctl daemon-reload
check_file "/home/$USER3/.config/systemd/user/receiver.service"
su -c 'export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER && systemctl --user daemon-reload && systemctl --user stop receiver' $USER3
echo "Receiver closed"
