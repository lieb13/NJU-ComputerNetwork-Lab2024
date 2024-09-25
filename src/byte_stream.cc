#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  // Your code here.
  // (void)data;
  int len = min(data.length(), capacity_ - pushed_ + popped_);
  pushed_ += len;
  buffer_.insert(buffer_.end(), data.begin(), data.begin() + len);
  return;
}

void Writer::close()
{
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - pushed_ + popped_;
}

uint64_t Writer::bytes_pushed() const
{
  return pushed_;
}

bool Reader::is_finished() const
{
  return closed_ && (pushed_ == popped_);
}

uint64_t Reader::bytes_popped() const
{
  return popped_;
}

string_view Reader::peek() const
{
  return std::string_view(buffer_.data() + popped_, pushed_ - popped_);
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  // (void)len;
  popped_ += min(len, pushed_ - popped_);
}

uint64_t Reader::bytes_buffered() const
{
  return pushed_ - popped_;
}
