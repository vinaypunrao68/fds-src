VALUE=$(cat /proc/cpuinfo  | grep processor.*: | wc -l)
echo "PUTVAL \"${COLLECTD_HOSTNAME}/cpu-core-count/gauge\" interval=${COLLECTD_INTERVAL} N:$VALUE"