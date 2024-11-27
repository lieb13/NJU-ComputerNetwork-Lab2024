#include "wrapping_integers.hh"

using namespace std;

const uint64_t seg32 = 1UL << 32;
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  (void)n;
  (void)zero_point;
  return Wrap32 { (uint32_t)n + zero_point.raw_value_ };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  (void)zero_point;
  (void)checkpoint;
  uint64_t raw = (uint64_t)raw_value_, zero = (uint64_t)zero_point.raw_value_;
  uint64_t offset = raw < zero ? raw + seg32 - zero : raw - zero;
  if ( offset >= checkpoint )
    return offset;
  offset += checkpoint / seg32 * seg32;
  if ( offset < checkpoint )
    offset += seg32;
  uint64_t ret = offset - checkpoint < checkpoint + seg32 - offset ? offset : offset - seg32;
  return ret;
}
