// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations under
// the License.

#include "third_party/logle/plaso_event.h"

#include <iostream>
#include <map>

#include "google/protobuf/message.h"
#include "third_party/logle/plaso_defs.h"
#include "third_party/logle/type.h"
#include "third_party/logle/util/logging.h"
#include "third_party/logle/util/string_utils.h"
#include "third_party/logle/util/time_utils.h"
#include "third_party/logle/value.h"

namespace third_party_logle {
namespace plaso {

namespace type = ast::type;
namespace value = ast::value;

namespace {

namespace proto = google::protobuf;

// A ParseOption is an operation that should be performed on the contents of a
// member of a JSON object provided as input.
enum class ParseOption { kCopy, kMakeFile };

// A ParseAction entry specifies the source and target fields for a parse option
// operation. For example:
//   { {ParseOption::kCopy, {{"url", "source_url"}}},
//     {ParseOption::kMakeFile, {{"display_name", "data_file"},
//                               {"download_file", "target_file"}}}  }
// This vector specifies that 'url' should be copied to 'source_url', while the
// JSON members 'display_name' and 'download_file' should be converted into the
// 'File' messages 'data_file' and 'target_file', respectively.
using ParseAction = vector<std::pair<ParseOption, map<string, string>>>;

// A map that describes how to populate fields in a PlasoEvent proto based on a
// Plaso 'data_type' value. For each Plaso 'data_type' value, the map specifies:
//   - an EventType value. A 'data_type' determines the names and fields that
//     occur in a JSON event object, while an EventType is a conceptual. For
//     example, "chrome:history:file_downloaded" and
//     "firefox:downloads:download" are two different Plaso 'data_type' values
//     that will map to EventType::FILE_DOWNLOADED.
//   - a ParseAction specifying how data about that event should be processed.
const map<string, std::pair<EventType, ParseAction>> kParseActions = {
    {"chrome:cache:entry",
     {EventType::PAGE_VISITED,
      {
          {ParseOption::kCopy, {{"original_url", "source_url"}}},
      }}},
    {"chrome:cookie:entry",
     {EventType::PAGE_VISITED,
      {
          {ParseOption::kCopy, {{"url", "source_url"}}},
      }}},
    {"chrome:extension_activity:activity_log",
     {EventType::PAGE_VISITED,
      {
          {ParseOption::kCopy,
           {{"page_url", "source_url"}, {"extension_id", "extension_id"}}},
      }}},
    {"chrome:history:file_downloaded",
     {EventType::FILE_DOWNLOADED,
      {{ParseOption::kCopy, {{"url", "source_url"}}},
       {ParseOption::kMakeFile, {{"full_path", "target_file"}}}}}},
    {"chrome:history:page_visited",
     {EventType::PAGE_VISITED,
      {
          {ParseOption::kCopy,
           {{"from_visit", "source_url"}, {"url", "target_url"}}},
      }}},
    {"chrome:preferences:extension_installation",
     {EventType::BROWSER_EXTENSION_INSTALLED,
      {
          {ParseOption::kCopy,
           {
               {"extension_name", "extension_name"},
               {"extension_id", "extension_id"},
           }},
      }}},
    {"firefox:cache:record", {EventType::SKIP, {}}},
    {"firefox:cookie:entry", {EventType::SKIP, {}}},
    {"firefox:downloads:download",
     {EventType::FILE_DOWNLOADED,
      {{ParseOption::kCopy, {{"url", "source_url"}}},
       {ParseOption::kMakeFile, {{"full_path", "target_file"}}}}}},
    {"firefox:places:bookmark", {EventType::SKIP, {}}},
    {"firefox:places:bookmark_annotation", {EventType::SKIP, {}}},
    {"firefox:places:bookmark_folder", {EventType::SKIP, {}}},
    {"firefox:places:page_visited",
     {EventType::PAGE_VISITED,
      {
          {ParseOption::kCopy, {{"url", "source_url"}}},
      }}},
    {"macosx:application_usage",
     {EventType::APPLICATION_EXECUTED,
      {{ParseOption::kCopy, {{"application", "application_name"}}}}}},
    {"task_scheduler:task_cache:entry",
     {EventType::APPLICATION_EXECUTED,
      {{ParseOption::kCopy, {{"task_name", "application_name"}}}}}},
    {"windows:evt:record",
     {EventType::APPLICATION_EXECUTED,
      {
          {ParseOption::kCopy, {{"event_identifier", "event_id"}}},
          {ParseOption::kMakeFile, {{"source_name", "target_file"}}},
      }}},
    {"windows:evtx:record",
     {EventType::APPLICATION_EXECUTED,
      {
          {ParseOption::kCopy, {{"event_identifier", "event_id"}}},
          {ParseOption::kMakeFile, {{"source_name", "target_file"}}},
      }}},
    {"windows:prefetch:execution",
     {EventType::APPLICATION_EXECUTED,
      {{ParseOption::kMakeFile, {{"executable", "target_file"}}}}}},
    {"windows:registry:appcompatcache",
     {EventType::APPLICATION_EXECUTED,
      {{ParseOption::kMakeFile, {{"path", "target_file"}}}}}},
    {"windows:shell_item:file_entry",
     {EventType::APPLICATION_EXECUTED,
      {{ParseOption::kMakeFile, {{"name", "target_file"}}}}}},
    {"windows:tasks:job",
     {EventType::APPLICATION_EXECUTED,
      {{ParseOption::kCopy, {{"application", "application_name"}}}}}},
};

string GetJSONField(const string& field_name, const ::Json::Value& json_event) {
  CHECK(json_event.isObject(),
        util::StrCat("GetJSONField requires an object but was ", "called on ",
                     json_event.toStyledString()));
  CHECK(json_event.isMember(field_name),
        util::StrCat("No field named ", field_name, " in the JSON object:\n",
                     json_event.toStyledString()));
  return json_event[field_name].asString();
}

void SetTimestamp(const string& timestamp_str, PlasoEvent* event) {
  int64_t unix_micros;
  bool parsed = util::RFC3339ToUnixMicros(timestamp_str, &unix_micros);
  if (parsed) {
    event->set_timestamp(unix_micros);
  }
}

// For each (key, value) pair in 'field_map', sets a field 'value' in the proto
// 'event' to the contents of the field 'key' in 'json_event'. Crashes if
//   * some 'key' is not a string member of 'json_event'.
//   * some 'value' is not a string member of '*event'.
void CopyJSONToProtoStrings(const Json::Value& json_event,
                            const map<string, string>& field_map,
                            PlasoEvent* event) {
  const proto::Reflection* reflection = event->GetReflection();
  const proto::FieldDescriptor* field;
  for (const auto& field_pair : field_map) {
    field = (event->GetDescriptor())->FindFieldByName(field_pair.second);
    CHECK(field != nullptr,
          util::StrCat("The PlasoEvent proto has no field named ",
                       field_pair.second));
    CHECK(field->type() == proto::FieldDescriptor::TYPE_STRING,
          util::StrCat("The field ", field_pair.second, " is not a string."));
    reflection->SetString(event, field,
                          GetJSONField(field_pair.first, json_event));
  }
}

// For each (key, value) pair in 'field_map', set the field named 'value' in
// 'event' to a File message derived from 'key'.  Crashes if 'value' is not a
// string member of 'event'. Crashes if
//   * some 'key' is not a string member of 'json_event'.
//   * some 'value' is not a File message in '*event'.
void SetFileFields(const Json::Value& json_event,
                   const map<string, string>& field_map, PlasoEvent* event) {
  const proto::Reflection* reflection = event->GetReflection();
  const proto::FieldDescriptor* field;
  for (const auto& field_pair : field_map) {
    field = (event->GetDescriptor())->FindFieldByName(field_pair.second);
    CHECK(field != nullptr,
          util::StrCat("The PlasoEvent proto has no field named ",
                       field_pair.second));
    File file = ParseFilename(GetJSONField(field_pair.first, json_event));
    proto::Message* m = reflection->MutableMessage(event, field);
    m->CopyFrom(file);
  }
}

void SetEventFields(const Json::Value& json_event, PlasoEvent* event) {
  event->set_desc(GetJSONField(plaso::kDescriptionName, json_event));
  string plaso_type = GetJSONField(plaso::kDataTypeName, json_event);
  auto action_it = kParseActions.find(plaso_type);
  if (action_it == kParseActions.end()) {
    event->set_type(EventType::DEFAULT);
  } else {
    event->set_type((action_it->second).first);
    if (event->type() == EventType::SKIP) {
      return;
    }
    for (const auto& action : (action_it->second).second) {
      switch (action.first) {
        case ParseOption::kCopy:
          CopyJSONToProtoStrings(json_event, action.second, event);
          break;
        case ParseOption::kMakeFile:
          SetFileFields(json_event, action.second, event);
          break;
      }
    }
  }
}

}  // namespace

File ParseFilename(const string& filename) {
  vector<string> dirs = util::SplitToVector(filename, '/');
  File file;
  // The result of Split will be a nonempty vector, so dirs.back() is defined.
  if (!dirs.back().empty()) {
    file.set_filename(dirs.back());
  }
  dirs.pop_back();
  for (const auto& dir : dirs) {
    file.mutable_directory()->add_path(dir);
  }
  return file;
}

PlasoEvent ParseJSON(const ::Json::Value& json_event) {
  PlasoEvent event;
  // Convert the nanosecond timestamp from the JSON input to a microsecond
  // timstamp used in the graph.
  ::Json::Int64 unix_nanos = json_event[plaso::kTimestampName].asInt64();
  event.set_timestamp(
      static_cast<int64_t>(unix_nanos / static_cast<::Json::Int64>(1000)));
  string filename = GetJSONField(plaso::kSourceFileName, json_event);
  *event.mutable_event_source_file() = ParseFilename(filename);
  SetEventFields(json_event, &event);
  return event;
}

AST ToAST(const File& file) {
  AST file_ast = value::MakeNullTuple(2);
  AST path_type = type::MakeDirectory();
  AST file_type = type::MakeFile();
  AST path_ast = value::MakeEmptyList();
  if (file.has_directory()) {
    for (const string& dir : file.directory().path()) {
      value::Append(path_type, value::MakeString(dir), &path_ast);
    }
  }
  value::SetField(file_type, 0, path_ast, &file_ast);
  AST filename_ast = value::MakePrimitiveNull(PrimitiveType::STRING);
  if (file.has_filename()) {
    filename_ast.mutable_p_ast()->mutable_val()->set_string_val(
        file.filename());
  }
  value::SetField(file_type, 1, filename_ast, &file_ast);
  return file_ast;
}

string ToString(const File& file) {
  string filename;
  if (file.has_directory()) {
    for (const string& dir : file.directory().path()) {
      util::StrAppend(&filename, dir, "/");
    }
  }
  if (file.has_filename()) {
    util::StrAppend(&filename, file.filename());
  }
  return filename;
}

}  // namespace plaso
}  // namespace third_party_logle
