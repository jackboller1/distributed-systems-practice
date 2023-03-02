#include <iostream>
#include <string>
#include <unistd.h>
#include <grpc++/grpc++.h>
#include "client.h"
#include "sns.grpc.pb.h"

using grpc::Status;
using csce438::Message;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;

using std::string;

class Client : public IClient
{
    public:
        Client(const std::string& hname,
               const std::string& uname,
               const std::string& p)
            :hostname(hname), username(uname), port(p)
            {}
    protected:
        virtual int connectTo();
        virtual IReply processCommand(std::string& input);
        virtual void processTimeline();
    private:
        std::string hostname;
        std::string username;
        std::string port;
        
        // You can have an instance of the client stub
        // as a member variable.
        //std::unique_ptr<NameOfYourStubClass::Stub> stub_;
        std::unique_ptr<csce438::SNSService::Stub> stub;
};

int main(int argc, char** argv) {

    std::string hostname = "localhost";
    std::string username = "default";
    std::string port = "3010";
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:")) != -1){
        switch(opt) {
            case 'h':
                hostname = optarg;break;
            case 'u':
                username = optarg;break;
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    Client myc(hostname, username, port);
    // You MUST invoke "run_client" function to start business logic
    myc.run_client();

    return 0;
}

int Client::connectTo()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to create a stub so that
    // you call service methods in the processCommand/porcessTimeline
    // functions. That is, the stub should be accessible when you want
    // to call any service methods in those functions.
    // I recommend you to have the stub as
    // a member variable in your own Client class.
    // Please refer to gRpc tutorial how to create a stub.
	// ------------------------------------------------------------

    //Instantiate the client
    // std::cout << hostname << std::endl;
    // std::cout << port << std::endl;
    // std::cout << username << std::endl;
    auto channel = grpc::CreateChannel(hostname + ":" + port, grpc::InsecureChannelCredentials());
    stub = csce438::SNSService::NewStub(channel);

    //Data we are sending to the server
    csce438::Request login_request;
    login_request.set_username(username);

    //Container for the data we expect from the server
    csce438::Reply login_reply;
    grpc::ClientContext context;

    //RPC call
    grpc::Status status = stub->Login(&context, login_request, &login_reply);

    //Act upon the status
    if (status.ok()) {
        return 1; //return 1 if success, otherwise return -1
    }
    else {
        return -1;
    }

}

IReply Client::processCommand(std::string& input)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse the given input
    // command and create your own message so that you call an 
    // appropriate service method. The input command will be one
    // of the followings:
	//
	// FOLLOW <username>
	// UNFOLLOW <username>
	// LIST
    // TIMELINE
	//
	// ------------------------------------------------------------
	
    // ------------------------------------------------------------
	// GUIDE 2:
	// Then, you should create a variable of IReply structure
	// provided by the client.h and initialize it according to
	// the result. Finally you can finish this function by returning
    // the IReply.
	// ------------------------------------------------------------
    
	// ------------------------------------------------------------
    // HINT: How to set the IReply?
    // Suppose you have "Follow" service method for FOLLOW command,
    // IReply can be set as follow:
    // 
    //     // some codes for creating/initializing parameters for
    //     // service method
    //     IReply ire;
    //     grpc::Status status = stub_->Follow(&context, /* some parameters */);
    //     ire.grpc_status = status;
    //     if (status.ok()) {
    //         ire.comm_status = SUCCESS;
    //     } else {
    //         ire.comm_status = FAILURE_NOT_EXISTS;
    //     }
    //      
    //      return ire;
    // 
    // IMPORTANT: 
    // For the command "LIST", you should set both "all_users" and 
    // "following_users" member variable of IReply.
    // ------------------------------------------------------------
    
    string command = input;
    string username_arg;

    if (input != "LIST" || input != "TIMELINE") {
        int space_idx = input.find(" ");
        command = input.substr(0, space_idx);
        username_arg = input.substr(space_idx + 1, input.length());
    }

    //Data we are sending to the server
    csce438::Request request;
    request.set_username(username);

    //Container for the data we expect from the server
    csce438::Reply reply;
    grpc::ClientContext context;
    IReply ire;

    if (command == "FOLLOW") {
        request.add_arguments(username_arg);
        grpc::Status status = stub->Follow(&context, request, &reply);
        ire.grpc_status = status;
        if (status.ok()) {
            ire.comm_status = SUCCESS;
        }
        else if (reply.msg() == "already following user") {
            ire.comm_status = FAILURE_ALREADY_EXISTS;
        }
        else if (reply.msg() == "username does not exist") {
            ire.comm_status = FAILURE_NOT_EXISTS;
        }
        return ire;
    }

    else if (command == "UNFOLLOW") {

    }

    else if (command == "LIST") {

    }

    else if (command == "TIMELINE") {

    }

    return ire;
}

void Client::processTimeline()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions
    // for both getting and displaying messages in timeline mode.
    // You should use them as you did in hw1.
	// ------------------------------------------------------------

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
	// ------------------------------------------------------------
}
