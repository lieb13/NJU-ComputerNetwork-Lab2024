#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.
  (void)dgram;
  (void)next_hop;
  EthernetFrame msg;
  uint32_t hop_address = next_hop.ipv4_numeric();
  if ( IP_Ethernet_map_.find( hop_address ) != IP_Ethernet_map_.end() ) {
    msg.header.type = EthernetHeader::TYPE_IPv4;
    msg.header.dst = IP_Ethernet_map_[hop_address].first;
    msg.header.src = ethernet_address_;
    msg.payload = serialize( dgram );
    transmit( msg );
  } else {
    msg.header.type = EthernetHeader::TYPE_ARP;
    msg.header.dst = ETHERNET_BROADCAST;
    msg.header.src = ethernet_address_;

    ARPMessage request;
    request.opcode = ARPMessage::OPCODE_REQUEST;
    request.sender_ip_address = ip_address_.ipv4_numeric();
    request.sender_ethernet_address = ethernet_address_;
    request.target_ip_address = next_hop.ipv4_numeric();
    msg.payload = serialize( request );

    if ( ethernet_address_waitlist_.find( hop_address ) == ethernet_address_waitlist_.end() ) {
      ethernet_address_waitlist_[hop_address] = {};
    }
    ethernet_address_waitlist_[hop_address].push_back( dgram );

    if ( request_flood_.find( hop_address ) != request_flood_.end() ) {
      if ( timer_ - request_flood_[hop_address] <= 5 * 1000 )
        return;
    }

    transmit( msg );
    request_flood_[hop_address] = timer_;
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  (void)frame;
  // only recieve broadcast or message toward this
  if ( frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_ ) {
    return;
  }
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      datagrams_received_.push( dgram );
    }
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage amsg;
    if ( parse( amsg, frame.payload ) ) {

      // try to send dgrams not send
      uint32_t address = amsg.sender_ip_address;
      if ( ethernet_address_waitlist_.find( address ) != ethernet_address_waitlist_.end() ) {
        for ( const auto& dgram : ethernet_address_waitlist_[address] ) {
          EthernetFrame msg;
          msg.header.type = EthernetHeader::TYPE_IPv4;
          msg.header.dst = amsg.sender_ethernet_address;
          msg.header.src = ethernet_address_;
          msg.payload = serialize( dgram );
          transmit( msg );
        }
        ethernet_address_waitlist_[address].clear();
      }

      // remember IP_Ethernet mapping for 30s
      IP_Ethernet_map_[address] = make_pair( amsg.sender_ethernet_address, timer_ );

      // reply to ARP request
      if ( amsg.opcode == ARPMessage::OPCODE_REQUEST && amsg.target_ip_address == ip_address_.ipv4_numeric() ) {
        EthernetFrame msg;
        msg.header.src = ethernet_address_;
        msg.header.dst = amsg.sender_ethernet_address;
        msg.header.type = EthernetHeader::TYPE_ARP;
        ARPMessage reply;
        reply.opcode = ARPMessage::OPCODE_REPLY;
        reply.sender_ip_address = ip_address_.ipv4_numeric();
        reply.sender_ethernet_address = ethernet_address_;
        reply.target_ip_address = amsg.sender_ip_address;
        reply.target_ethernet_address = amsg.sender_ethernet_address;
        msg.payload = serialize( reply );
        transmit( msg );
      }
    }
  } else {
    return;
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  (void)ms_since_last_tick;
  timer_ += ms_since_last_tick;

  // expire IP-Ethernet mappings
  for ( const auto& mapp : IP_Ethernet_map_ ) {
    size_t time = mapp.second.second;
    if ( timer_ - time > 30 * 1000 ) {
      IP_Ethernet_map_.erase( mapp.first );
    }
  }
}
