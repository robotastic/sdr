#include "talkgroup.h"

Talkgroup::Talkgroup(long num, char m, std::string a, std::string d, std::string t, std::string g) {
	number = num;
	mode = m;
	alpha_tag = a;
	description = d;
	tag = t;
	group = g;
}