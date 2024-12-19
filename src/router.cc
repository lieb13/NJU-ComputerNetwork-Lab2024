#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  _routes.push_back(RouteInfo(route_prefix, prefix_length, next_hop, interface_num));
  // Your code here.
}

inline bool match(uint32_t prefix, uint8_t prefix_length, uint32_t dst) {
  bool ret = (prefix_length == 0) ? true : ((prefix >> (32 - prefix_length)) ^ (dst >> (32 -prefix_length))) == 0; 
  cerr << "MATCH: " << Address::from_ipv4_numeric(prefix).to_string() << "/" << (uint32_t)prefix_length << " with " << 
  Address::from_ipv4_numeric(dst).to_string() << " is " << ret << endl; 
  return ret;

}
// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  for (size_t i = 0; i < _interfaces.size() ; i++) {
    while (!interface(i)->datagrams_received().empty()) {
      InternetDatagram dgram = interface(i)->datagrams_received().front();
      interface(i)->datagrams_received().pop();

      if (dgram.header.ttl <= 1) {
        return;
      } 
      dgram.header.ttl = dgram.header.ttl - (uint8_t)1;
      dgram.header.compute_checksum();

      RouteInfo max_len;
      bool matched = false;
      for (RouteInfo r : _routes) {
        if (match(r.prefix, r.prefix_len, dgram.header.dst) && r.prefix_len >= max_len.prefix_len) {
          max_len = r;
          matched = true;
        }
      }

      if (matched) {
        // dgram.header.ttl--;
        if (max_len.next_hop.has_value())
          interface(max_len.interface_num)->send_datagram(dgram, max_len.next_hop.value());
        else 
          interface(max_len.interface_num)->send_datagram(dgram, Address::from_ipv4_numeric(dgram.header.dst));
      }
    }
  } 
}

