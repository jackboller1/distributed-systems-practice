#include <grpc++/grpc++.h>
#include<glog/logging.h>
#define log(severity, msg) LOG(severity) << msg; google::FlushLogFiles(google::severity); 

#include "sns.grpc.pb.h"
#include "snsFollowSync.grpc.pb.h"


int main(int argc, char** argv) {

  
  
  std::string port = "3010";
  
//   int opt = 0;
//   while ((opt = getopt(argc, argv, "p:")) != -1){
//     switch(opt) {
//       case 'p':
//           port = optarg;break;
//       default:
// 	  std::cerr << "Invalid Command Line Argument\n";
//     }
//   }
  
  std::string log_file_name = std::string("sync-") + port;
    google::InitGoogleLogging(log_file_name.c_str());
    log(INFO, "Logging Initialized. Server starting...");
  //RunServer(port);

  return 0;
}