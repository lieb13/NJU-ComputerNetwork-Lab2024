#include "reassembler.hh"

using namespace std;

#include <iostream>

int cnt;
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  // (void)first_index;
  // (void)data;
  // (void)is_last_substring;
  if (writer().is_closed() || first_index + data.length() < pushed_) return;
  buffer_.resize( pushed_ + writer().available_capacity() );
  buffer_used_.resize( pushed_ + writer().available_capacity() );
  if (first_index >= buffer_.size()) return;

    cerr << first_index << " " << data.length() << " " << buffer_.size() << endl;
  if (is_last_substring) {
    if (first_index + data.length() <= buffer_.size()) 
      last_byte_ = first_index + data.length()-1;
  }
  uint64_t i;
  for ( i = first_index; i < min( buffer_.size(), first_index + data.length() ); i++ ) {
    if (buffer_used_[i] == false) inserted_++;
    buffer_[i] = data[i - first_index];
    buffer_used_[i] = true;
  }
  // if (cnt == 6) return;
  // cerr << "fi = "<< first_index << " data = " << data << "size = " << buffer_.size() << "\n---------------checkpoint-----------------\n"; 
  i = pushed_;
  while (i < buffer_.size() && buffer_used_[i]) {
    i++;
    if ((int64_t)i-1 == last_byte_) break;
  }
  writer().push( std::string( buffer_.begin() + pushed_, buffer_.begin() + i ) );
  pushed_ = i;
  if ((int64_t)i-1 == last_byte_) {
    writer().close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return inserted_ - pushed_;
}
