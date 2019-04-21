cd /home/akash/data/kernel-modules/readings_script

ssh_hostname=`cat config.json |  jq -r '.ssh.host'`
ssh_user=`cat config.json |  jq -r '.ssh.user'`
ssh_password=`cat config.json |  jq -r '.ssh.password'`
ssh_port=`cat config.json |  jq -r '.ssh.port'`
remote_path=`cat config.json |  jq -r '.remote_path'`
home_script=`cat config.json |  jq -r '.home_script'`

pid=$1

if test -z "$pid"
then
    echo "PID Missing"
    exit 1
fi


echo "Running : $home_script $pid"
bash $home_script $pid

echo "Running: ./scp_script $ssh_hostname $ssh_user $remote_path $ssh_password"
./scp_script $ssh_hostname $ssh_user "$remote_path" $ssh_password             
