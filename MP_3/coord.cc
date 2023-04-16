#include <grpc++/grpc++.h>
#include<glog/logging.h>
#define log(severity, msg) LOG(severity) << msg; google::FlushLogFiles(google::severity); 

#include "sns.grpc.pb.h"
#include "snsCoordinator.grpc.pb.h"
#include <string>
#include <unordered_map>


using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using snsCoordinator::SNSCoordinator;
using snsCoordinator::User;
using snsCoordinator::ClusterId;
using snsCoordinator::MASTER;
using snsCoordinator::SLAVE;
using snsCoordinator::NONE;

using std::string;
using std::unordered_map;

#define INACTIVE 0
#define ACTIVE 1


struct RoutingTableRow {
  string ip;
  string port_num;
  int status;
};

unordered_map<int, RoutingTableRow> master_table;
unordered_map<int, RoutingTableRow> slave_table;
unordered_map<int, RoutingTableRow> sync_table;



class SNSServiceImplCoord final : public SNSCoordinator::Service {

  Status GetServer (ServerContext* context, User* user, snsCoordinator::Server* server_reply) {
    int cluster_id = (user->user_id() % 3) + 1;

    //If master exists and is active, return master info
    if (master_table.count(cluster_id) > 0 && master_table[cluster_id] == ACTIVE) {
      server_reply->set_server_ip(master_table[cluster_id].ip);
      server_reply->set_port_num(master_table[cluster_id].port_num);
      server_reply->set_server_id(cluster_id);
      server_reply->set_server_type(MASTER);
    }
    //Otherwise, return slave info
    else if (slave_table.count(cluster_id) > 0 && slave_table[cluster_id] == ACTIVE) {
      server_reply->set_server_ip(slave_table[cluster_id].ip);
      server_reply->set_port_num(slave_table[cluster_id].port_num);
      server_reply->set_server_id(cluster_id);
      server_reply->set_server_type(SLAVE);
    }

    return Status::OK;
  }

  Status GetSlave (ServerContext* context, ClusterId* c_id ,snsCoordinator::Server* server_reply) {
    int cluster_id = (c_id->cluster());

    if (slave_table.count(cluster_id) > 0 && slave_table[cluster_id] == ACTIVE) {
      server_reply->set_server_ip(slave_table[cluster_id].ip);
      server_reply->set_port_num(slave_table[cluster_id].port_num);
      server_reply->set_server_id(cluster_id);
      server_reply->set_server_type(SLAVE);
    }
    else {
      server_reply->set_server_type(NONE);
    }

    return Status::OK;
  }

};


void run_coordinator(std::string port_no) {
  std::string coord_address = "0.0.0.0:"+port_no;
  SNSServiceImplCoord service;

  ServerBuilder builder;
  builder.AddListeningPort(coord_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> coordinator(builder.BuildAndStart());
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