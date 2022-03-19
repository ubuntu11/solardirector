[Unit]
Description=Sunny Island Agent
After=network-online.target network.target rsyslog.service

[Service]
Type=simple
ExecStart=/opt/sd/bin/si -c /opt/sd/etc/si.conf -l /opt/sd/log/si.log -a
Restart=on-failure

[Install]
WantedBy=multi-user.target
