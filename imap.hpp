#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <string>
#include <functional>

namespace IMAP {
class Message {
private: 
	uint32_t uid; 
	//probs be poor form to have this, will think about it
	static struct mailimap *parent_session;  

public:
	Message(uint32_t uid); 
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

	static uint32_t parse_uid(struct mailimap_msg_att *msg_att); 
	static char* parse_body(struct mailimap_msg_att *msg_att); 
	static char* parse_field(struct mailimap_msg_att *msg_att); 
	

	uint32_t get_uid(); 

	static struct mailimap* set_parent_session(struct mailimap *imap); 
};

class Session {
private: 
	struct mailimap *imap_session; 
public:
	Session(std::function<void()> updateUI);
	Session();


	/**
	 * Get all messages in the INBOX mailbox terminated by a nullptr (like we did in class)
	 */
	Message** getMessages();

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
