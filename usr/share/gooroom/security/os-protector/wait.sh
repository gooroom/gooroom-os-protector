#!/bin/bash
# Network Manager가 수행 완료될 때까지 최대 10초 대기
for ((i=0;i<10;i++));
do
	/bin/journalctl -b | grep -n "NetworkManager" | grep "manager: startup complete" > /dev/null
	if [[ $? -eq 0 ]];
	then
		exit 0
	fi
done

exit -1
