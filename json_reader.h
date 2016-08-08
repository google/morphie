#ifndef LOGLE_JSON_READER_H
#define LOGLE_JSON_READER_H

#include <fstream>
#include <memory>
#include "third_party/jsoncpp/json.h"

namespace tervuren {

// Contains classes that handle loading of input files in different formats.
// It supports JSON and JSON stream format.
//
// JSON format consists of one monolithic JSON object (dict of objects).
// It is easier to use, but parsing a JSON file has big memory overhead and
// limits usage of big input files. Moreover we do not need all fields from
// objects, so there is no need to have them in memory for more time than
// necessary.

// Each line of JSON stream file contains one JSON object.
// This can be easily read and parsed one line at a time and we only
// need to store one object at a time.

// Interface that handles loading of input JSON file.
class JsonDocumentIterator{
 public:
  virtual ~JsonDocumentIterator() {}

  // Returns whether Next() could be called.
  // This will not change the iterator.
  virtual bool HasNext() = 0;
  // Returns current object and advances the iterator.
  // The iterator initially points to the first object.
  virtual const Json::Value* Next() = 0;
};

// Support for loading objects from one monolithic JSON object.
// This is memory demanding but widely used format.
// Example:
// {"object1": {"field1": "value1"},
// "object2": {"field1": "value2"}
// }
class FullJson: public JsonDocumentIterator{
 public:
  FullJson(std::istream* json_stream);
  ~FullJson();
  bool HasNext();
  const Json::Value* Next();
 private:
  std::unique_ptr<Json::Value> json_doc_;
  // Contains object names (keys from JSON dict).
  Json::Value::Members members_;
  // Index to members_ specifying the name of next object.
  int current_index_ = 0;
};

// Support for JSON stream format.
// Each line of input file is one JSON encoded object
// Example:
// {"field1": "value1"}
// {"field1": "value2"}
//
class StreamJson: public JsonDocumentIterator{
 public:
  StreamJson(std::istream* json_stream);
  ~StreamJson();
  bool HasNext();
  const Json::Value* Next();
 private:
  std::istream* input_file_;
  // contains last read JSON object
  Json::Value current_object_;
};

}  // namespace tervuren

#endif
