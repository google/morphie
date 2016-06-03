// This file contains constants used in the analysis of Curio streams.
#ifndef LOGLE_CURIO_DEFS_H_
#define LOGLE_CURIO_DEFS_H_

namespace logle {
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
}  // logle

#endif  // LOGLE_CURIO_DEFS_H_
