#!/bin/bash

docker-compose up --build

directoryExists($1, $2) {
    if [ -d "/grafana_data" ]; then
        echo "grafana_data exists"
    else 
        echo "grafana_data does not exist, creating and setting permissions"
        mkdir "/grafana_data"
        chown 472:472 grafana_data
    fi
}

influxdbExists() {

}


#check for dead containers
#       sudo lsof -i :8086
#       sudo kill <pid>
#


#setup directories for grafana and influxdb with permissions chown
#   setup grafana
#       /var/lib/grafana
#       /var/lib/grafana/plugins
#   setup influxdb
#       /var/lib/influxdb/meta
#


#on kill the shell should gracefully exit by closing all docker containers and saving data


#vibecoded snippet to log docker, need to change maybe

while true; do
  echo "---- $(date) ----" >> docker_status.log
  docker ps -a >> docker_status.log
  sleep 60  # check every minute
done
