// This file contains definitions of constants used for account access analysis.
#ifndef THIRD_PARTY_LOGLE_ACCOUNT_ACCESS_DEFS_H_
#define THIRD_PARTY_LOGLE_ACCOUNT_ACCESS_DEFS_H_

namespace third_party_logle {
// The 'access' namespace contains definitions of fields used in access-oriented
// analysis of access to email accounts.
namespace access {

// A comma-separated list of fields that must be present in the input.
extern const char kRequiredFields[];
// The strings below are the names of the fields that provide information about
// a single account access.
//
// The account used to perform an access.
extern const char kActor[];
// The manager of the accessor.
extern const char kActorManager[];
// The job title of the accessor (Software Engineer, Content Reviewer, etc.).
extern const char kActorTitle[];
// The number of accesses.
extern const char kNumAccesses[];
// The account that is accessed.
extern const char kUser[];

}  // namespace access
}  // namespace third_party_logle

#endif  // THIRD_PARTY_LOGLE_ACCOUNT_ACCESS_DEFS_H_
