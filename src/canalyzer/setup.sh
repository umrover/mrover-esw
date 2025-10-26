#!/bin/bash
directoryExists()
cleanup()
docker-compose up --build 2>&1 | tee -a docker_compose.log
while ($run == 1) {
    check_docker()
    trap cleanup SIGINT
}




directoryExists() {
    if [ -d "/grafana_data" ]; then
        echo "grafana_data exists"
        
        user=$(stat -c "%U" "/grafana_data")
        if [ "$user" != "472" ]; then
            echo "Adjusting permissions for grafana_data..."
            chown 472:472 "/grafana_data"
        else
            echo "Ownership already correct (472)"
        fi
    else 
        echo "grafana_data does not exist, creating and setting permissions..."
        mkdir "/grafana_data"
        chown 472:472 "/grafana_data"
    fi

    if [ -d "/influxdb_data" ]; then
        echo "influxdb_data exists"
        
        user=$(stat -c "%U" "/influxdb_data")
        if [ "$user" != "1500   " ]; then
            echo "Adjusting permissions for influxdb_data..."
            chown 1500:1500 "/influxdb_data"
        else
            echo "Ownership already correct (1500)"
        fi
    else 
        echo "influxdb_data does not exist, creating and setting permissions..."
        mkdir "/influxdb_data"
        chown 1500:1500 "/influxdb_data"
    fi
}

deadContainers() {
    
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

#influx is uid 1500


#on kill the shell should gracefully exit by closing all docker containers and saving data

cleanup() {
    echo "Gracefully exiting: "
    docker-compose down
    exit 0
}


#vibecoded snippet to log docker, need to change maybe

check_docker() {
    while true; do
    echo "---- $(date) ----" >> docker_status.log
    docker ps -a >> docker_status.log
    sleep 60  # check every minute
    trap cleanup SIGINT
    done
}