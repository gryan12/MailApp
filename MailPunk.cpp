#include "imap.hpp"
#include <libetpan/libetpan.h> 
#include "UI.hpp"
#include <memory>
using namespace IMAP; 
int main() {

	std::cout <<"\n=====TYRING SAME WITH SESSION OBJECT=======\n";
	Session new_session; 
	new_session.connect("mailpunk.lsds.uk", 143); 
	new_session.login("gr1015mail", "deb252fc"); 
	new_session.selectMailbox("INBOX"); 

	std::cout<<"\nMOVING ONTO EXTRACTING MESSAGES\n"; 

	Message **message_list;
	message_list = new_session.getMessages(); 

	int message_length =1;  
	std::string message_body; 
	std::cout <<"\nMessage length: " << message_length; 
	for (int i = 0; i <= message_length; i++) {
		message_body = message_list[i]->getBody(); 
		std::cout << "MESSAGE NUMBER: " << i <<"\n" 
			  <<  message_body << "\n";  
	}




	std::cout<<"============EXPERIEMNTING HEADERS================"; 

	
	std::cout <<"\nMessage length: " << message_length; 
	for (int i = 0; i <= message_length; i++) {
		std::string field; 
		field = message_list[i]->getField("SUBJECT"); 
		
		std::cout <<"\nField:" << field <<"\n"; 

	}

	int no_message; 
	no_message = new_session.get_mailbox_message_no_status(); 

	std::cout <<"\nNumber of messages: " << no_message << "\n"; 
	//message_list[1]->deleteFromMailbox(); 

	return 0; 
}



//int main(int argc, char** argv) {
//	auto elements = std::make_unique<UI>(argc, argv);
//	return elements->exec();
//}
