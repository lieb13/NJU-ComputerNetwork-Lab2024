#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

#define next_absqeno ( reader().bytes_popped() + set_SYN_ + set_FIN_ )

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return seqno_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutive_rt_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  (void)transmit;

  while ( read_size_ > 0 ) {
    TCPSenderMessage msg;
    msg.seqno = Wrap32::wrap( next_absqeno, isn_ );
    if ( !set_SYN_ ) {
      msg.SYN = 1;
      set_SYN_ = true;
      read_size_--;
    }
    uint64_t len = min( read_size_, TCPConfig::MAX_PAYLOAD_SIZE );
    read( input_.reader(), len, msg.payload );
    read_size_ -= msg.payload.length();
    std::cerr << reader().is_finished() << " " << read_size_ << " " << set_FIN_ << endl;
    if ( reader().is_finished() && read_size_ > 0 && !set_FIN_ ) {
      msg.FIN = 1;
      set_FIN_ = true;
      read_size_--;
    }
    msg.RST = input_.has_error();
    if ( msg.sequence_length() == 0 )
      break;

    transmit( msg );
    outstanding_.push( msg );
    seqno_flight_ += msg.sequence_length();
    timer_running_ = true;
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage empty_msg;
  empty_msg.RST = input_.has_error();
  empty_msg.seqno = Wrap32::wrap( next_absqeno, isn_ );
  return empty_msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  (void)msg;
  window_size_ = msg.window_size;
  read_size_ = window_size_;
  if ( window_size_ == 0 )
    read_size_ = 1;

  if ( msg.RST )
    input_.set_error();

  if ( msg.ackno == std::nullopt )
    return;
  uint64_t ackno = ( msg.ackno.value() ).unwrap( isn_, next_absqeno );
  if ( ackno > next_absqeno )
    return;
  read_size_ -= next_absqeno - ackno;

  bool ack = false;
  while ( !outstanding_.empty() ) {
    TCPSenderMessage front = outstanding_.front();
    if ( ackno >= front.seqno.unwrap( isn_, next_absqeno ) + front.sequence_length() ) {
      outstanding_.pop();
      seqno_flight_ -= front.sequence_length();
      ack = true;
    } else
      break;
  }
  if ( ack ) {
    RTO_ = initial_RTO_ms_;
    consecutive_rt_ = 0;
    if ( outstanding_.empty() ) {
      timer_running_ = false;
    } else {
      timer_ = 0;
      timer_running_ = true;
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  (void)ms_since_last_tick;
  (void)transmit;

  if ( RTO_ == 0 )
    RTO_ = initial_RTO_ms_;
  // std::cerr << RTO_ << endl;
  if ( timer_running_ )
    timer_ += ms_since_last_tick;
  if ( timer_ >= RTO_ ) {
    transmit( outstanding_.front() );
    if ( window_size_ > 0 ) {
      consecutive_rt_++;
      RTO_ <<= 1; // Double the RTO
    }
    timer_ = 0;
    timer_running_ = true;
  }
}
