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
#ifndef THIRD_PARTY_LOGLE_UTIL_STATUS_H_
#define THIRD_PARTY_LOGLE_UTIL_STATUS_H_

// This file defines a Status class for representing the success of an
// operation. A function that operates successfully returns an OK status object.
// A function whose operation fails returns a non-OK object, which contains
// details about why the operation failed.
#include "third_party/logle/base/string.h"

namespace tervuren {

// A set of error codes.
enum class Code {
  // No error. Returned on success.
  OK = 0,
  // The error is caused by the client providing invalid data.
  INVALID_ARGUMENT = 1,
  // An invariant of the system has been violated.
  INTERNAL = 2,
  // The error is caused by interaction with an external system. For example, an
  // attempt to open a file may have failed.
  EXTERNAL = 3
};

namespace util {

// A Status object consists of an error code and an error message. This class
// adopts the convention that a status object with code Code::OK has no message
// and also defines a unique OK status object.
class Status {
 public:
  // Creates a status object with code OK and an empty message.
  Status() : code_(Code::OK), message_("") {}

  // Creates a status object with the provided code. Sets the status message to
  // the second argument if the code is not Code::OK and ignores the second
  // argument otherwise.
  Status(Code code, const string& message) : code_(code), message_(message) {
    if (code_ == Code::OK) {
      message_.clear();
    }
  }

  // Copy constructor.
  Status(const Status& other) : code_(other.code_), message_(other.message_) {}

  // Assignment operator.
  Status& operator=(const Status& other) {
    code_ = other.code_;
    message_ = other.message_;
    return *this;
  }

  // A status object with code OK and an empty message.
  static const Status& OK;

  // Returns true if the status code is OK.
  bool ok() const { return code_ == Code::OK; }

  Code code() const { return code_; }
  const string& message() const { return message_; }

 private:
  Code code_;
  string message_;
};

}  // namespace util
}  // namespace tervuren

#endif  // THIRD_PARTY_LOGLE_UTIL_STATUS_H_
