#include "Arduino.h"
#include "kegboard.h"
#include "KegboardPacketUDP.h"

KegboardPacketUDP::KegboardPacketUDP(IPAddress dest_ip, int dest_port) {
  if (dest_ip == IPAddress((uint32_t) 0) && dhcp_connected()) {
    _remote_ip = Ethernet.localIP();
    _remote_ip[3] = 113;
  }
  else {
    _remote_ip = dest_ip;
  }

  _remote_port = dest_port;

  dhcp_connected();
}

int KegboardPacketUDP::dhcp_connected() {
  if (!_dhcp_status) {
    _dhcp_status = Ethernet.begin(KB_ETHERNET_MAC);
  }
  else {
    _dhcp_status = Ethernet.maintain();
    if (_dhcp_status == 0 || _dhcp_status == 2 || _dhcp_status == 4)
      _dhcp_status = 1;
    else
      _dhcp_status = 0;
  }

  return _dhcp_status;
}

void KegboardPacketUDP::Print() {
  int i;
  uint16_t m_crc = GenCrc();

  _udp.beginPacket(_remote_ip, _remote_port);

  _udp.write((byte*) &m_type, 2);
  i = m_len;
  _udp.write((byte*) &i, 2);

  _udp.write(m_payload, i);
  _udp.write((byte*) &crc, 2);
  _udp.write("\r\n");
}
