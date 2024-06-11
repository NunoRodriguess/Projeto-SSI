#!/bin/bash


cp /home/nuno/Desktop/concordiaApp/concordiaDaemon/src/concordia.service ~/.config/systemd/user/
export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$UID/bus" && loginctl enable-linger $USER
systemctl --user daemon-reload
systemctl --user start concordia.service