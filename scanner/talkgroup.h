#include <string>
class Talkgroup {
public:
	long number;
	char mode;
	std::string alpha_tag;
	std::string description;
	std::string tag;
	std::string group;
	Talkgroup(long num, char m, std::string a, std::string d, std::string t, std::string g);
};