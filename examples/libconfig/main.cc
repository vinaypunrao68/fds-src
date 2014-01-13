#include <iostream>
#include <libconfig.h++>
#include <string>
#include <exception>

using namespace std;
using namespace libconfig;

int main() {
	Config cfg;
	try{
		cfg.readFile("example.cfg");
		string name = cfg.lookup("fds.name");
		cout << "Name: " << name << endl;
	} catch (exception &e) {
		cout << e.what() << endl; 
		return -1;
	}
	return 0;
}
