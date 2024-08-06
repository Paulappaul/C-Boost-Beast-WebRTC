This is a WebRTC/Boost_Server implementation that allows a single user to connect to another user on a local network. Example compilation: 


g++ -o boost_server boost_server.cpp -I/root/webrtc/projects/final/json/single_include -lboost_system -lboost_thread -lboost_filesystem -lssl -lcrypto -lpthread
