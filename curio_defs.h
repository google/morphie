// This file contains constants used in the analysis of Curio streams.
#ifndef THIRD_PARTY_LOGLE_CURIO_DEFS_H_
#define THIRD_PARTY_LOGLE_CURIO_DEFS_H_

namespace third_party_logle {
// The 'curio' namespace encloses definitions used to analyze Curio streams.
namespace curio {

// The field name for children of a stream.
extern const char kChildrenField[];

// Tags used in graph labels.
extern const char kStreamTag[];
extern const char kStreamIdTag[];
extern const char kStreamNameTag[];
extern const char kDependentTag[];

}  // namespace curio
}  // namespace third_party_logle

#endif  // THIRD_PARTY_LOGLE_CURIO_DEFS_H_
