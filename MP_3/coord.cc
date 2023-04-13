#include <grpc++/grpc++.h>
#include<glog/logging.h>
#define log(severity, msg) LOG(severity) << msg; google::FlushLogFiles(google::severity); 

#include "sns.grpc.pb.h"
#include "snsCoordinator.grpc.pb.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::Status;
using snsCoordinator::SNSCoordinator;
#include <string>


class SNSServiceImplCoord final : public SNSCoordinator::Service {

};


void run_coordinator(std::string port_no) {
  std::string coord_address = "0.0.0.0:"+port_no;
  SNSServiceImplCoord service;

  ServerBuilder builder;
  builder.AddListeningPort(coord_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> coordinator(builder.BuildAndStart());
  std::cout << "Coordinator listening on " << coord_address << std::endl;
  log(INFO, "Coordinator listening on "+coord_address);

  coordinator->Wait();
}

int main(int argc, char** argv) {

  std::string port = "3010";
  
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  
  std::string log_file_name = std::string("coord-") + port;
  google::InitGoogleLogging(log_file_name.c_str());
  log(INFO, "Logging Initialized. Coordinator starting...");
  run_coordinator(port);

  return 0;
}