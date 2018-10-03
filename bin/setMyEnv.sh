#!/bin/bash

# must be sourced like this:
# ssh mysql@server ". /home/mysql/product/myenv/bin/setMyEnv.sh mariadb-103 ; type mysql"

source /etc/myenv/MYENV_BASE
source $MYENV_BASE/bin/myenv.profile
setMyEnv $1
