#include "Message.h"

using namespace std;

string Message::generateRandomChar()
{
  string temp[10] = {"@", "#", "$", "&", "?", "+", "~", "=", "^", "/"};
  return temp[rand()%10];
}

Message::Message(int id) {
  this->id = id;
  this->symbol = generateRandomChar();
}
