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

#include "analyzers/plaso/plaso_event.h"

#include <cstdint>
#include <map>

#include "analyzers/plaso/plaso_defs.h"
#include "base/string.h"
#include "graph/type.h"
#include "graph/type_checker.h"
#include "gtest.h"
#include "third_party/logle/analyzers/plaso/plaso_event.pb.h"
#include "util/time_utils.h"

namespace morphie {
namespace {

static const char kJSONstring[] =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "File Hosted Date",
  "data_type" : "windows:registry:key_value",
  "message" : "IDX Version: 100 Host IP address: [127.0.0.1]",
  "display_name" : "C:/Documents and Settings/user/Application Data/binks.jar"
})";

static Json::Reader reader;
static Json::Value json_doc;

static void DeleteField(const string& field, Json::Value* json_doc) {
  if (json_doc->isMember(field)) {
    json_doc->removeMember(field);
  }
}

TEST(PlasoEventDeathTest, RequiresNonEmptyJSONInput) {
  PlasoEvent e;
  reader.parse("", json_doc);
  EXPECT_DEATH({ e = plaso::ParseJSON(json_doc); }, ".*requires an object.*");
  reader.parse("{}", json_doc);
  EXPECT_DEATH({ e = plaso::ParseJSON(json_doc); }, "No field named.*");
  reader.parse("[]", json_doc);
  EXPECT_DEATH({ e = plaso::ParseJSON(json_doc); }, ".*requires an object.*");
}

TEST(PlasoEventDeathTest, JSONHasRequiredFields) {
  PlasoEvent e;
  reader.parse(kJSONstring, json_doc);
  DeleteField(plaso::kTimestampName, &json_doc);
  EXPECT_DEATH({ e = plaso::ParseJSON(json_doc); }, "No field named.*");
  reader.parse(kJSONstring, json_doc);
  DeleteField(plaso::kDescriptionName, &json_doc);
  EXPECT_DEATH({ e = plaso::ParseJSON(json_doc); }, "No field named.*");
  reader.parse(kJSONstring, json_doc);
  DeleteField(plaso::kDataTypeName, &json_doc);
  EXPECT_DEATH({ e = plaso::ParseJSON(json_doc); }, "No field named.*");
  reader.parse(kJSONstring, json_doc);
  DeleteField(plaso::kSourceFileName, &json_doc);
  EXPECT_DEATH({ e = plaso::ParseJSON(json_doc); }, "No field named.*");
}

class PlasoEventTest : public ::testing::Test {
 protected:
  PlasoEvent event_;
};

// Returns an RFC3339 representation of a Unix microsecond timestamp.
static string GetTimeString(int64_t unix_micros) {
  return util::UnixMicrosToRFC3339(unix_micros);
}

// Test parsing of filenames into the File proto. The test includes the examples
// in the documentation of ParseFilename.
TEST_F(PlasoEventTest, ParsesFilenames) {
  File file;
  // The empty string does not produce file information.
  file = plaso::ParseFilename("");
  EXPECT_FALSE(file.has_directory());
  EXPECT_FALSE(file.has_filename());
  // The root directory is "/".
  file = plaso::ParseFilename("/");
  EXPECT_EQ(1, file.directory().path_size());
  EXPECT_EQ("", file.directory().path(0));
  EXPECT_FALSE(file.has_filename());
  // When provided only a filename, there is no directory.
  file = plaso::ParseFilename("filename.txt");
  EXPECT_FALSE(file.has_directory());
  EXPECT_TRUE(file.has_filename());
  EXPECT_EQ("filename.txt", file.filename());
  // A '/' terminated path is parsed with no filename.
  file = plaso::ParseFilename("/usr/local/");
  EXPECT_EQ(3, file.directory().path_size());
  EXPECT_EQ("", file.directory().path(0));
  EXPECT_EQ("usr", file.directory().path(1));
  EXPECT_EQ("local", file.directory().path(2));
  EXPECT_FALSE(file.has_filename());
  // A non-empty filename with a path.
  file = plaso::ParseFilename("/foo/bar/baz.f");
  EXPECT_EQ(3, file.directory().path_size());
  EXPECT_EQ("", file.directory().path(0));
  EXPECT_EQ("foo", file.directory().path(1));
  EXPECT_EQ("bar", file.directory().path(2));
  EXPECT_TRUE(file.has_filename());
  EXPECT_EQ("baz.f", file.filename());
}

TEST_F(PlasoEventTest, ParseJSONExtractsTimestamp) {
  PlasoEvent e;
  reader.parse(kJSONstring, json_doc);
  e = plaso::ParseJSON(json_doc);
  EXPECT_EQ(1333412795000000, e.timestamp());
}

TEST_F(PlasoEventTest, ChromeFileDownloadProcessing) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Chrome History",
  "data_type" : "chrome:history:file_downloaded",
  "message" : "This is an inaccurate message",
  "url" : "http://source.of.file.org/filename.txt",
  "full_path" : "/target/location/of/filename.txt",
  "display_name" : "/some/chrome/history/file"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::FILE_DOWNLOADED, event_.type());
  EXPECT_TRUE(event_.has_source_url());
  EXPECT_EQ(json_doc["url"].asString(), event_.source_url());
  EXPECT_TRUE(event_.has_target_file());
  EXPECT_EQ(json_doc["full_path"].asString(),
            plaso::ToString(event_.target_file()));
}

TEST_F(PlasoEventTest, ChromePageVisitedProcessing) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Chrome History",
  "data_type" : "chrome:history:page_visited",
  "message" : "This is an inaccurate message",
  "url" : "http://page.that.visited.was/today",
  "host" : "page.that.visited.was",
  "title" : "I am a test page.",
  "host" : "page.that.visited.was",
  "from_visit" : "https://secure.page.exited/previously",
  "visit_source" : "I am a secure page.",
  "display_name" : "/some/chrome/history/file"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::PAGE_VISITED, event_.type());
  EXPECT_TRUE(event_.has_source_url());
  EXPECT_EQ(json_doc["from_visit"].asString(), event_.source_url());
  EXPECT_TRUE(event_.has_target_url());
  EXPECT_EQ(json_doc["url"].asString(), event_.target_url());
}

TEST_F(PlasoEventTest, ChromeCacheEntryProcessing) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Chrome Cache",
  "data_type" : "chrome:cache:entry",
  "message" : "This is an inaccurate message",
  "original_url" : "http://page.that.visited.was/today",
  "display_name" : "/some/chrome/cache/file"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::PAGE_VISITED, event_.type());
  EXPECT_TRUE(event_.has_source_url());
  EXPECT_EQ(json_doc["original_url"].asString(), event_.source_url());
}

TEST_F(PlasoEventTest, ChromeCookieProcessing) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Chrome Cookies",
  "data_type" : "chrome:cookie:entry",
  "message" : "This is an inaccurate message",
  "url" : "http://page.that.visited.was/today",
  "display_name" : "/some/chrome/cache/file"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::PAGE_VISITED, event_.type());
  EXPECT_TRUE(event_.has_source_url());
  EXPECT_EQ(json_doc["url"].asString(), event_.source_url());
}

TEST_F(PlasoEventTest, ChromeExtensionActivityProcessing) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Chrome Extension Activity",
  "data_type" : "chrome:extension_activity:activity_log",
  "message" : "This is an inaccurate message",
  "page_url" : "http://page.that.visited.was/today",
  "extension_id" : "ExtensionID[inaccurate]",
  "display_name" : "/some/chrome/cache/file"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::PAGE_VISITED, event_.type());
  EXPECT_TRUE(event_.has_source_url());
  EXPECT_EQ(json_doc["page_url"].asString(), event_.source_url());
  EXPECT_TRUE(event_.has_extension_id());
  EXPECT_EQ(json_doc["extension_id"].asString(), event_.extension_id());
}

TEST_F(PlasoEventTest, ChromeExtensionInstallationProcessing) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Chrome Extension Activity",
  "data_type" : "chrome:preferences:extension_installation",
  "message" : "This is an inaccurate message",
  "extension_name" : "Random extension name",
  "extension_id" : "RandomExtID[inaccurate]",
  "display_name" : "/some/chrome/cache/file"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::BROWSER_EXTENSION_INSTALLED, event_.type());
  EXPECT_TRUE(event_.has_extension_name());
  EXPECT_EQ(json_doc["extension_name"].asString(), event_.extension_name());
  EXPECT_TRUE(event_.has_extension_id());
  EXPECT_EQ(json_doc["extension_id"].asString(), event_.extension_id());
}

TEST_F(PlasoEventTest, FirefoxDownloadProcessing) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Chrome History",
  "data_type" : "firefox:downloads:download",
  "message" : "This is an inaccurate message",
  "url" : "http://page.that.visited.was/today",
  "full_path" : "/target/location/of/filename.txt",
  "display_name" : "/some/chrome/history/file"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::FILE_DOWNLOADED, event_.type());
  EXPECT_TRUE(event_.has_source_url());
  EXPECT_EQ(json_doc["url"].asString(), event_.source_url());
  EXPECT_TRUE(event_.has_target_file());
  EXPECT_EQ(json_doc["full_path"].asString(),
            plaso::ToString(event_.target_file()));
}

// Currently, no events are generated from bookmark data.
TEST_F(PlasoEventTest, FirefoxBookmarkProcessing) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Firefox History",
  "data_type" : "firefox:places:bookmark",
  "message" : "This is an inaccurate message",
  "display_name" : "/some/chrome/history/file"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::SKIP, event_.type());
  json_doc["data_type"] = "firefox:places:bookmark_annotation";
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::SKIP, event_.type());
  json_doc["data_type"] = "firefox:places:bookmark_folder";
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::SKIP, event_.type());
}

TEST_F(PlasoEventTest, FirefoxPageVisitedProcessing) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Firefox History",
  "data_type" : "firefox:places:page_visited",
  "message" : "This is an inaccurate message",
  "url" : "http://page.that.visited.was/today",
  "display_name" : "/some/chrome/history/file"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::PAGE_VISITED, event_.type());
  EXPECT_TRUE(event_.has_source_url());
  EXPECT_EQ(json_doc["url"].asString(), event_.source_url());
}

TEST_F(PlasoEventTest, MacApplicationUsage) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Application Usage",
  "data_type" : "macosx:application_usage",
  "application" : "Chrome Browser",
  "app_version" : "Most RecentVersion",
  "display_name" : "/some/osx/log/file"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::APPLICATION_EXECUTED, event_.type());
  EXPECT_TRUE(event_.has_application_name());
  EXPECT_EQ(json_doc["application"].asString(), event_.application_name());
}

TEST_F(PlasoEventTest, TaskSchedulerTaskCache) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Application Usage",
  "data_type" : "task_scheduler:task_cache:entry",
  "task_name" : "Chrome Browser",
  "display_name" : "/some/osx/log/file"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::APPLICATION_EXECUTED, event_.type());
  EXPECT_TRUE(event_.has_application_name());
  EXPECT_EQ(json_doc["task_name"].asString(), event_.application_name());
}

TEST_F(PlasoEventTest, WindowsEvtRecord) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Executable Information",
  "data_type" : "windows:evtx:record",
  "event_identifier" : "bad_event",
  "source_name" : "/i/am/a/bad.exe",
  "display_name" : "/location/of/evtx/record"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::APPLICATION_EXECUTED, event_.type());
  EXPECT_TRUE(event_.has_event_id());
  EXPECT_EQ(json_doc["event_identifier"].asString(), event_.event_id());
  EXPECT_TRUE(event_.has_target_file());
  EXPECT_EQ(json_doc["source_name"].asString(),
            plaso::ToString(event_.target_file()));
}

TEST_F(PlasoEventTest, WindowsEvtxRecord) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Executable Information",
  "data_type" : "windows:evtx:record",
  "event_identifier" : "bad_event",
  "source_name" : "/i/am/a/bad.exe",
  "display_name" : "/location/of/evtx/record"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::APPLICATION_EXECUTED, event_.type());
  EXPECT_TRUE(event_.has_event_id());
  EXPECT_EQ(json_doc["event_identifier"].asString(), event_.event_id());
  EXPECT_TRUE(event_.has_target_file());
  EXPECT_EQ(json_doc["source_name"].asString(),
            plaso::ToString(event_.target_file()));
}

TEST_F(PlasoEventTest, WindowsAppCompatCacheProcessing) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Executable Information",
  "data_type" : "windows:registry:appcompatcache",
  "path" : "/HKLM/SYSTEM/CurrentControlSet/Control/Session/Manager/)"
R"(AppCompatibility/AppCompatCache",
  "display_name" : "/HKLM/SYSTEM/CurrentControlSet/Control/Session/Manager/)"
R"(AppCompatibility/AppCompatCache" })";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::APPLICATION_EXECUTED, event_.type());
  EXPECT_TRUE(event_.has_target_file());
  EXPECT_EQ(json_doc["path"].asString(), plaso::ToString(event_.target_file()));
}

TEST_F(PlasoEventTest, WindowsPrefetchExecution) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Executable Information",
  "data_type" : "windows:prefetch:execution",
  "executable" : "name/of/executable.exe",
  "display_name" : "location/of/executable.exe"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::APPLICATION_EXECUTED, event_.type());
  EXPECT_TRUE(event_.has_target_file());
  EXPECT_EQ(json_doc["executable"].asString(),
            plaso::ToString(event_.target_file()));
}
TEST_F(PlasoEventTest, WindowsShellItem) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Executable Information",
  "data_type" : "windows:shell_item:file_entry",
  "name" : "name/of/executable.exe",
  "display_name" : "HKEY_USERS\person\Software\Microsoft\\Shell\"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::APPLICATION_EXECUTED, event_.type());
  EXPECT_TRUE(event_.has_target_file());
  EXPECT_EQ(json_doc["name"].asString(), plaso::ToString(event_.target_file()));
}

TEST_F(PlasoEventTest, WindowsTasksJob) {
  string json_str =
R"({
  "timestamp" : 1333412795000000,
  "timestamp_desc" : "Executable Information",
  "data_type" : "windows:tasks:job",
  "application" : "name/of/executable.exe",
  "display_name" : "location/of/application"
})";
  reader.parse(json_str, json_doc);
  event_ = plaso::ParseJSON(json_doc);
  EXPECT_EQ(EventType::APPLICATION_EXECUTED, event_.type());
  EXPECT_TRUE(event_.has_application_name());
  EXPECT_EQ(json_doc["application"].asString(), event_.application_name());
}

namespace type = ast::type;

TEST_F(PlasoEventTest, FileMessageToAST) {
  File file;
  AST file_ast = plaso::ToAST(file);
  string err;
  AST file_type = type::MakeFile();
  EXPECT_TRUE(type::IsTyped(file_type, file_ast, &err)) << err;
  file.set_filename("foo.txt");
  file_ast = plaso::ToAST(file);
  EXPECT_TRUE(type::IsTyped(file_type, file_ast, &err)) << err;
  EXPECT_EQ("foo.txt", file_ast.c_ast().arg(1).p_ast().val().string_val());
  file.mutable_directory()->add_path("/");
  file.mutable_directory()->add_path("usr");
  file_ast = plaso::ToAST(file);
  EXPECT_TRUE(type::IsTyped(file_type, file_ast, &err)) << err;
  EXPECT_EQ("/",
            file_ast.c_ast().arg(0).c_ast().arg(0).p_ast().val().string_val());
  EXPECT_EQ("usr",
            file_ast.c_ast().arg(0).c_ast().arg(1).p_ast().val().string_val());
}

}  // namespace
}  // namespace morphie
