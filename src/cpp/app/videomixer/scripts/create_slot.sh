for i in {0..1} ; do
  time grpcurl -plaintext -d '{"sink_hostname": "127.0.0.1", "sink_port": '$((50000 + $i))', "room_id": "kek" }' localhost:8080 VideoMixer/StartVideoSlot ;
  # gst-launch-1.0  --quiet audiotestsrc do-timestamp=true is-live=true num-buffers=1 ! audioconvert ! audioresample ! 'audio/x-raw,format=S16LE,rate=48000,channels=1' ! audioconvert ! opusenc bitrate=48000 ! rtpopuspay ! udpsink host=0.0.0.0 port=$((40000 + $i)) &
  # gst-launch-1.0  --quiet filesrc location=/workdir/noise.wav num-buffers=2 ! wavparse ! audioconvert ! audioresample ! 'audio/x-raw,format=S16LE,rate=48000' ! audioconvert ! opusenc bitrate=48000 ! rtpopuspay ! udpsink host=0.0.0.0 port=$((40000 + $i)) &
done

time grpcurl -plaintext -d '{"sink_hostname": "127.0.0.1", "sink_port": '$((50004))', "room_id": "kek"  }' localhost:8080 VideoMixer/StartVideoSlot ;

wait
