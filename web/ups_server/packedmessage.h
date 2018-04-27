
#ifndef PACKEDMESSAGE_H
#define PACKEDMESSAGE_H

#include <string>
#include <cassert>
#include <vector>
#include <cstdio>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>

typedef std::vector<boost::uint8_t> data_buffer;


// A generic function to show contents of a container holding byte data 
// as a string with hex representation for each byte.
//
template<class CharContainer>
std::string show_hex(const CharContainer& c)
{
    std::string hex;
    char buf[16];
    typename CharContainer::const_iterator i;
    for (i = c.begin(); i != c.end(); ++i) {
        std::sprintf(buf, "%02X ", static_cast<unsigned>(*i) & 0xFF);
        hex += buf;
    }
    return hex;
}


// The header size for packed messages
//
const unsigned HEADER_SIZE = 4;


// PackedMessage implements simple "packing" of protocol buffers Messages into
// a string prepended by a header specifying the message length.
// MessageType should be a Message class generated by the protobuf compiler.
//
template <class MessageType>
class PackedMessage 
{
public:
    typedef boost::shared_ptr<MessageType> MessagePointer;

    PackedMessage(MessagePointer msg = MessagePointer())
        : m_msg(msg)
    {}

    void set_msg(MessagePointer msg)
    {
        m_msg = msg;
    }

    MessagePointer get_msg()
    {
        return m_msg;
    }

    // Pack the message into the given data_buffer. The buffer is resized to
    // exactly fit the message.
    // Return false in case of an error, true if successful.
    //
    bool pack(data_buffer& buf) const
    {
        if (!m_msg)
            return false;

        unsigned msg_size = m_msg->ByteSize();
	unsigned header_size;
        encode_header(buf, msg_size, header_size);
        buf.resize(header_size + msg_size);
        return m_msg->SerializeToArray(&buf[header_size], msg_size);
    }

    // Given a buffer with the first HEADER_SIZE bytes representing the header,
    // decode the header and return the message length. Return 0 in case of 
    // an error.
    //
    unsigned int get_message_size(const data_buffer& buf) const{
      size_t s =0;
      int res=0;
      for(size_t i=0;i<HEADER_SIZE;++i){
	if(buf.at(i)<0x80){
	  return res | int(buf.at(i))<<s;
	}
	res |= int(buf.at(i)&0x7f) << s;
	s+=7;
      }
      return 0;
    }
    int get_header_size(const data_buffer& buf) const{
      for(size_t i=0;i<HEADER_SIZE;++i){
	if(buf.at(i)<0x80){
	  return i+1;
	}
      }
      return -1;
    }
    unsigned decode_header(const data_buffer& buf,int& headersize) const
    {
        if (buf.size() < HEADER_SIZE)
            return 0;
        unsigned msg_size = get_message_size(buf);
	headersize = get_header_size(buf);
        return msg_size;
    }

    // Unpack and store a message from the given packed buffer.
    // Return true if unpacking successful, false otherwise.
    //
    bool unpack(const data_buffer& buf)
    {
      int headersize = get_header_size(buf);
        return m_msg->ParseFromArray(&buf[headersize], buf.size() - headersize);
    }
private:
    // Encodes the side into a header at the beginning of buf
    //
    void encode_header(data_buffer& buf, unsigned size, unsigned& header_size) const
    {
      header_size = 0;
      int temp =0;
      while(size > 0x01111111) {
	header_size++;
	temp = size % 7 + 0x10000000;
	buf.insert(buf.begin(), temp);
	size  = size >> 7;
      }
      header_size++;
      buf.insert(buf.begin(), size);
    }

    MessagePointer m_msg;
};

#endif /* PACKEDMESSAGE_H */

