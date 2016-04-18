#!/bin/bash

dirname=$(dirname $0)

. ${dirname}/setup.defaults


function write_config() {
   host=$1
   configfile=$2
   cmd=""
   for line in $(cat ${confdir}/config)
   do
      option=$(echo ${line} | cut -f 1 -d "=")
      value=$(echo ${line} | cut -f 2 -d "=")
      cmd="${cmd}perl -pi -e 's!(${option} *=) *.*!\1 ${value}!' ${configfile};"
   done
   eval ssh $sshParameter ${host} \"${cmd}\"

   return $?
}

if [ "$1" = "" ];then exit 1;fi
if [ "$2" = "" ];then exit 1;fi

write_config $1 $2
if [ $? -ne 0 ];then exit 1;fi
