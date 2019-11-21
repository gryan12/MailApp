#include "imap.hpp"
#include <vector>
#include <map>
#include <string> 
#include <string.h>
#include <stdio.h>
#include <iostream> 
#include <functional>


using namespace IMAP;

/* internal helper functions for string procesing */

/* function to remove white spaces from string */
std::string strip(std::string &str) {
	str.erase(0, str.find_first_not_of(' '));
	str.erase(str.find_last_not_of(' ')+1); 
	return str; 
}


/*internal helper function for string processing */ 
//allocates cstring on heap, returns address
char* stringToCharArray(std::string str) {
	int length = str.length(); 
	char *char_array = new char[length+1]; 
	strcpy(char_array, str.c_str()); 
	return char_array; 
}

/*end internal helper functions */



/*static parsing functions*/

/*returns uid if contained in @msg_att, else returns 0*/
 uint32_t Message::parseUid(struct mailimap_msg_att *msg_att) {
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

/*returns body_section if contained in @msg_att, else returns NULL */ 
 char* Message::parseBodySection(struct mailimap_msg_att *msg_att) {
        clistiter *cur;
        char *body = NULL; 

        for (cur = clist_begin(msg_att->att_list); cur != NULL; cur = clist_next(cur)) {
                auto item = (struct mailimap_msg_att_item*)clist_content(cur);
                if (item->att_type == MAILIMAP_MSG_ATT_ITEM_STATIC) {
                        if (item-> att_data.att_static->att_type == MAILIMAP_MSG_ATT_BODY_SECTION) {
                                body = item->att_data.att_static->att_data.att_body_section->sec_body_part;
				return body; 
                        }
                }
        }
        return NULL;
}


/* CONSTRUCTORS */
Message::Message(uint32_t uid, Session *session):uid_(uid), session_(session){}

Message::Message() {
	uid_ = 0; 	
}

Message::~Message() {}


/*SETTERS*/
void Message::setBody(const std::string &body) {
	body_ = body; 
}
void Message::setHeaders(const std::map<std::string,std::string> &headers) {
	headers_ = headers; 
}
void Message::addHeader(const std::string &header, const std::string &content) {
	headers_[header] = content; 
}


/* GETTERS */ 
uint32_t Message::getUid() {
	return uid_; 
}

std::string Message::getBody() {
	return body_; 
}

/*if @fieldname present in headers_, return value */ 
std::string Message::getField(std::string fieldname) {
	if (headers_.find(fieldname) == headers_.end()) {
		//mb throw error? 
		return NULL; 
	}
	else {
		return headers_.at(fieldname); 
	}
}



/*remove message from mailbox */
void Message::deleteFromMailbox() {
	session_->deleteMessage(this); 
}

//===========END MESSAGE==============

/*makes fetch request for body of @message. returns true if successful*/
bool Session::fetchMessageBody(Message *message) {
        clistiter *cur;
        struct mailimap_set *set;
        struct mailimap_section *section;
        char *msg_content;
        struct mailimap_fetch_type *fetch_type;
        struct mailimap_fetch_att *fetch_att;
        int res;
        clist *fetch_result;

        set = mailimap_set_new_single(message->getUid());
        fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
        section = mailimap_section_new(NULL); 
        fetch_att = mailimap_fetch_att_new_body_peek_section(section);
        mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);

	//fetch, check for errors
        res = mailimap_uid_fetch(imapSession_, set, fetch_type, &fetch_result);

        //this should only be of size one, so mb add checks for this
        for (cur = clist_begin(fetch_result) ; cur != NULL ; cur = clist_next(cur)) {
                auto msg_att = (struct mailimap_msg_att*)clist_content(cur);
                msg_content = Message::parseBodySection(msg_att);
		
		//for whatever reason, this line does not seem to work
		if (msg_content != NULL) {
			std::cout <<"\nDEBUG HELP\n"; 
			std::string message_body(msg_content);
			message->setBody(message_body);

			mailimap_fetch_list_free(fetch_result); 
			return true;
		}
        }

	mailimap_fetch_list_free(fetch_result); 
	return false; 
}

/*makes fetch request for header described by @fieldname */ 
bool Session::fetchMessageHeader(Message *message, const std::string &fieldname) {
        size_t msg_len;
        char *msg_content;
        int res;
        clist *fetch_result;
	clist *headers = clist_new(); 
	//allocation 
	char *c_field = new char[fieldname.length()+1]; 
	clist_append(headers, c_field); 
	strcpy(c_field, fieldname.c_str()); 

	struct mailimap_header_list *header_list = mailimap_header_list_new(headers); 
	struct mailimap_section *section = mailimap_section_new_header_fields(header_list); 
	struct mailimap_fetch_att *fetch_att = mailimap_fetch_att_new_body_peek_section(section); 
        struct mailimap_set *set = mailimap_set_new_single(message->getUid());
        struct mailimap_fetch_type *fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
        mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);

        res = mailimap_uid_fetch(imapSession_, set, fetch_type, &fetch_result);
	check_error(res, "fetch request failed"); 

        clistiter *cur;
        for (cur = clist_begin(fetch_result) ; cur != NULL ; cur = clist_next(cur)) {
                auto msg_att = (struct mailimap_msg_att*)clist_content(cur);
                msg_content = Message::parseBodySection(msg_att);
		
		if (msg_content != NULL) {
			std::string message_body(msg_content);

			mailimap_fetch_list_free(fetch_result); 
			clist_free(headers); 
			message_body =  message_body.substr(message_body.find(":")+1); 
			strip(message_body); 

			message->addHeader(fieldname, message_body); 
			return true; 
		}
        }

	mailimap_fetch_list_free(fetch_result); 
	clist_free(headers); 
	return false; 
}



//bool Session::fetchHeaders(Message *message) {
//	size_t msg_len ; 
//	char *msg_content; 
//	int res; 
//	clist *fetchResult; 
//	clist *headers = clist_new(); 
//	std::vector<std::string> fields; 
//	std::string s1 = "From"; 
//	std::string s2 = "Subject"; 
//	fields.push_back(s1); 
//	fields.push_back(s2); 
//		
//	std::cout <<"\nCheck 0\n"; 
//	for (int i = 0; i < 2; i++) {
//		char toAppend[fields[i].size()+1];
//		clist_append(headers, toAppend); 
//		strcpy(toAppend, fields[i].c_str()); 
//	}
//
//	struct mailimap_header_list *header_list; 
//	struct mailimap_section *section; 
//	struct mailimap_fetch_att *fetch_att; 
//	struct mailimap_set *set; 
//	struct mailimap_fetch_type *fetch_type; 
//
//
//	header_list = mailimap_header_list_new(headers); 
//	section = mailimap_section_new_header_fields(header_list); 
//	fetch_att = mailimap_fetch_att_new_body_peek_section(section); 
//	set = mailimap_set_new_single(message->getUid()); 
//	fetch_type = mailimap_fetch_type_new_fetch_att(fetch_att); 
//
//	res = mailimap_uid_fetch(imapSession_, set, fetch_type, &fetchResult); 
//
//	if (clist_isempty(fetchResult)) {
//		return false; 
//	}
//	clistiter *cur; 
//	cur = clist_begin(fetchResult); 
//	auto msg_att = (struct mailimap_msg_att*)clist_content(cur); 
//
//
//	char *header = NULL; 
//	for (cur = clist_begin(msg_att->att_list); cur != NULL; cur = clist_next(cur)) {
//		auto item = (struct mailimap_msg_att_item*)clist_content(cur); 
//
//		if (item->att_type == MAILIMAP_MSG_ATT_ITEM_STATIC) {
//			if (item->att_data.att_static->att_type == MAILIMAP_MSG_ATT_BODY_SECTION) {
//				header = item->att_data.att_static->att_data.att_body_section->sec_body_part; 
//				size_t si; 
//				si = item->att_data.att_static->att_data.att_body_section->sec_length; 
//				std::string newHeader(header); 
//				
//				std::cout << " H: " << newHeader << "L: " << si << " : " << newHeader.length(); 
//			}
//		}
//	}
//
//	mailimap_fetch_list_free(fetchResult); 
//	clist_free(headers); 
//	return true; 
//}


/*makes store request to set deleted flag for @message, then expunges the mailbox */
void Session::deleteMessage(Message *message) {
	int res; 
	struct mailimap_set *set = mailimap_set_new_single(message->getUid()); 
	struct mailimap_flag_list *flags = mailimap_flag_list_new_empty(); 
	struct mailimap_flag *flag = mailimap_flag_new_deleted(); 
	struct mailimap_store_att_flags *store_att; 

	res = mailimap_flag_list_add(flags, flag); 
	check_error(res, "error adding flag to flag list"); 
	store_att = mailimap_store_att_flags_new_set_flags(flags); 

	res = mailimap_uid_store(imapSession_, set, store_att); 
	check_error(res, "store request unsuccessful"); 
	
	mailimap_expunge(imapSession_); 
	mailimap_store_att_flags_free(store_att); 
	mailimap_set_free(set); 

	//refresh the list of messages
	if (updateUI_ != NULL) {
		updateUI_(); 
	}
}

/* returns the number of messages in currentMailbox*/
int Session::fetchMailboxNumberStatus() {

	struct mailimap_status_att_list *status_list = mailimap_status_att_list_new_empty(); 
	struct mailimap_mailbox_data_status *status_data; 
	clistiter *cur; 
	int res; 
	res = mailimap_status_att_list_add(status_list, 0); 
	check_error(res, "failed to add to status list"); 

	//allocation
	char *mailbox_arr = stringToCharArray(currentMailbox_); 

	//allocation
	res = mailimap_status(imapSession_, mailbox_arr, status_list, &status_data); 
	check_error(res, "failed to retrieve mailbox status"); 	

	int return_count = clist_count(status_data->st_info_list); 

	int status_value; 
	for (cur = clist_begin(status_data->st_info_list); cur != NULL; cur = clist_next(cur)) {
		auto status_info = (struct mailimap_status_info*)clist_content(cur); 
		status_value = status_info->st_value; 
		return status_value; 
	}

	//deallocation
	mailimap_status_att_list_free(status_list); 
	mailimap_mailbox_data_status_free(status_data); 
	delete mailbox_arr; 
	return 0; 
}

/*requests UIDs for all messages in currentMailBox_, then makes
 * (separate) fetch requests for the body and headers (from and subject)
 * of these messages. response(s) are used to initialise message objects 
 * on heap, pointers to these stored in messageList_ */
bool Session::fetchMessages() {

	//get the number of messages in current mailbox
	int messageNumber; 
	messageNumber = fetchMailboxNumberStatus(); 
	//if no messages in heap, initialise messageList_ on heap with only a single null pointer
	if (messageNumber == 0) {
		std::cout <<"\nMessage number: \n"; 
		messageList_ = new Message*[1]; 
		messageList_[0] = NULL; 
		return true; 
	}

        clist *result;
        clistiter *cur;
        int res;
        struct mailimap_set *set = mailimap_set_new_interval(1,0);
        struct mailimap_fetch_type *fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
        struct mailimap_fetch_att *fetch_att = mailimap_fetch_att_new_uid();
        mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);
        res = mailimap_fetch(imapSession_, set, fetch_type, &result);

        check_error(res, "could not connect");

	//allocate
        messageList_ = new Message*[messageNumber+1];
        messageList_[messageNumber] = NULL;


	//parse response of fetch request
	int count; 
        count = 0;
	Message *msg_ptr;
        for (cur = clist_begin(result) ; cur != NULL ; cur = clist_next(cur)) {
                uint32_t uid;
                auto msg_att = (struct mailimap_msg_att*)clist_content(cur);

                uid = Message::parseUid(msg_att);

                //if not 0
                if (uid) {
                        //allocation alert
                        msg_ptr = new Message(uid, this);
                        messageList_[count] = msg_ptr;
			this->fetchMessageBody(messageList_[count]); 
			this->fetchMessageHeader(messageList_[count], "Subject"); 
			this->fetchMessageHeader(messageList_[count], "From"); 
                        count++;
                } 
        }

        mailimap_fetch_list_free(result);
	std::cout <<"\n IN FETCH \n"; 
        return true;

}

/* frees heap memory allocated for the messageList */
void Session::deallocateMessages() {

	if (getListSize() != 0) {
		size_t size = (sizeof(messageList_)/sizeof(messageList_[0]));

		if (size) {
			for (size_t i = 0; i < size; i++) {
				delete messageList_[i];
			}
		}
	}

        delete[] messageList_;
}

/*retuns the size of messageList_ */
size_t Session::getListSize() {
	if (messageList_ == NULL) {
		return 0; 
	}
	size_t count = 0; 
	while (messageList_[count] != NULL) {
		count++; 
	}
	std::cout <<"\n List size: " << count <<'\n';
	return count; 
}


/*returns true if getListSize() returns 0 (messageList is empty)*/ 
bool Session::listEmpty() {
	if (getListSize() == 0) {
		return true; 
	}
	return false; 
}

/* returns messageList_, the list of pointers to message objects
 * retrieved from currentMailbox_. if list is not null, then memory allocated for
 * messageList is first freed, then makes fetch request for messages from the server (where memory is reallocated) */

Message** Session::getMessages() {
	if (messageList_== NULL) {
		std::cout <<"\nLIST NULL\n"; 
		fetchMessages(); 
	} else if (messageList_!= NULL) {
		deallocateMessages(); 
		fetchMessages(); 
	}
	return messageList_; 
}

/*connect to specified server */
void Session::connect(std::string const& server, size_t port) {
        int res;
	int length = server.length(); 

	char serv[length+1]; 
	strcpy(serv, server.c_str()); 
        res = mailimap_socket_connect(imapSession_, serv, port);
	check_error(res, "could not connect"); 
	std::cout <<"\nResponse code: " << res; 
}


/*log into the server */
void Session::login(std::string const& userid, std::string const&password) {
	std::cout <<"\nAttempting to login\n"; 
        int res;
	int useridlength, passlength; 

	useridlength = userid.length(); 
	passlength = password.length(); 

	char userid_arr[useridlength+1]; 
	char password_arr[passlength+1]; 

	strcpy (userid_arr, userid.c_str()); 
	strcpy(password_arr, password.c_str()); 

        res = mailimap_login(imapSession_, userid_arr, password_arr);
	check_error(res, "could not login"); 
	std::cout <<"\nResponse code: " << res; 
}

/*select mailbox */
void Session::selectMailbox(std::string const& mailbox) {
        int res;
	int length = mailbox.length(); 

	char mailarr[length+1]; 
	strcpy(mailarr, mailbox.c_str()); 

        res = mailimap_select(imapSession_, mailarr);
	std::cout <<"\nResponse code: " << res; 
	if (!res) {
		currentMailbox_= mailbox; 
	}
}

/*parametised construtor */
Session::Session(std::function<void()> updateUI) {
	std::cout <<"\nattempgin tot call parametised constructor \n"; 
	updateUI_ = updateUI; 
	imapSession_ = mailimap_new(0,NULL); 
	messageList_= NULL; 
}

/*default constructor for testing */
Session::Session() {
	std::cout <<"\nsession object created\n"; 
	imapSession_ = mailimap_new(0,NULL); 
	messageList_= NULL; 
}

//destructor
Session::~Session() {
	
	deallocateMessages(); 
        int res;
	//so the below code throws an drror, this was jut called as part
	//of the destrution of my session object 
        //res = mailimap_logout(imapSession_);
	//check_error(res, "failure logging out "); 
        mailimap_free(imapSession_);
}



