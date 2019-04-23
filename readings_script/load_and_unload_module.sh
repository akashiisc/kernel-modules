config_file_name="config_module.json"

pid=$1

if test -z "$pid" 
then 
    echo "PID Missing"
    exit 1
fi

module_path=`cat $config_file_name |  jq -r '.module.path'`
module_file_name=`cat $config_file_name |  jq -r '.module.file_name'`
module_name=`cat $config_file_name |  jq -r '.module.module_name'`

echo > /var/log/syslog
cd $module_path
insmod $module_file_name pid=$pid printstring="TechnicalWays"
rmmod $module_name
cd -
cat /var/log/syslog | grep "READINGS:" > files/stats_file
