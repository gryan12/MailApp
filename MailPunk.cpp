#include "imap.hpp"
#include <libetpan/libetpan.h> 
#include "UI.hpp"
#include <sstream> 
#include <memory>
using namespace IMAP; 


void printMessages(Message **messageList) {
	std::string message_body; 
	for (size_t i = 0; messageList[i] != NULL; i++) {
		message_body = messageList[i]->getBody(); 
		std::cout << "MESSAGE NUMBER: " << i+1 <<"\n" 
			  <<  message_body << "\n";  
	}
}


void printHeaders(Message **messageList, std::string header) {
	std::string headerField; 
	for (size_t i = 0; messageList[i] != NULL; i++) {
		headerField = messageList[i]->getField(header); 
		std::cout << "\nMESSAGE NUMBER: " << i+1 <<"\n" 
			  <<  headerField << "\n";  
	}
}

int main() {

	std::cout <<"\n=====TYRING SAME WITH SESSION OBJECT=======\n";
	Session new_session; 
	new_session.connect("mailpunk.lsds.uk", 143); 
	new_session.login("gr1015mail", "deb252fc"); 
	new_session.selectMailbox("INBOX"); 

	std::cout<<"\nMOVING ONTO EXTRACTING MESSAGES\n"; 

	Message **messageList; 
	messageList = new_session.getMessages(); 
	size_t message_length = new_session.getListSize(); 

	printMessages(messageList); 

	messageList = new_session.getMessages(); 
	printMessages(messageList); 

	messageList = new_session.getMessages(); 
	printMessages(messageList); 
	messageList = new_session.getMessages(); 
	printMessages(messageList); 
	messageList = new_session.getMessages(); 
	printMessages(messageList); 
	messageList = new_session.getMessages(); 
	printMessages(messageList); 





//	std::cout<<"============EXPERIEMNTING HEADERS================"; 
//
//	printHeaders(messageList, "SUBJECT"); 
//
//	int no_message; 
//	no_message = new_session.get_mailbox_message_no_status(); 
//
//	std::cout <<"\nNumber of messages: " << no_message << "\n"; 
	//messageList[1]->deleteFromMailbox(); 

	return 0; 
}



//int main(int argc, char** argv) {
//	auto elements = std::make_unique<UI>(argc, argv);
//	return elements->exec();
//}
