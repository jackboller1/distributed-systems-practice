#include <glog/logging.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include "interface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>

using std::vector;
using std::map;
using std::string;

#define MAX_CHAT_ROOM 25
#define SERVER_PORT 8080
#define MAX_MEMBER 30
#define MAX_CLIENTS 256

struct Chat_Room {
    int port_num;
    string room_name;
    int num_members;
    int slave_socket[MAX_MEMBER];
};


int init_socket(int port);
bool does_room_exist(string room_name, vector<Chat_Room> chat_room_db);
void add_chat_room_fd(int chat_room_socket[], int chat_room_fd);

int main(int argc, char *argv[]){
    google::InitGoogleLogging(argv[0]);

    //check the number of arguments
    if (argc != 1) {
        LOG(ERROR) << "USAGE: Do not enter the server port number";
        exit(1);
    }
    
    LOG(INFO) << " Before Starting Server";
    //initialize master socket
    int server_fd = init_socket(SERVER_PORT);

    //initialize data structures to hold Chat_Rooms and chat_room file descriptors
    vector<Chat_Room> chat_room_db;
    int chat_room_socket[MAX_CHAT_ROOM];
    map<int, Chat_Room> fd_to_chat_room;
    int client_socket[MAX_CLIENTS];
    string client_room_name[MAX_CLIENTS];

    //initialize chat room socket to 0
    for (int i = 0; i < MAX_CHAT_ROOM; i++) {
        chat_room_socket[i] = 0;
    }

    //initialize client sockets to 0
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    //initialize room names to empty string
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_room_name[i] = "";
    }


    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);
    int address_len = sizeof(address);

    //new client connection
    int new_socket;
    char message[256];
    //max file descriptor to be used for select
    int max_fd;
    //set of socket file descriptors to be uesd for select
    fd_set read_fd_set;

    
    while (true) {
        
        //clear socket set
        FD_ZERO(&read_fd_set);

        //add server to socket set
        FD_SET(server_fd, &read_fd_set);
        max_fd = server_fd;

        //add chat room sockets to socket set
        for (int i = 0; i < MAX_CHAT_ROOM; i++) {
            int chat_room_fd = chat_room_socket[i];
            if (chat_room_fd > 0) {
                FD_SET(chat_room_fd, &read_fd_set);
            }
            //update max_fd if needed
            if (chat_room_fd > max_fd) {
                max_fd = chat_room_fd;
            }
        }

        //add client sockets to socket set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_socket[i];
            if (client_fd > 0) {
                FD_SET(client_fd, &read_fd_set);
            }
            //update max_fd if needed
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
        }

        //wait indefinitely for activity on the server or a chat room socket
        if (select(max_fd+1, &read_fd_set, NULL, NULL, NULL) < 0 ) {
            LOG(ERROR) << "Select failure";
        }

        //Activity occured on the server socket, so it is an incoming connection
        if (FD_ISSET(server_fd, &read_fd_set)) {
        
            if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&address_len)) < 0) {
                LOG(ERROR) << "Accept failure";
                exit(1);
            }

            //add new fd to client sockets
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        //Activity occured on a chat room socket, so it is an incoming connection from a client in the chat room
        for(int i = 0; i < MAX_CHAT_ROOM; i++) {
            Chat_Room curr_room;
            struct sockaddr_in cr_address;
            cr_address.sin_family = AF_INET;
            cr_address.sin_addr.s_addr = INADDR_ANY;
            int new_conn_fd;

            if (FD_ISSET(chat_room_socket[i], &read_fd_set)) {
                curr_room = fd_to_chat_room[chat_room_socket[i]];
                cr_address.sin_port = htons(curr_room.port_num);
                int cr_address_len = sizeof(cr_address);
                if ((new_conn_fd = accept(chat_room_socket[i], (struct sockaddr*)&cr_address, (socklen_t*)&cr_address_len)) < 0) {
                    LOG(ERROR) << "Accept failure for chat room socket";
                    exit(1);
                }
                //add new fd to client socket and update room_name for client
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_socket[j] == 0) {
                        client_socket[j] = new_conn_fd;
                        client_room_name[j] = curr_room.room_name;
                        break;
                    }
                }
            }
        }

        //otherwise its an operation on chat room/client socket
        for (int i = 0; i < MAX_CLIENTS; i++) {
            
            if (FD_ISSET(client_socket[i], &read_fd_set)) {
            if  ( read(client_socket[i], message, 256) <= 0) {
                //client socket closed
                close(client_socket[i]);
                client_socket[i] = 0;
                client_room_name[i] = "";
            }
            

            else {
            //parse message
            LOG(INFO) << message;
            string cr_message_str = message;
            string curr_word = strtok(message, " ");
            for (int j = 0; j < curr_word.size(); j++) {
                curr_word.at(j) = toupper(curr_word.at(j));
            }
            string room_name;
            Reply reply;

            if (curr_word == "CREATE") {
                LOG(INFO) << "Client called create";
                //get next word after create
                if (curr_word.length() > 0) {
                    curr_word = strtok(NULL, " ");
                    room_name = curr_word;
                    LOG(INFO) << "Create room " << room_name;
                    if (!does_room_exist(room_name, chat_room_db)) {
                        reply.status = SUCCESS;
                        int new_port = SERVER_PORT + 1 + chat_room_db.size();
                        //initialize num_members and all slave sockets to 0
                        struct Chat_Room new_chat_room = {new_port, room_name, 0, {0}};
                        chat_room_db.push_back(new_chat_room);
                        //create new socket for chat room
                        int chat_room_fd = init_socket(new_port);
                        //map file descriptor to chat_room struct
                        fd_to_chat_room[chat_room_fd] = new_chat_room;
                        //add new chat room to array of file descriptors
                        add_chat_room_fd(chat_room_socket, chat_room_fd);
                    }
                    else {
                        reply.status = FAILURE_ALREADY_EXISTS;
                    }
                    //send Reply back to client
                    if (send(client_socket[i], &reply, sizeof(Reply), 0) < 0){
                        LOG(ERROR) << "Failure to send response after create";
                    }
                }
            }
            else if (curr_word == "JOIN") {
                LOG(INFO) << "Client called join";
                //get next word after join
                if (curr_word.length() > 0) {
                    curr_word = strtok(NULL, " ");
                    room_name = curr_word;
                    LOG(INFO) << "Join room " << room_name;
                    if (does_room_exist(room_name, chat_room_db)) {
                        reply.status = SUCCESS;
                        //get the port of the chat_room
                        Chat_Room chat_room;
                        for (int i = 0; i < chat_room_db.size(); i++) {
                            if (chat_room_db.at(i).room_name == room_name) {
                                chat_room = chat_room_db.at(i);
                                break;
                            }
                        }
                        int cr_port = chat_room.port_num;
                        int num_members = 0;
                        for (int i = 0; i < MAX_CLIENTS; i++) {
                            if (client_room_name[i] == room_name) {
                                num_members += 1;
                            }
                        }
                        
                        reply.port = cr_port;
                        reply.num_member = num_members;
                    }
                    else {
                        reply.status = FAILURE_NOT_EXISTS;
                    }
                    //send Reply back to client
                    if (send(client_socket[i], &reply, sizeof(Reply), 0) < 0) {
                        LOG(ERROR) << "Failure to send response after join";
                    }
                    close(client_socket[i]);
                    client_socket[i] = 0;
                }
            }
            else if (curr_word == "LIST") {
                LOG(INFO) << "Client called list";
                string room_list = "";
                if (chat_room_db.size() > 0) {
                    reply.status = SUCCESS;
                    for (int i = 0; i < chat_room_db.size(); i++) {
                        room_list += chat_room_db.at(i).room_name + ",";
                    }
                }
                else {
                    reply.status = SUCCESS;
                    room_list = "empty";
                }
                strcpy(reply.list_room, room_list.c_str());
                //send Reply back to client
                if (send(client_socket[i], &reply, sizeof(Reply), 0) < 0){
                    LOG(ERROR) << "Failure to send response after LIST";
                }
            }
            else if (curr_word == "DELETE") {
                LOG(INFO) << "Client called delete";
                //get next word after join
                if (curr_word.length() > 0) {
                    curr_word = strtok(NULL, " ");
                    room_name = curr_word;
                    LOG(INFO) << "Delete room " << room_name;
                    if (does_room_exist(room_name, chat_room_db)) {
                        reply.status = SUCCESS;
                        //find chat_room_fd
                        int chat_room_fd;
                        for (auto j : fd_to_chat_room) {
                            if (j.second.room_name == room_name) {
                                chat_room_fd = j.first;
                                break;
                            }
                        }
                        //Close all clients that are set with room
                        string closing_message_str = "Warning:the chatting room is going to be closed...";
                        char closing_message[closing_message_str.length() + 1];
                        strcpy(closing_message, closing_message_str.c_str());
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (client_socket[j] > 0 && client_room_name[j] == room_name) {
                                //send closing message
                                if (send(client_socket[j], closing_message, sizeof(closing_message), 0) < 0) {
                                    LOG(ERROR) << "Failure to send closing message to client socket";
                                }
                                close(client_socket[j]);
                                client_socket[j] = 0;
                                client_room_name[j] = "";
                            }
                        }
                        //delete chat room from the map and database
                        Chat_Room chat_room = fd_to_chat_room[chat_room_fd];
                        fd_to_chat_room.erase(chat_room_fd);
                        int cr_index;
                        for (int j = 0; j < chat_room_db.size(); j++) {
                            if (chat_room_db.at(j).room_name == room_name) {
                                cr_index =  j;
                                break;
                            }
                        }
                        chat_room_db.erase(chat_room_db.begin() + cr_index);
                        close(chat_room_fd);
                        for (int j = 0; j < MAX_CHAT_ROOM; j++) {
                            if (chat_room_socket[j] == chat_room_fd) {
                                chat_room_socket[j] = 0;
                                break;
                            }
                        }
                    }
                    else {
                        reply.status = FAILURE_NOT_EXISTS;
                    }
                    //send Reply back to client
                    if (send(client_socket[i], &reply, sizeof(Reply), 0) < 0){
                        LOG(ERROR) << "Failure to send response after DELETE";
                    }
                }
            }
            //incoming chat message from client in a room
            else if (client_socket[i] > 0 && client_room_name[i] != "") {
                char cr_message[cr_message_str.length() + 1];
                strcpy(cr_message, cr_message_str.c_str());
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (i != j && client_socket[j] > 0 && client_room_name[j] == client_room_name[i]) {
                        if (send(client_socket[j], cr_message, sizeof(cr_message), 0) < 0) {
                            LOG(ERROR) << "Failure to send chat room message";
                        }
                    }
                }
            }

            }
        }
        }
    }

    LOG(INFO) << "Server ending";
    
 
}


void add_chat_room_fd(int chat_room_socket[], int chat_room_fd) {
    for (int i = 0; i < MAX_CHAT_ROOM; i++) {
        if (chat_room_socket[i] == 0) {
            chat_room_socket[i] = chat_room_fd;
            break;
        }
    }
}

bool does_room_exist(string room_name, vector<Chat_Room> chat_room_db) {
    //iterate through chat room
    for (int i = 0; i < chat_room_db.size(); i++) {
        if (room_name == chat_room_db.at(i).room_name) {
            return true;
        }
    }
    return false;
}

int init_socket(int port) {
    //Create server socket
    int socket_fd;
    
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOG(ERROR) << "Socket creation failure";
        exit(1);
    }

    //Attach socket to given port
    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        LOG(ERROR) << "Setsockpot failure";
        exit(1);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    //Bind socket to given port
    if (bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        LOG(ERROR) << "Bind failure";
        exit(1);
    }

    if (listen(socket_fd, (MAX_CHAT_ROOM + 1) * MAX_MEMBER) < 0) {
        LOG(ERROR) << "Listen failure";
        exit(1);
    }

    return socket_fd;
}

