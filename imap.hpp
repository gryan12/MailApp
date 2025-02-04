#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <map>
#include <string>
#include <functional>

namespace IMAP {
class Session; 
class Message {
private: 
	/*data members*/
	uint32_t uid; 
	Session *session_; 
	std::string body_; 
	std::map<std::string, std::string> headers_; 


public:
	Message(uint32_t uid, Session *session); 
	Message(); 
	/**
	 * Get the body of the message. You may chose to either include the headers or not.
	 */
	std::string getBody();
	/**
	 * Get one of the descriptor fields (subject, from, ...)
	 */
	std::string getField(std::string fieldname);
	/**
	 * Remove this mail from its mailbox
	 */
	void deleteFromMailbox();

	/* static functions for parsing message data from mailimap msg_att structures */
	static uint32_t parse_uid(struct mailimap_msg_att *msg_att); 
	static char* parse_body(struct mailimap_msg_att *msg_att); 
	static char* parse_field(struct mailimap_msg_att *msg_att); 

	uint32_t get_uid(); 

	static struct mailimap* set_parent_session(struct mailimap *imap); 

	void setBody(const std::string &body); 
	void setHeaders(const std::map<std::string, std::string> &headers); 
	void addHeader(const std::string &header, const std::string &content); 
	

	~Message(); 
};

class Session {
private: 
	/* data members */ 
	struct mailimap *imap_session; 
	std::string current_mailbox; 
	Message** messageList_; 

	/* member functions */ 
	void deallocateMessages(); 
	void deleteMessages(); 

	bool fetchMessageBody(Message *message); 
	bool fetchMessageHeader(Message *message, const std::string &headerToFetch); 
	std::function<void()> updateUI_; 
public:
	Session(std::function<void()> updateUI);
	Session();

	void deleteMessage(Message *message); 

	size_t getListSize(); 
	bool listEmpty(); 

	int fetchMailboxNumberStatus(); 

	/**
	 * Get all messages in the INBOX mailbox terminated by a nullptr (like we did in class)
	 */
	Message** getMessages();

	bool fetchMessages(); 


	/**
	 * connect to the specified server (143 is the standard unencrypte imap port)
	 */
	void connect(std::string const& server, size_t port = 143);

	/**
	 * log in to the server (connect first, then log in)
	 */
	void login(std::string const& userid, std::string const& password);

	/**
	 * select a mailbox (only one can be selected at any given time)
	 * 
	 * this can only be performed after login
	 */
	void selectMailbox(std::string const& mailbox);

	~Session();
};
}

#endif /* IMAP_H */
