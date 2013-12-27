all: proto sh sm

proto:
	protoc --cpp_out=gen putget.proto

sh: proto
	g++ -std=c++11 -g sh.cc gen/putget.pb.cc -lprotobuf -lzmq -lczmq -o sh

sm: proto
	g++ -std=c++11 -g sm.cc gen/putget.pb.cc -lprotobuf -lzmq -lczmq -o sm
clean:
	$(RM) *.o sh sm gen/*
