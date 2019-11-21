#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <map>
#include <string>
#include <functional>

/*two classes: Message, Session. 
 * to separate concerns, all communicaiton with server handled
 * through the session class. 
 * for message functions that require server communication such as deletion
 * a poiter to a session object is used. 
 */
namespace IMAP {

class Session; 
class Message {
private: 
	/*data members*/
	/* unique id number of message */
	uint32_t uid_; 

	//the body of the message
	std::string body_; 

	//map of headers vs header-content
	std::map<std::string, std::string> headers_; 

	//session object that called message constructor
	Session *session_; 

public:
	/*parametised constructor. body_ and headers_ are set to null*/ 
	Message(uint32_t uid, Session *session); 

	/*default*/
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

	/* static functions for parsing data from a mailimap_msg_att struct*/
	static uint32_t parseUid(struct mailimap_msg_att *msg_att); 

	static char* parseBodySection(struct mailimap_msg_att *msg_att); 

	//getter
	uint32_t getUid(); 

	//setters
	void setBody(const std::string &body); 
	void setHeaders(const std::map<std::string, std::string> &headers); 
	void addHeader(const std::string &header, const std::string &content); 

	~Message(); 
};

class Session {
private: 
	/* data members */ 
	struct mailimap *imapSession_; 

	/*currently selected mailbox in imapSession_ */
	std::string currentMailbox_; 

	/*array of pointers to message objects that are in
	 * selected mailbox of imapSession_ */
	Message** messageList_; 

	/* function passed to parametised constructor. */ 
	std::function<void()> updateUI_; 

	/* member functions */ 
	/* frees heap memory allocates for the messageList */
	void deallocateMessages(); 

	/*creates fetch request for the body of @message */ 
	bool fetchMessageBody(Message *message); 

	/* creates fetch request for the header desribed by @headerToFetch for 
	 * @message */ 
	bool fetchMessageHeader(Message *message, const std::string &headerToFetch); 

public:
	/*fetches the subject and from headers for @message */ 
	bool fetchHeaders(Message *message); 

	/*parametised constructor */ 
	Session(std::function<void()> updateUI);
	/*default constructor*/ 
	Session();

	/*sets delete flag for @message and expunges
	 * the mailbox */ 
	void deleteMessage(Message *message); 

	/*retuns the size of messageList_ */ 
	size_t getListSize(); 

	/*returns true if messageList_ is null or contains
	 * only null pointers */ 
	bool listEmpty(); 

	/*returns the number of messages in currentMailbox_*/ 
	int fetchMailboxNumberStatus(); 

	
	/* first deallocates messageList_(if not null), then generates new fetch request for
	 * messages and returns messageList_ */ 
	Message** getMessages();

	/*requests the UIDs for all messages in currentMailbox_, then makes
	 * fetch requests for the body and headers of these messages. responses used to 
	 * initialise message objects on heap, pointers stored in messageList_*/ 
	bool fetchMessages(); 

	/*makes fetch request for envelope of a given message */ 
	bool fetchEnvelope(Message *message); 


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
