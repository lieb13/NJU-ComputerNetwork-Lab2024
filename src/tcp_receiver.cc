#include "tcp_receiver.hh"
#include <iostream>
using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if ( message.RST ) {
    reader().set_error();
  }
  if ( !KNOW_ISN_ ) {
    if ( message.SYN ) {
      ISN_ = message.seqno;
      KNOW_ISN_ = true;
    } else
      return;
  }

  uint64_t first_index = message.seqno.unwrap( ISN_, writer().bytes_pushed() + 1 ) - ( !message.SYN );
  reassembler_.insert( first_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage message;
  if ( !KNOW_ISN_ ) {
    message.ackno = std::nullopt;
  } else {
    message.ackno = Wrap32::wrap( writer().bytes_pushed() + 1 + writer().is_closed(), Wrap32( ISN_ ) );
  }
  message.window_size = min( 65535UL, writer().available_capacity() );
  message.RST = writer().has_error();
  return message;
}
