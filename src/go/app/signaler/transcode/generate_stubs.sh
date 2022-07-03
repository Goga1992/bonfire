# videoscaler
protoc --go_out=. --go_opt=paths=source_relative \
       --go-grpc_out=. --go-grpc_opt=paths=source_relative \
       --proto_path ../../../../cpp/app/videoscaler/proto/ \
       videoscaler_service.proto

# audiomixer
protoc --go_out=. --go_opt=paths=source_relative \
       --go-grpc_out=. --go-grpc_opt=paths=source_relative \
       --proto_path ../../../../cpp/app/audiomixer/proto/ \
       audiomixer_service.proto