#include <iostream>
#include <fstream>

#include "third_party/jsoncpp/json.h"
#include "third_party/logle/base/string.h"
#include "third_party/logle/json_reader.h"
#include "third_party/logle/util/logging.h"

namespace tervuren {

std::unique_ptr<Json::Value> GetJsonDoc(std::istream* json_stream) {
  Json::Reader json_reader;
  std::unique_ptr<Json::Value> json_doc(new Json::Value);
  bool success = json_reader.parse(
    *json_stream, *json_doc, false /*Do not parse comments*/);
  CHECK(success, "File is not in JSON format.");
  CHECK(json_doc->isObject(), "JSON object is not a dict.");
  return json_doc;
}

FullJson::FullJson(std::istream* json_stream) {
  json_doc_ = GetJsonDoc(json_stream);
  members_ = json_doc_->getMemberNames();
}

bool FullJson::HasNext() {
  return current_index_ < members_.size();
}

const Json::Value* FullJson::Next() {
  CHECK(HasNext(), "Called Next after last object is received.");
  std::string object_id = members_[current_index_];
  ++current_index_;
  Json::Value* json_object;
  json_object = &((*json_doc_)[object_id]);
  return json_object;
}

FullJson::~FullJson() {
}

StreamJson::StreamJson(std::istream* json_stream) {
  input_file_ = json_stream;
}

bool StreamJson::HasNext() {
  return  input_file_->peek() != '\n' && !input_file_->eof();
}

const Json::Value* StreamJson::Next() {
  CHECK(HasNext(), "Called Next at the end of a stream.");

  Json::Reader json_reader;
  std::string line;
  getline(*(input_file_), line);

  bool success = json_reader.parse(
    line.c_str(), current_object_, false /*Do not parse comments*/);
  CHECK(success, "Line is not in JSON format");

  return &(current_object_);
}

StreamJson::~StreamJson() {
}

}  // namespace tervuren
