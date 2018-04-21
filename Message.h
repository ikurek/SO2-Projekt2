#include <string>

using namespace std;

class Message {
public:
  int id;
  string symbol;
  string status;
  Message(int);
  string generateRandomChar();

};
