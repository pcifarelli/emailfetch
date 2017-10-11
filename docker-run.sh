#docker run --log-driver=syslog --log-opt syslog-facility=mail --user 5000:5000 --volume=/srv/vmail:/srv/vmail --detach pcifarelli/emailfetch
docker run --log-driver=awslogs --log-opt awslogs-region=us-east-1 --log-opt awslogs-group=emailfetch --user 5000:5000 --volume=/srv/vmail:/srv/vmail --detach pcifarelli/emailfetch
