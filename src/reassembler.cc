#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  (void)first_index;
  (void)data;
  (void)is_last_substring;

  buffer_.resize(pushed_ + writer().available_capacity());
  for (int i = first_index; i < min(buffer_.size(), first_index + data.length()); i++) {
    buffer_[i] = data[i - first_index];
    inserted_++;
  }
  int i;
  for (i = pushed_; buffer_[i] != '\0'; i++) {
    pushed_++;
  }
  writer().push(std::string(buffer_.begin() + pushed_, buffer_.begin() + i));
  if (is_last_substring)
    writer().close();
}

uint64_t Reassembler::bytes_pending() const
{
  return inserted_ - pushed_;
}
