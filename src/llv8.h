#ifndef SRC_LLV8_H_
#define SRC_LLV8_H_

#include <string>

#include <lldb/API/LLDB.h>

namespace llnode {
namespace v8 {

class LLV8;

class Error {
 public:
  Error() : failed_(false), msg_(nullptr) {}
  Error(bool failed, const char* msg) : failed_(failed), msg_(msg) {}

  static inline Error Ok() { return Error(false, "ok"); }
  static inline Error Failure(const char* msg) { return Error(true, msg); }

  inline bool Success() const { return !Fail(); }
  inline bool Fail() const { return failed_; }

 private:
  bool failed_;
  const char* msg_;
};

#define V8_VALUE_DEFAULT_METHODS(NAME, PARENT)                                \
  NAME(const NAME& v) = default;                                              \
  NAME() : PARENT() {}                                                        \
  NAME(LLV8* v8, int64_t raw) : PARENT(v8, raw) {}                            \
  NAME(Value& v) : PARENT(v) {}                                               \
  NAME(Value* v) : PARENT(v->v8(), v->raw()) {}

class Value {
 public:
  Value(const Value& v) = default;
  Value(Value& v) = default;
  Value() : v8_(nullptr), raw_(-1) {}
  Value(LLV8* v8, int64_t raw) : v8_(v8), raw_(raw) {}

  inline bool Check() const { return true; }

  inline int64_t raw() const { return raw_; }
  inline LLV8* v8() const { return v8_; }

  std::string ToString(Error& err);

 protected:
  LLV8* v8_;
  int64_t raw_;
};

class Smi : public Value {
 public:
  V8_VALUE_DEFAULT_METHODS(Smi, Value)

  inline bool Check() const;
  inline int64_t GetValue() const;

  std::string ToString(Error& err);
};

class HeapObject : public Value {
 public:
  V8_VALUE_DEFAULT_METHODS(HeapObject, Value)

  inline bool Check() const;
  inline int64_t LeaField(int64_t off) const;
  inline int64_t LoadField(int64_t off, Error& err);

  template <class T>
  inline T LoadFieldValue(int64_t off, Error& err);

  inline HeapObject GetMap(Error& err);
  inline int64_t GetType(Error& err);
};

class Map : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(Map, HeapObject)

  int64_t GetType(Error& err);
};

class String : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(String, HeapObject)

  inline int64_t Encoding(Error& err);
  inline int64_t Representation(Error& err);
  inline Smi Length(Error& err);

  std::string GetValue(Error& err);
};

class Script : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(Script, HeapObject)

  inline String Name(Error& err);
  inline Smi LineOffset(Error& err);
  inline HeapObject Source(Error& err);
  inline HeapObject LineEnds(Error& err);

  int64_t GetLineFromPos(int64_t pos, Error& err);
  int64_t ComputeLineFromPos(int64_t pos, Error& err);
};

class SharedFunctionInfo : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(SharedFunctionInfo, HeapObject)

  inline String Name(Error& err);
  inline String InferredName(Error& err);
  inline Script GetScript(Error& err);
  inline int64_t StartPosition(Error& err);

  std::string GetPostfix(Error& err);
};

class JSFunction : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(JSFunction, HeapObject)

  inline SharedFunctionInfo Info(Error& err);

  std::string GetDebugLine(Error& err);
};

class OneByteString : public String {
 public:
  V8_VALUE_DEFAULT_METHODS(OneByteString, String)

  inline std::string GetValue(Error& err);
};

class TwoByteString : public String {
 public:
  V8_VALUE_DEFAULT_METHODS(TwoByteString, String)

  inline std::string GetValue(Error& err);
};

class ConsString : public String {
 public:
  V8_VALUE_DEFAULT_METHODS(ConsString, String)

  inline String First(Error& err);
  inline String Second(Error& err);

  inline std::string GetValue(Error& err);
};

class SlicedString : public String {
 public:
  V8_VALUE_DEFAULT_METHODS(SlicedString, String)

  inline String Parent(Error& err);
  inline Smi Offset(Error& err);

  inline std::string GetValue(Error& err);
};

class FixedArrayBase : public HeapObject {
 public:
  V8_VALUE_DEFAULT_METHODS(FixedArrayBase, HeapObject);

  inline Smi Length(Error& err);
};

class FixedArray : public FixedArrayBase {
 public:
  V8_VALUE_DEFAULT_METHODS(FixedArray, FixedArrayBase)

  template <class T>
  inline T Get(int index, Error& err);

  inline int64_t LeaData() const;
};

class LLV8 {
 public:
  LLV8(lldb::SBTarget target);

  std::string GetJSFrameName(Value frame, Error& err);
  std::string GetSharedInfoPostfix(SharedFunctionInfo info, Error& err);

 private:
  template <class T>
  inline T LoadValue(int64_t addr, Error& err);

  int64_t LoadConstant(const char* name);
  int64_t LoadPtr(int64_t addr, Error& err);
  std::string LoadString(int64_t addr, int64_t length, Error& err);
  std::string LoadTwoByteString(int64_t addr, int64_t length, Error& err);
  uint8_t* LoadChunk(int64_t addr, int64_t length, Error& err);

  lldb::SBTarget target_;
  lldb::SBProcess process_;

  int64_t kPointerSize;

  struct {
    int64_t kTag;
    int64_t kTagMask;
    int64_t kShiftSize;
  } smi_;

  struct {
    int64_t kTag;
    int64_t kTagMask;

    int64_t kMapOffset;
  } heap_obj_;

  struct {
    int64_t kInstanceAttrsOffset;
  } map_;

  struct {
    int64_t kSharedInfoOffset;
  } js_function_;

  struct {
    int64_t kNameOffset;
    int64_t kInferredNameOffset;
    int64_t kScriptOffset;
    int64_t kStartPositionOffset;

    int64_t kStartPositionShift;
  } shared_info_;

  struct {
    int64_t kNameOffset;
    int64_t kLineOffsetOffset;
    int64_t kSourceOffset;
    int64_t kLineEndsOffset;
  } script_;

  struct {
    int64_t kEncodingMask;
    int64_t kRepresentationMask;

    // Encoding
    int64_t kOneByteStringTag;
    int64_t kTwoByteStringTag;

    // Representation
    int64_t kSeqStringTag;
    int64_t kConsStringTag;
    int64_t kSlicedStringTag;
    int64_t kExternalStringTag;

    int64_t kLengthOffset;
  } string_;

  struct {
    int64_t kCharsOffset;
  } one_byte_string_;

  struct {
    int64_t kCharsOffset;
  } two_byte_string_;

  struct {
    int64_t kFirstOffset;
    int64_t kSecondOffset;
  } cons_string_;

  struct {
    int64_t kParentOffset;
    int64_t kOffsetOffset;
  } sliced_string_;

  struct {
    int64_t kLengthOffset;
  } fixed_array_base_;

  struct {
    int64_t kDataOffset;
  } fixed_array_;

  struct {
    int64_t kContextOffset;
    int64_t kFunctionOffset;
    int64_t kArgsOffset;
    int64_t kMarkerOffset;

    int64_t kAdaptorFrame;
    int64_t kEntryFrame;
    int64_t kEntryConstructFrame;
    int64_t kExitFrame;
    int64_t kInternalFrame;
    int64_t kConstructFrame;
    int64_t kJSFrame;
    int64_t kOptimizedFrame;
  } frame_;

  struct {
    int64_t kFirstNonstringType;

    int64_t kCodeType;
    int64_t kJSFunctionType;
    int64_t kFixedArrayType;
  } types_;

  friend class Value;
  friend class Smi;
  friend class HeapObject;
  friend class Map;
  friend class String;
  friend class Script;
  friend class SharedFunctionInfo;
  friend class JSFunction;
  friend class OneByteString;
  friend class TwoByteString;
  friend class ConsString;
  friend class SlicedString;
  friend class FixedArrayBase;
  friend class FixedArray;
};

#undef V8_VALUE_DEFAULT_METHODS

}  // namespace v8
}  // namespace llnode

#endif  // SRC_LLV8_H_
