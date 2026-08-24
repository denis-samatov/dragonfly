// Copyright 2026, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#pragma once

#include <cstdint>

namespace dfly {

// Uses RAII to track the memory delta caused by a section of code, during which the scope object is
// alive, for a fixed object type only.
//
// A scope can have a child scope, ie a subsection whose memory usage is related to a different
// type. This child scope carries a pointer to the parent so there can be a chain of such child
// scopes.
//
// The scope also uses helio hook mechanism to stop tracking memory usage while its fiber is
// suspended.
class MemoryScope {
 public:
  explicit MemoryScope(int obj_type = -1);

  MemoryScope(const MemoryScope&) = delete;
  MemoryScope& operator=(const MemoryScope&) = delete;

  // An escape hatch eg when we want to exclude key size from calculation
  void MarkDeducted(int64_t bytes);

  void Suspend();
  void Resume();

  ~MemoryScope();

 private:
  void Checkpoint(int64_t used_memory);

  int obj_type_;
  int64_t mem_baseline_ = 0;

  // Raw memory movement accumulated by completed running segments of this scope.
  int64_t delta_ = 0;

  // The sum of movements in all child scopes below this scope
  int64_t child_delta_ = 0;
  // The part of the delta that we do not want to count, eg key allocations
  int64_t deductions_ = 0;

  bool suspended_ = false;
  MemoryScope* parent_ = nullptr;
};

void MarkDeductedFromCurrentScope(int64_t bytes);
void SuspendCurrentCmdMemoryScope();
void ResumeCurrentCmdMemoryScope();

template <typename F> auto WithMemTrack(int obj_type, F f) {
  MemoryScope scope(obj_type);
  return f();
}

}  // namespace dfly
