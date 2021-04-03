#include "Bus_Base.hpp"
#include <ctime>
int main(int argc, char** argv){
	srand(time(NULL));
	if(argc > 1){
		otherfunc(atof(argv[1]));
	} else {
		otherfunc(rand()%20);
	}
}
