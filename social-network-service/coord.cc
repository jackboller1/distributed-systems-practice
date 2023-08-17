#include <grpc++/grpc++.h>
#include<glog/logging.h>
#define log(severity, msg) LOG(severity) << msg; google::FlushLogFiles(google::severity); 

#include "sns.grpc.pb.h"
#include "snsCoordinator.grpc.pb.h"
#include <string>
#include <unordered_map>
#include <ctime>
#include <iostream>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>


using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;
using google::protobuf::Timestamp; 
using snsCoordinator::SNSCoordinator;
using snsCoordinator::User;
using snsCoordinator::ClusterId;
using snsCoordinator::Heartbeat;
using snsCoordinator::ServerType;
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
  Timestamp last_hb;
};

void print_table(unordered_map<int, RoutingTableRow> table) {
  for (auto& pair : table) {
    std::cout  <<"Cluster ID: " << pair.first << " IP: " << pair.second.ip << ", Port: " << pair.second.port_num << ", ACTIVE: " 
    << pair.second.status << ", Last heartbeat: " << pair.second.last_hb << std::endl;
  }
}

unordered_map<int, RoutingTableRow> master_table;
unordered_map<int, RoutingTableRow> slave_table;
unordered_map<int, RoutingTableRow> sync_table;


class SNSServiceImplCoord final : public SNSCoordinator::Service {

  Status GetServer (ServerContext* context, const User* user, snsCoordinator::Server* server_reply) override {
    int cluster_id = (user->user_id() % 3) + 1;

    //std::cout <<  "Master table count: " << master_table.count(cluster_id) << std::endl;
    //If master exists and is active, return master info
    if (master_table.count(cluster_id) > 0 && master_table[cluster_id].status == ACTIVE) {
      //std::cout << "Searching master" << std::endl;
      server_reply->set_server_ip(master_table[cluster_id].ip);
      server_reply->set_port_num(master_table[cluster_id].port_num);
      server_reply->set_server_id(cluster_id);
      server_reply->set_server_type(MASTER);
    }
    //Otherwise, return slave info
    else if (slave_table.count(cluster_id) > 0 && slave_table[cluster_id].status == ACTIVE) {
      server_reply->set_server_ip(slave_table[cluster_id].ip);
      server_reply->set_port_num(slave_table[cluster_id].port_num);
      server_reply->set_server_id(cluster_id);
      server_reply->set_server_type(SLAVE);
    }
    log(INFO, "Getting server on cluster id " + std::to_string(cluster_id));
    return Status::OK;
  }

  Status GetSlave (ServerContext* context, const ClusterId* c_id, snsCoordinator::Server* server_reply) override {
    int cluster_id = (c_id->cluster());

    if (slave_table.count(cluster_id) > 0 && slave_table[cluster_id].status == ACTIVE) {
      server_reply->set_server_ip(slave_table[cluster_id].ip);
      server_reply->set_port_num(slave_table[cluster_id].port_num);
      server_reply->set_server_id(cluster_id);
      server_reply->set_server_type(SLAVE);
    }
    else {
      server_reply->set_server_type(NONE);
    }
    log(INFO, "Getting slave on cluster id " + std::to_string(cluster_id));
    return Status::OK;
  }

  Status HandleHeartBeats (ServerContext* context, ServerReaderWriter<Heartbeat, Heartbeat>* stream) override {
    Heartbeat hb;
    int cluster_id;
    ServerType type;
    Timestamp hb_timestamp;

    while (stream->Read(&hb)) {
      cluster_id = hb.server_id();
      type = hb.server_type();
      hb_timestamp = hb.timestamp();

      if (type == MASTER) {
        master_table[cluster_id] = {hb.server_ip(), hb.server_port(), ACTIVE, hb_timestamp};
        log(INFO, "Heartbeat received from master with cluster id " + std::to_string(cluster_id));
        //std::cout << "Updating master_table" << std::endl;
      }
      else if (type == SLAVE) {
        slave_table[cluster_id] = {hb.server_ip(), hb.server_port(), ACTIVE, hb_timestamp};
        log(INFO, "Heartbeat received from slave with cluster id " + std::to_string(cluster_id));
      }
      
      if (master_table.count(cluster_id) > 0) {
        Timestamp last_master =  master_table[cluster_id].last_hb;
        if (difftime(google::protobuf::util::TimeUtil::TimestampToTimeT(hb_timestamp), google::protobuf::util::TimeUtil::TimestampToTimeT(last_master)) > 20) {
          master_table[cluster_id].status = INACTIVE;
          log(INFO, "Setting master with cluster id " + std::to_string(cluster_id) + " to INACTIVE");
        }
      }

      if (slave_table.count(cluster_id) > 0) {
        Timestamp last_slave =  slave_table[cluster_id].last_hb;
        if (difftime(google::protobuf::util::TimeUtil::TimestampToTimeT(hb_timestamp), google::protobuf::util::TimeUtil::TimestampToTimeT(last_slave)) > 20) {
          slave_table[cluster_id].status = INACTIVE;
          log(INFO, "Setting slave with cluster id " + std::to_string(cluster_id) + " to INACTIVE");
        }
      }

      
      //print_table(master_table);

    }

    //Once stream has ended, set to inactive
    if (type == MASTER) {
      master_table[cluster_id].status = INACTIVE;
      log(INFO, "Stream ended on master with cluster id " + std::to_string(cluster_id));
    }
    else if (type == SLAVE) {
      slave_table[cluster_id].status = INACTIVE;
      log(INFO, "Stream ended on slave with cluster id " + std::to_string(cluster_id));
    }

    
    //print_table(master_table);

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
  
  FLAGS_log_dir = "/home/csce438/CSCE438/MP_3/temp";
  std::string log_file_name = std::string("coord-") + port;
  google::InitGoogleLogging(log_file_name.c_str());
  log(INFO, "Logging Initialized. Coordinator starting...");
  run_coordinator(port);

  return 0;
}