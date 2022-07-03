for i in $(eval echo {0..$1}); do
  time grpcurl -plaintext -d \
  '{"sink_hostname": "127.0.0.1", "sink_ports": ['$((40000 + $i*3))', '$((40000 + $i*3 + 1))', '$((40000 + $i*3 + 2))'] }' \
  localhost:7000 VideoScaler/StartVideoSlot ;
done

wait
