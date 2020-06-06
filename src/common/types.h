#ifndef TYPES_H_
#define TYPES_H_
using std::string;

typedef string UUID;
typedef uint32_t VersionNumber;
typedef string ClientId;
typedef string Data;
typedef struct {
  size_t offset;
  size_t nbytes;
} ByteRange;

#endif
