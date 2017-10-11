docker run --log-driver=syslog --log-opt syslog-facility=mail --user 5000:5000 --volume=/srv/vmail:/srv/vmail --detach pcifarelli/emailfetch
