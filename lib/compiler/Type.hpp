#pragma once

namespace cfg {
  struct Value;
  
  // Placeholder type. For now, everything is an F32
  struct Type {
    inline bool operator ==(Type const &rhs) const {
      return true;
    }
    inline bool operator !=(Type const &rhs) const {
      return !(*this == rhs);
    }
  };
}
