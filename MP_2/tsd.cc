#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>

#include "sns.grpc.pb.h"
#include <vector>
#include <sys/stat.h>
#include <map>

using google::protobuf::Timestamp;
using google::protobuf::Duration;
using google::protobuf::util::TimeUtil;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using csce438::Message;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;

using std::string;
using std::vector;
using std::fstream;
using std::ios;
using std::map;
using std::endl;

vector<string> all_user_vect;
vector<string> session_user_vect;
map<string, ServerReaderWriter<Message, Message>*> username_to_srw;
string user_dir = "UserInfo/";


void add_user_to_following(string orig_username, string new_username) {
  //add the new_user to orig_username_following.txt
  fstream fio;
  fio.open(user_dir + orig_username + "_following.txt", ios::out | ios::in | ios::app);
  fio << new_username << std::endl;
  // auto timestamp = new google::protobuf::Timestamp{};
  // timestamp->set_seconds(time(NULL));
  // timestamp->set_nanos(0);
  // fio << *timestamp << std::endl;
  fio.close();
}

void add_user_to_followers(string orig_username, string follower) {
  //add the follower to orig_username_followers.txt
  fstream fio;
  fio.open(user_dir + orig_username + "_followers.txt", ios::out | ios::in | ios::app);
  fio << follower << std::endl;
  fio.close();
}

void create_user_files(string username) {
  //create timeline file
  fstream fio;
  fio.open(user_dir + username + "_timeline.txt", ios::out | ios::in | ios::app);
  fio.close();

  //create followers file
  fio.open(user_dir + username + "_followers.txt", ios::out | ios::in | ios::app);
  fio.close();
  
  //add the user to user_names.txt
  fio.open(user_dir + "user_names.txt", ios::out | ios::app);
  fio << username << std::endl;
  fio.close();

  //add the user to username_following.txt
  add_user_to_following(username, username);
}

void collect_users() {
  //create UserInfo/user_names.txt if it doesn't exists
  fstream fio;
  fio.open(user_dir + "user_names.txt", ios::out | ios::in | ios::app);
  
  string line;
  // Execute a loop until EOF (End of File)
    while (fio) {
        // Read a Line from File
        getline(fio, line);
        if (line.length() > 0) {
          all_user_vect.push_back(line);
        }   
    }
 
  fio.close();
}

bool does_user_exist(vector<string> user_vect, string username) {
  return std::count(user_vect.begin(), user_vect.end(), username) > 0;
}

bool does_user_exist(map<string, ServerReaderWriter<Message, Message>*> username_to_srw, string username) {
  return username_to_srw.find(username) != username_to_srw.end();
}

vector<string> get_users_from_file(string filename) {
  vector<string> users;
  string line;

  fstream fio;
  fio.open(user_dir + filename, ios::out | ios::in | ios::app);
  while (fio) {
    getline(fio, line);
    if (line.length() > 0) {
      users.push_back(line);
    }
  }
  fio.close();

  return users;
}

vector<Message> get_posts_from_timeline(string username) {
  vector<Message> posts;
  Message curr_post;
  string line;
  fstream fio;
  //format of _timeline.txt is:
  //username
  //timestamp
  //message
  fio.open(user_dir + username + "_timeline.txt", ios::out | ios::in | ios::app);

  while (fio) {
    getline(fio, line);
    curr_post.set_username(line);
    std::cout << "Get posts: username" << std::endl;

    getline(fio, line);
    auto ts = new Timestamp{};
    //convert line into timestamp object
    TimeUtil::FromString(line, ts);
    curr_post.set_allocated_timestamp(ts);

    std::cout << "Get posts: timestamp" << std::endl;

    getline(fio, line);
    curr_post.set_msg(line);
    std::cout << "Get posts: message" << std::endl;

    posts.push_back(curr_post);
  }

  fio.close();
  std::cout << "Get posts: return" << std::endl;
  return posts;
}

void add_post_to_timeline(string username, Message post) {
  fstream fio;
  fio.open(user_dir + username + "_timeline.txt", ios::out | ios::in | ios::app);

  fio << post.username() << endl;
  fio << post.timestamp() << endl;
  fio << post.msg() << endl;

  fio.close();
}


class SNSServiceImpl final : public SNSService::Service {
  
  Status List(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // LIST request from the user. Ensure that both the fields
    // all_users & following_users are populated
    // ------------------------------------------------------------
    string username = request->username();

    vector<string> following_users = get_users_from_file(username + "_following.txt");
    reply->mutable_all_users()->Assign(all_user_vect.begin(), all_user_vect.end());
    reply->mutable_following_users()->Assign(following_users.begin(), following_users.end());

    return Status::OK;
  }

  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // request from a user to follow one of the existing
    // users
    // ------------------------------------------------------------
      string username = request->username();
      string user_to_follow = request->arguments(0);
      string msg;
      vector<string> following_users_vect = get_users_from_file(username + "_following.txt");

      //check if user_to_follow exists
      if (!does_user_exist(all_user_vect, user_to_follow)) {
        msg = "username does not exist";
        reply->set_msg(msg);
        return Status::CANCELLED;
      }
      //check if user_to_follow is already in user_following.txt
      else if (does_user_exist(following_users_vect, user_to_follow)) {
        msg = "already following user";
        reply->set_msg(msg);
        return Status::CANCELLED;
      }
      
      add_user_to_followers(user_to_follow, username);
      add_user_to_following(username, user_to_follow);
      return Status::OK;

  }

  Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // request from a user to unfollow one of his/her existing
    // followers
    // ------------------------------------------------------------
    return Status::OK;
  }
  
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // a new user and verify if the username is available
    // or already taken
    // ------------------------------------------------------------
    string username = request->username();
    //std::cout << username << std::endl;

    if (does_user_exist(session_user_vect, username)) {
      return Status::CANCELLED;
    }
    else if (does_user_exist(all_user_vect, username)) {
      session_user_vect.push_back(username);
      return Status::OK;
    }
    else {
      session_user_vect.push_back(username);
      all_user_vect.push_back(username);
      create_user_files(username);
      return Status::OK;
    }
    
  }

  Status Timeline(ServerContext* context, ServerReaderWriter<Message, Message>* stream) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // receiving a message/post from a user, recording it in a file
    // and then making it available on his/her follower's streams
    // ------------------------------------------------------------

    Message client_msg;
    string username;
    vector<string> following_users;
    vector<string> followers;

    while (stream->Read(&client_msg)) { //while the server can still read messages from the client
      username = client_msg.username();
      std::cout << username << std::endl;
      std::cout << client_msg.msg() << std::endl;

      if (does_user_exist(username_to_srw, username)) {
        //Client is already in timeline mode, so they are sending a message to followers
        std::cout << "User does exist" << std::endl;
        followers = get_users_from_file(username + "_followers.txt");
        for (int i = 0; i < followers.size(); i++) {
          //If the follower is currently in timeline mode, send them the message
          if ( does_user_exist(username_to_srw, followers.at(i)) && username != followers.at(i) ) {
            username_to_srw[followers.at(i)]->Write(client_msg);
          }
          //add the post to the followers timeline
          add_post_to_timeline(followers.at(i), client_msg);
        }
      }

      else {
        //Client is entering timeline mode, so get last 20 messages from the its timeline
        std::cout << "User doesn't exists" << std::endl;
        following_users = get_users_from_file(username + "_following.txt");
        //add stream to the map
        username_to_srw[username] = stream;
        std::cout << "I added the stream to the map" << std::endl;
        //get the posts from the users's timeline
        vector<Message> timeline_post_vect = get_posts_from_timeline(username);
        std::cout << "I have gotten the posts from the timeline" << std::endl;
        int num_messages = 0;
        //get the last (up to) 20 messages
        for (int i = timeline_post_vect.size() - 1; i >= 0; i--) {
          if (num_messages == 20) {
            break;
          }
          //if the client is still following the user, send the message to the client
          else if ( does_user_exist(following_users, timeline_post_vect.at(i).username()) ) {
            stream->Write(timeline_post_vect.at(i));
            num_messages += 1;
          }
        }

      }

    }

    //After server can no longer read messages from the client, remove the entry from username_to_srw map
    //username_to_srw.erase(username);

    return Status::OK;
  }

};

void RunServer(std::string port_no) {
  // ------------------------------------------------------------
  // In this function, you are to write code 
  // which would start the server, make it listen on a particular
  // port number.
  // ------------------------------------------------------------
  string server_address("0.0.0.0:" + port_no);
  SNSServiceImpl service;

  ServerBuilder builder;
  //Listen on given address without any authentication mechanism
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  //Register "service" as the instance through which we'll communicate with clients.
  //It corresponds to a synchronous service
  builder.RegisterService(&service);
  //Assemble the server
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  //create directory to store UserInfo
  mkdir("UserInfo", 0777);

  //store all users in the vector
  collect_users();
  //Wait for the server to shutdown. Note that some other thread must be responsible for shutting down
  //the server for this call to ever return
  server->Wait();
}

int main(int argc, char** argv) {
  
  std::string port = "3010";
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;
          break;
      default:
	         std::cerr << "Invalid Command Line Argument\n";
    }
  }
  RunServer(port);
  return 0;
}
