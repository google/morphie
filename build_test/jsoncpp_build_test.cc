#include <iostream>
#include "json/json.h"

int main(int argc, char **argv) {
  Json::Value json_doc;
  std::cout << json_doc.asString() << std::endl;
  return 0;
}
