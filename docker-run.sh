docker run --log-driver=syslog --log-opt syslog-facility=mail --volume=/efs/root/vmail:/vmail --detach pcifarelli/emailfetch:latest
#docker run --log-driver=awslogs --log-opt awslogs-region=us-east-1 --log-opt awslogs-group=emailfetch --volume=/efs/root/vmail:/vmail --detach pcifarelli/emailfetch
