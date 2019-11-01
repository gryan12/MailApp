#include "imap.hpp"
#include <string> 
#include <string.h>
#include <stdio.h>
#include <iostream> 
using namespace IMAP;

struct mailimap* Message::parent_session = NULL; 
	

 uint32_t Message::parse_uid(struct mailimap_msg_att *msg_att) {
        clistiter *cur;
        uint32_t uid = 0;

        for (cur = clist_begin(msg_att -> att_list); cur != NULL ; cur = clist_next(cur)) {

                auto item = (struct mailimap_msg_att_item*)clist_content(cur);

                if (item->att_type == MAILIMAP_MSG_ATT_ITEM_STATIC) {
                        if (item->att_data.att_static->att_type == MAILIMAP_MSG_ATT_UID) {
                                uid = item->att_data.att_static->att_data.att_uid;
                                return uid;
                        }
                }
        }
        return uid;
}

 char* Message::parse_body(struct mailimap_msg_att *msg_att) {
	std::cout <<"\nparsestart\n"; 
        clistiter *cur;
        char *body = NULL; 

        for (cur = clist_begin(msg_att->att_list); cur != NULL; cur = clist_next(cur)) {
		std::cout <<"\nat list start\n"; 

                auto item = (struct mailimap_msg_att_item*)clist_content(cur);
		
		std::cout <<"\nmade item\n"; 

                if (item->att_type == MAILIMAP_MSG_ATT_ITEM_STATIC) {
				std::cout<<"\nfnd stat: \n"; 
				std::cout << " type " << item-> att_type; 
				std::cout << item->att_data.att_static->att_type; 
                        if (item-> att_data.att_static->att_type == MAILIMAP_MSG_ATT_BODY_SECTION) {
				std::cout<<"\nfndbody: "; 
                                body = item->att_data.att_static->att_data.att_body_section->sec_body_part;
				return body; 
                        }
                }
		std::cout << "\noutside loop\n"; 
        }
	if (body == NULL) {
		std::cout << "\nBODY IS NULL\n"; 
	}
        return NULL;
}


Message::Message(uint32_t uid) {
	this->uid = uid; 	
}

Message::Message() {
	this->uid = 0; 	
}

uint32_t Message::get_uid() {
	return this->uid; 
}

std::string Message::getBody() {
        clistiter *cur;
        struct mailimap_set *set;
        struct mailimap_section *section;
        size_t msg_len;
        char *msg_content;
        struct mailimap_fetch_type *fetch_type;
        struct mailimap_fetch_att *fetch_att;
        int res;
        clist *fetch_result;

        set = mailimap_set_new_single(this->uid);
        fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
        section = mailimap_section_new(NULL); 
        fetch_att = mailimap_fetch_att_new_body_peek_section(section);
	std::cout <<"\n===DEBUG1====\n"; 
        mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);
	std::cout <<"\n===DEBUG2====\n"; 

	std::cout <<"\n===DEBUG3====\n"; 
        res = mailimap_uid_fetch(Message::parent_session, set, fetch_type, &fetch_result);
	std::cout <<"\n===DEBUG4====\n"; 
	std::cout <<"\nResponse code: " << res; 
	std::cout <<"\n===DEBUG5====\n"; 

        //this should only be of size one, so mb add checks for this
        for (cur = clist_begin(fetch_result) ; cur != NULL ; cur = clist_next(cur)) {
	std::cout <<"\n===DEBUG6====\n"; 
                auto msg_att = (struct mailimap_msg_att*)clist_content(cur);
                msg_content = Message::parse_body(msg_att);
		
		std::cout <<"\n===DEBUG7====\n"; 
		//for whatever reason, this line does not seem to work
		if (msg_content != NULL) {
			std::cout <<"\n===DEBUG8====\n"; 
			std::string message_body(msg_content);
			std::cout <<"\n===DEBUG9====\n"; 
			mailimap_fetch_list_free(fetch_result); 
			return message_body;
		}
        }


	mailimap_fetch_list_free(fetch_result); 
        return NULL;
}


std::string Message::getField(std::string fieldname) {

	return "hello world"; 
}

void Message::deleteFromMailbox() {

	std::cout <<"hi"<<"\n"; 
}

struct mailimap* Message::set_parent_session(struct mailimap *imap) {
	parent_session = imap; 
}


//===========END MESSAGE==============


Message** Session::getMessages() {

	struct mailimap_set *set;
        struct mailimap_fetch_type *fetch_type;
        struct mailimap_fetch_att *fetch_att;
        clist *result;
        clistiter *cur;
        int res;

        set = mailimap_set_new_interval(1,0);
        fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
        //want uid's
        fetch_att = mailimap_fetch_att_new_uid();
        mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);
        res = mailimap_fetch(this->imap_session, set, fetch_type, &result);
	check_error(res, "could not connect"); 
	std::cout <<"\nResponse code: " << res; 
        //check erros :)
	
	std::cout <<"\nresult size: " << clist_count(result) << "\n"; 

        //get the size of the result array, use to declare messages array length
        int count = 0;
        count = clist_count(result);
        //list of messages to return
        Message **messages = new Message*[count+1]; 

        //final index make null pointer
        messages[count] = NULL;

        count = 0;
        for (cur = clist_begin(result) ; cur != NULL ; cur = clist_next(cur)) {
                Message *msg_ptr;
                uint32_t uid;
                auto msg_att = (struct mailimap_msg_att*)clist_content(cur);
                uid = Message::parse_uid(msg_att);

		std::cout <<"\nat UID section, uid: " << uid << "\n"; 

                //if not 0
                if (uid) {
                        msg_ptr = new Message(uid);
                        messages[count] = msg_ptr;
                        count++;
                }
        }

        //terminate with nullptr;
        messages[count] = NULL;

        //deallocate
        mailimap_fetch_list_free(result);

        //return message list
        return messages;

}

Session::Session(std::function<void()>) {
	std::cout <<"\nsession object created\n"; 
	this->imap_session = mailimap_new(0,NULL); 
	Message::set_parent_session(this->imap_session); 
}

Session::Session() {
	std::cout <<"\nsession object created\n"; 
	this->imap_session = mailimap_new(0,NULL); 
	Message::set_parent_session(this->imap_session); 
}
void Session::connect(std::string const& server, size_t port) {
        int res;
	int length = server.length(); 

	char serv[length+1]; 
	strcpy(serv, server.c_str()); 
        res = mailimap_socket_connect(this->imap_session, serv, port);
	check_error(res, "could not connect"); 
	std::cout <<"\nResponse code: " << res; 
}

void to_char_array(std::string const& string, char *char_array) {
	strcpy(char_array, string.c_str()); 
}


void Session::login(std::string const& userid, std::string const&password) {
        int res;
	int useridlength, passlength; 

	useridlength = userid.length(); 
	passlength = password.length(); 

	char userid_arr[useridlength+1]; 
	char password_arr[passlength+1]; 

	strcpy (userid_arr, userid.c_str()); 
	strcpy(password_arr, password.c_str()); 

        res = mailimap_login(this->imap_session, userid_arr, password_arr);
	check_error(res, "could not login"); 
	std::cout <<"\nResponse code: " << res; 
}


void Session::selectMailbox(std::string const& mailbox) {
        int res;
	int length = mailbox.length(); 

	char mailarr[length+1]; 
	strcpy(mailarr, mailbox.c_str()); 

        res = mailimap_select(this->imap_session, mailarr);
	std::cout <<"\nResponse code: " << res; 
}

Session::~Session() {
        int res;
        res = mailimap_logout(this->imap_session);
        mailimap_free(this->imap_session);
}



