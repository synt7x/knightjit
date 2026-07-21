#include <cstdint>

class ir {
public:
  using idx = uint32_t;

  enum class opcode : idx {
    PANIC,
    
  };

  struct compact {
    uint32_t flag : 2;
    opcode op : 6;
    idx anchor : 24;
    idx v2 : 16;
    idx v3 : 16;
  };

  struct extended {
    uint32_t flag : 2;
    opcode op : 6;
    idx e_anchor : 32;
  };

  struct raw {
    uint64_t flag : 2;
    uint64_t is_string : 1;
    uint64_t value : 61;
  };

  union instruction {
    compact compact;
    extended extended;
    raw raw;
  };

private:
  static_assert(sizeof(compact) == 8);
  static_assert(sizeof(extended) == 8);
  static_assert(sizeof(raw) == 8);

  static_assert(sizeof(instruction) == 8);
};