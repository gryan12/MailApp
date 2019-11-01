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


	Message *message1; 
	std::cout <<"\nHelo1\n"; 
	message1 = message_list[1]; 
	std::cout <<"\nHelo2\n"; 

	std::cout << "\nprebody\n"; 
	std::string body; 
	std::cout << "\npostbody\n"; 

	std::cout << "\nprebodycall\n"; 
	body = message1->getBody(); 

	std::cout <<"\ntest1: " << body << "\n"; 


	return 0; 
}



//int main(int argc, char** argv) {
//	auto elements = std::make_unique<UI>(argc, argv);
//	return elements->exec();
//}
