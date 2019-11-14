#include "imap.hpp"
#include <vector>
#include <map>
#include <string> 
#include <string.h>
#include <stdio.h>
#include <iostream> 



//what about: 
//having the prev idea of body etc headers 
//but the fetch request(s) are made at the point of 
//creating a message
//i.e. parametised cosntructor takes UID and imap isession, 
//then fills in the body. 
//the only remaining question is how deletion is handled 


using namespace IMAP;


/* helper functions for string procesing */
/* function to remove white spaces from string */
std::string strip(std::string &str) {
	str.erase(0, str.find_first_not_of(' '));
	str.erase(str.find_last_not_of(' ')+1); 
	return str; 
}

//std::string to cstring
//WARNING: Allocation
char* string_to_char_array(std::string str) {
	int length = str.length(); 
	char *char_array = new char[length+1]; 
	strcpy(char_array, str.c_str()); 
	return char_array; 
}

/*end helper functions */

struct mailimap* Message::parent_session = NULL; 

bool Message::shouldDelete() {
	return delete_;  
}

/* =======START STATIC PARSING FUNCTIONS ================ */ 
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

//returns the body of a message as a cstring if one exists
//@param *msg att : a pointer to a list of pointers of message attribute items 
//can call this parse_content aand use for fetching both headers and body 
 char* Message::parse_body(struct mailimap_msg_att *msg_att) {
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


/* ===============END STATIC PARSING FUNCTIONS ================= */ 

/* =================START CONSTRUCTORS ================= */ 
Message::Message(uint32_t uid) {
	this->uid = uid; 	
}

Message::Message() {
	uid = 0; 	
}

/* ======================END CONSTRUCTORS======================== */ 

/* ===============START FETCHING AND RETURNNG =================== */ 
//retuns the body of a message, along with the headers 




/* ==========SETTERS===========*/

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

uint32_t Message::get_uid() {
	return uid; 
}

std::string Message::getBody() {
	return body_; 
}

/*not quite a getter */ 
/* if field present in header map, return that header */ 
std::string Message::getField(std::string fieldname) {
	if (headers_.find(fieldname) == headers_.end()) {
		//mb throw error? 
		return NULL; 
	}
	else {
		return headers_.at(fieldname); 
	}
}


/* ======================END FETCHING AND RETURNING ================== */ 

/* ==================START DELETING ================== */ 
//ok following the packed design where message class has access to the imap session. 
//i feel like it woud be better if this was somehow handled by session byt im not really sure how
//that would work 
void Message::deleteFromMailbox() {
	delete_ = true; 
}

//make bool 
void Message::requestDelete(mailimap *imap_session) {
	int res; 
	struct mailimap_flag_list *flags; 
	struct mailimap_flag *flag; 
	struct mailimap_store_att_flags *store_att; 
	struct mailimap_set *set; 

	set = mailimap_set_new_single(uid); 
	flags = mailimap_flag_list_new_empty(); 
	flag = mailimap_flag_new_deleted(); 
	res = mailimap_flag_list_add(flags, flag); 
	store_att = mailimap_store_att_flags_new_set_flags(flags); 

	res = mailimap_uid_store(imap_session, set, store_att); 
	check_error(res, "store request unsuccessful"); 
	
	mailimap_expunge(imap_session); 
	mailimap_store_att_flags_free(store_att); 
	//needed?
	mailimap_set_free(set); 
}


/* =========================END DELETING==================*/

struct mailimap* Message::set_parent_session(struct mailimap *imap) {
	parent_session = imap; 
}

Message::~Message() {

}


//===========END MESSAGE==============

/* DEALIGN WITH MAILBOX STATUS */ 
/* HERE SHOULD FETCH OTHER STUFF, USE FOR ERROR CODES
 * AND FOR POPULATING IWTH MESSAGE NUMBER AND STUFF */ 


/* ==========SPECIFIC FETCHING ============= */ 
bool Session::fetchMessageBody(Message *message) {
	std::cout <<"\nFetchMessageBody\n"; 
        clistiter *cur;
        struct mailimap_set *set;
        struct mailimap_section *section;
        char *msg_content;
        struct mailimap_fetch_type *fetch_type;
        struct mailimap_fetch_att *fetch_att;
        int res;
        clist *fetch_result;

        set = mailimap_set_new_single(message->get_uid());
        fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
        section = mailimap_section_new(NULL); 
        fetch_att = mailimap_fetch_att_new_body_peek_section(section);
        mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);

	//fetch, check for errors
        res = mailimap_uid_fetch(imap_session, set, fetch_type, &fetch_result);

        //this should only be of size one, so mb add checks for this
        for (cur = clist_begin(fetch_result) ; cur != NULL ; cur = clist_next(cur)) {
                auto msg_att = (struct mailimap_msg_att*)clist_content(cur);
                msg_content = Message::parse_body(msg_att);
		
		//for whatever reason, this line does not seem to work
		if (msg_content != NULL) {
			std::string message_body(msg_content);
			message->setBody(message_body);

			mailimap_fetch_list_free(fetch_result); 
			return true;
		}
        }

	mailimap_fetch_list_free(fetch_result); 
	return false; 
}

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
        struct mailimap_set *set = mailimap_set_new_single(message->get_uid());
        struct mailimap_fetch_type *fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
        mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);

        res = mailimap_uid_fetch(imap_session, set, fetch_type, &fetch_result);
	check_error(res, "fetch request failed"); 

        //this should only be of size one, so mb add checks for this
        clistiter *cur;
        for (cur = clist_begin(fetch_result) ; cur != NULL ; cur = clist_next(cur)) {
                auto msg_att = (struct mailimap_msg_att*)clist_content(cur);
                msg_content = Message::parse_body(msg_att);
		
		//for whatever reason, this line does not seem to work
		if (msg_content != NULL) {
			std::string message_body(msg_content);
			//deallocation
			mailimap_fetch_list_free(fetch_result); 
			clist_free(headers); 
			message_body =  message_body.substr(message_body.find(":")+1); 
			strip(message_body); 
			//add to map
			message->addHeader(fieldname, message_body); 

			return true; 
		}
        }

	//deallocation; check if works
	mailimap_fetch_list_free(fetch_result); 
	//go back and use malloc i think 
	clist_free(headers); 
	return false; 
}

void Session::deleteMessages() {
	if (messageList_ == NULL) {
		return; 
	}
	std::cout << " \n in delete \n"; 
	clist *toDelete; 
	for (int i = 0; messageList_[i] != NULL; i++) {
		if (messageList_[i]->shouldDelete()) {
			messageList_[i]->requestDelete(imap_session); 
		}
	}
	mailimap_expunge(imap_session); 
}




int Session::get_mailbox_message_no_status() {
	struct mailimap_status_att_list *status_list = mailimap_status_att_list_new_empty(); 
	struct mailimap_mailbox_data_status *status_data; 
	clistiter *cur; 
	int res; 
	res = mailimap_status_att_list_add(status_list, 0); 
	check_error(res, "failed to add to status list"); 

	//allocation
	char *mailbox_arr = string_to_char_array(current_mailbox); 

	//allocation
	res = mailimap_status(imap_session, mailbox_arr, status_list, &status_data); 
	check_error(res, "failed to retrieve mailbox status"); 	

	int return_count = clist_count(status_data->st_info_list); 

	int status_value; 
	for (cur = clist_begin(status_data->st_info_list); cur != NULL; cur = clist_next(cur)) {
		auto status_info = (struct mailimap_status_info*)clist_content(cur); 
		status_value = status_info->st_value; 
		std::cout <<"\ngot to internal return\n"; 
		return status_value; 
	}
	std::cout <<"\nGot to end of function\n"; 

	//deallocation
	mailimap_status_att_list_free(status_list); 
	mailimap_mailbox_data_status_free(status_data); 
	delete mailbox_arr; 
	return 0; 
}


//NB::Net Allocation 
bool Session::fetchMessages() {
        clist *result;
        clistiter *cur;
        int res;
        struct mailimap_set *set = mailimap_set_new_interval(1,0);
        struct mailimap_fetch_type *fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
        struct mailimap_fetch_att *fetch_att = mailimap_fetch_att_new_uid();
        mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);
        res = mailimap_fetch(imap_session, set, fetch_type, &result);

        check_error(res, "could not connect");

        //get the size of the result array, use to declare messages array length
        int count = 0;
        count = clist_count(result);
	std::cout <<"\n Clist count: " << count << "\n"; 
        //list of messages to return
        //ALLOCATION
        messageList_ = new Message*[count+1];

        //final index make null pointer
        messageList_[count] = NULL;

        count = 0;
	Message *msg_ptr;
        for (cur = clist_begin(result) ; cur != NULL ; cur = clist_next(cur)) {
                uint32_t uid;
                auto msg_att = (struct mailimap_msg_att*)clist_content(cur);
                //these functions perhaps be part of session now, or general imap function
                uid = Message::parse_uid(msg_att);
                std::cout <<"\nat UID section, uid: " << uid << "\n";

                //if not 0
                if (uid) {
                        //allocation alert
                        msg_ptr = new Message(uid);
                        messageList_[count] = msg_ptr;
			std::cout <<"\nAt function calls\n"; 
			this->fetchMessageBody(messageList_[count]); 
			this->fetchMessageHeader(messageList_[count], "SUBJECT"); 
			this->fetchMessageHeader(messageList_[count], "FROM"); 
                        count++;
                } else {
			std::cout << "\nInvalid uid!\n"; 
			return false; 
		}
        }

        //terminate with nullptr;
        mailimap_fetch_list_free(result);
	std::cout <<"\n IN FETCH \n"; 
        return true;

}

void Session::deallocateMessages() {
        size_t size = (sizeof(messageList_)/sizeof(messageList_[0]));

        if (size) {
                for (size_t i = 0; i < size; i++) {
                        delete messageList_[i];
                }
        }
        delete[] messageList_;
}


size_t Session::getListSize() {
	if (messageList_ == NULL) {
		return 0; 
	}
	int count = 0; 
	while (messageList_[count] != NULL) {
		count++; 
	}
	std::cout <<"\n List size: " << count <<'\n';
	return count; 
}

bool Session::listEmpty() {
	if (getListSize() == 0) {
		return true; 
	}
	return false; 
}

//refreshes clients list of messages. 
//first deallocates memory for previous set of 
//messages, then fetches list of messages currently available 
Message** Session::getMessages() {

	deleteMessages(); 	

	std::cout <<"\nget called \n"; 
	if (!listEmpty()) {
		deallocateMessages(); 
	}

	std::cout <<"\nprefetch \n"; 
	fetchMessages(); 

	std::cout <<"\nprereturn\n"; 
	return messageList_; 

}

Session::Session(std::function<void()>) {
	std::cout <<"\nsession object created\n"; 
	imap_session = mailimap_new(0,NULL); 
	Message::set_parent_session(imap_session); 
}

Session::Session() {
	std::cout <<"\nsession object created\n"; 
	imap_session = mailimap_new(0,NULL); 
	Message::set_parent_session(imap_session); 
}
void Session::connect(std::string const& server, size_t port) {
        int res;
	int length = server.length(); 

	char serv[length+1]; 
	strcpy(serv, server.c_str()); 
        res = mailimap_socket_connect(imap_session, serv, port);
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

        res = mailimap_login(imap_session, userid_arr, password_arr);
	check_error(res, "could not login"); 
	std::cout <<"\nResponse code: " << res; 
}


void Session::selectMailbox(std::string const& mailbox) {
        int res;
	int length = mailbox.length(); 

	char mailarr[length+1]; 
	strcpy(mailarr, mailbox.c_str()); 

        res = mailimap_select(imap_session, mailarr);
	std::cout <<"\nResponse code: " << res; 
	if (!res) {
		current_mailbox = mailbox; 
	}
}

Session::~Session() {
	
	deallocateMessages(); 
        int res;
	//so the below code throws an drror, this was jut called as part
	//of the destrution of my session object 
        //res = mailimap_logout(imap_session);
	//check_error(res, "failure logging out "); 

        mailimap_free(imap_session);
}



