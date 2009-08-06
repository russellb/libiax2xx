/*
 * Copyright (C) 2006, Russell Bryant <russell@russellbryant.net> 
 *
 * This file is part of LibIAX2xx.
 *
 * LibIAX2xx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LibIAX2xx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LibIAX2xx; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*!
 * \file
 * \author Russell Bryant <russell@russellbryant.net>
 *
 * \brief IAX2 frame
 */

#ifndef IAX2_FRAME_H
#define IAX2_FRAME_H

#include <stdlib.h>
#include <sys/types.h>

#include <list>

/*! The ways of sending an IAX2 frame */
enum iax2_frame_shell {
	/*! Undefined */
	IAX2_FRAME_UNDEFINED,
	/*! Full Frame */
	IAX2_FRAME_FULL,
	/*! Mini Frame */
	IAX2_FRAME_MINI,
	/*! Meta Frame */
	IAX2_FRAME_META,
};

/*! Types of meta frames */
enum iax2_meta_type {
	/*! Undefined */
	IAX2_META_UNDEFINED,
	/*! Video */
	IAX2_META_VIDEO,
};

/*!
 * \brief Types for IAX2 full frames
 *
 * \note These values are defined by the IAX2 specification and MUST
 *       not be changed.
 */
enum iax2_frame_type {
	/*! Undefined or unknown */
	IAX2_FRAME_TYPE_UNDEFINED  = 0x00,
	/*! 
	 * \brief Mark the end of a DTMF digit 
	 * Subclass value: the digit, in ASCII
	 */
	IAX2_FRAME_TYPE_DTMF_END   = 0x01,
	/*!
	 * \brief A voice frame
	 * Normally, voice is sent in mini frames.
	 */
	IAX2_FRAME_TYPE_VOICE      = 0x02,
	/*!
	 * \brief A video frame
	 * Normally, video is sent in meta frames.
	 */
	IAX2_FRAME_TYPE_VIDEO      = 0x03,
	/*! A control frame */
	IAX2_FRAME_TYPE_CONTROL    = 0x04,
	/*! A null frame */
	IAX2_FRAME_TYPE_NULL       = 0x05,
	/*!
	 * \brief An IAX2 signalling frame
	 * Subclass value: \ref iax2_subclass
	 */
	IAX2_FRAME_TYPE_IAX2       = 0x06,
	/*! A text frame */
	IAX2_FRAME_TYPE_TEXT       = 0x07,
	/*! An image frame */
	IAX2_FRAME_TYPE_IMAGE      = 0x08,
	/*! An HTML frame */
	IAX2_FRAME_TYPE_HTML       = 0x09,
	/*! A CNG frame (Comfort Noise Generation) */
	IAX2_FRAME_TYPE_CNG        = 0x0A,
	/*! A modem over IP data frame */
	IAX2_FRAME_TYPE_MODEM      = 0x0B,
	/*!
	 * \brief Mark the beginning of a DTMF digit
	 * Subclass value: the digit, in ASCII
	 */
	IAX2_FRAME_TYPE_DTMF_BEGIN = 0x0C,
};

/*! 
 * \brief Subclass for frames of type IAX2 
 *
 * \note These values are defined by the IAX2 specification and MUST
 *       not be changed.
 */
enum iax2_subclass {
	/*! Initiate a new call */
	IAX2_SUBCLASS_NEW       = 0x01,
	/*! Ping request */
	IAX2_SUBCLASS_PING      = 0x02,
	/*! Ping or poke reply */
	IAX2_SUBCLASS_PONG      = 0x03,
	/*! Explicit acknowledgment */
	IAX2_SUBCLASS_ACK       = 0x04,
	/*! Initiate call teardown */
	IAX2_SUBCLASS_HANGUP    = 0x05,
	/*! Reject a call */
	IAX2_SUBCLASS_REJECT    = 0x06,
	/*! Accept a call */
	IAX2_SUBCLASS_ACCEPT    = 0x07,
	/*! Authentication request */
	IAX2_SUBCLASS_AUTHREQ   = 0x08,
	/*! Authentication reply */
	IAX2_SUBCLASS_AUTHREP   = 0x09,
	/*! Invalid message response */
	IAX2_SUBCLASS_INVAL     = 0x0A,
	/*! Lag request */
	IAX2_SUBCLASS_LAGRQ     = 0x0B,
	/*! Lag reply */
	IAX2_SUBCLASS_LAGRP     = 0x0C,
	/*! Registration request */
	IAX2_SUBCLASS_REGREQ    = 0x0D,
	/*! Registration authentication request */
	IAX2_SUBCLASS_REGAUTH   = 0x0E,
	/*! Registration acknowledgment */
	IAX2_SUBCLASS_REGACK    = 0x0F,
	/*! Registration reject */
	IAX2_SUBCLASS_REGREJ    = 0x10,
	/*! Registration release */
	IAX2_SUBCLASS_REGREL    = 0x11,
	/*! Voice/Video retransmit request */
	IAX2_SUBCLASS_VNAK      = 0x12,
	/*! Dialplan request */
	IAX2_SUBCLASS_DPREQ     = 0x13,
	/*! Dialplan reply */
	IAX2_SUBCLASS_DPREP     = 0x14,
	/*! Dial */
	IAX2_SUBCLASS_DIAL      = 0x15,
	/*! Transfer request */
	IAX2_SUBCLASS_TXREQ     = 0x16,
	/*! Transfer connect */
	IAX2_SUBCLASS_TXCNT     = 0x17,
	/*! Transfer accept */
	IAX2_SUBCLASS_TXACC     = 0x18,
	/*! Transfer ready */
	IAX2_SUBCLASS_TXREADY   = 0x19,
	/*! Transfer release */
	IAX2_SUBCLASS_TXREL     = 0x1A,
	/*! Transfer reject */
	IAX2_SUBCLASS_TXREJ     = 0x1B,
	/*! Halt audio/video [media] transmission */
	IAX2_SUBCLASS_QUELCH    = 0x1C,
	/*! Resume audio/video [media] transmission */
	IAX2_SUBCLASS_UNQUELCH  = 0x1D,
	/*! Poke request */
	IAX2_SUBCLASS_POKE      = 0x1E,
	/* Noted as Reserved in the RFC draft -- 0x1F */
	/*! Message waiting indication */
	IAX2_SUBCLASS_MWI       = 0x20,
	/*! Unsupported message */
	IAX2_SUBCLASS_UNSUPPORT = 0x21,
	/*! Remote transfer request */
	IAX2_SUBCLASS_TRANSFER  = 0x22,
	/*! Provision an IAX2 device */
	IAX2_SUBCLASS_PROVISION = 0x23,
	/*! Download firmware request */
	IAX2_SUBCLASS_FWDOWNL   = 0x24,
	/*! Transmit firmware data */
	IAX2_SUBCLASS_FWDATA    = 0x25,
};

/*!
 * \brief Types of IAX2 Information Elements
 */
enum iax2_ie_type {
	/*! Number/extension being called */
	IAX2_IE_CALLED_NUMBER   = 0x01,
	/*! Calling number */
	IAX2_IE_CALLING_NUMBER  = 0x02,
	/*! Calling number ANI for billing */
	IAX2_IE_CALLING_ANI     = 0x03,
	/*! Name of caller */
	IAX2_IE_CALLING_NAME    = 0x04,
	/*! Context for called number */
	IAX2_IE_CALLED_CONTEXT  = 0x05,
	/*! Username (peer or user) for authentication */
	IAX2_IE_USERNAME        = 0x06,
	/*! Password for authentication */
	IAX2_IE_PASSWORD        = 0x07,
	/*! Actual CODEC capability */
	IAX2_IE_CAPABILITY      = 0x08,
	/*! Desired CODEC format */
	IAX2_IE_FORMAT          = 0x09,
	/*! Desired language */
	IAX2_IE_LANGUAGE        = 0x0A,
	/*! Protocol version */
	IAX2_IE_VERSION         = 0x0B,
	/*! CPE ADSI capability */
	IAX2_IE_ADSICPE         = 0x0C,
	/*! Originally dialed DNID */
	IAX2_IE_DNID            = 0x0D,
	/*! Authentication method(s) */
	IAX2_IE_AUTHMETHODS     = 0x0E,
	/*! Challenge data for MD5/RSA */
	IAX2_IE_CHALLENGE       = 0x0F,
	/*! MD5 challenge result */
	IAX2_IE_MD5_RESULT      = 0x10,
	/*! RSA challenge result */
	IAX2_IE_RSA_RESULT      = 0x11,
	/*! Apparent address of peer */
	IAX2_IE_APPARENT_ADDR   = 0x12,
	/*! When to refresh registration */
	IAX2_IE_REFRESH         = 0x13,
	/*! Dialplan status */
	IAX2_IE_DPSTATUS        = 0x14,
	/*! Call number of peer */
	IAX2_IE_CALLNO          = 0x15,
	/*! Cause */
	IAX2_IE_CAUSE           = 0x16,
	/*! Unknown IAX command */
	IAX2_IE_IAX2_UNKNOWN    = 0x17,
	/*! How many messages waiting */
	IAX2_IE_MSGCOUNT        = 0x18,
	/*! Request auto-answer */
	IAX2_IE_AUTOANSWER      = 0x19,
	/*! Request music on hold with QUELCH */
	IAX2_IE_MUSICONHOLD     = 0x1A,
	/*! Transfer Request Identifier */
	IAX2_IE_TRANSFERID      = 0x1B,
	/*! Referring DNIS */
	IAX2_IE_RDNIS           = 0x1C,
	/*! Provisioning information */
	IAX2_IE_PROVISIONING    = 0x1D,
	/*! AES Provisioning information */
	IAX2_IE_AESPROVISIONING = 0x1E,
	/*! The current date and time */
	IAX2_IE_DATETIME        = 0x1F,
	/*! Device type */
	IAX2_IE_DEVICETYPE      = 0x20,
	/*! Service Identifier */
	IAX2_IE_SERVICEIDENT    = 0x21,
	/*! Firmware revision */
	IAX2_IE_FIRMWAREVER     = 0x22,
	/*! Firmware block description */
	IAX2_IE_FWBLOCKDESC     = 0x23,
	/*! Firmware block of data */
	IAX2_IE_FWBLOCKDATA     = 0x24,
	/*! Provisioning version */
	IAX2_IE_PROVVER         = 0x25,
	/*! Calling presentation */
	IAX2_IE_CALLINGPRES     = 0x26,
	/*! Calling type of number */
	IAX2_IE_CALLINGTON      = 0x27,
	/*! Calling transit network select */
	IAX2_IE_CALLINGTNS      = 0x28,
	/*! Supported sampling rates */
	IAX2_IE_SAMPLINGRATE    = 0x29,
	/*! Hangup cause */
	IAX2_IE_CAUSECODE       = 0x2A,
	/*! Encryption format */
	IAX2_IE_ENCRYPTION      = 0x2B,
	/*! 128-bit AES encryption key */
	IAX2_IE_ENCKEY          = 0x2C,
	/*! CODEC Negotiation */
	IAX2_IE_CODEC_PREFS     = 0x2D,
	/*! Received jitter, as in RFC1889 */
	IAX2_IE_RR_JITTER       = 0x2E,
	/*! Received loss, as in RFC1889 */
	IAX2_IE_RR_LOSS         = 0x2F,
	/*! Received frames */
	IAX2_IE_RR_PKTS         = 0x30,
	/*! Max playout delay for received frames in ms */
	IAX2_IE_RR_DELAY        = 0x31,
	/*! Dropped frames (presumably by jitterbuffer) */
	IAX2_IE_RR_DROPPED      = 0x32,
	/*! Frames received Out of Order */
	IAX2_IE_RR_OOO          = 0x33,
	/*! Variable */
	IAX2_IE_VARIABLE        = 0x34,
	/*! OSP Token */
	IAX2_IE_OSPTOKEN        = 0x35,
};

/*!
 * \brief Media frame formats
 */
enum iax2_format {
	/*! G.723.1 compression */
	IAX2_FORMAT_G723_1 =     (1 << 0),
	/*! GSM compression */
	IAX2_FORMAT_GSM =        (1 << 1),
	/*! Raw mu-law data (G.711) */
	IAX2_FORMAT_ULAW =       (1 << 2),
	/*! Raw A-law data (G.711) */
	IAX2_FORMAT_ALAW =       (1 << 3),
	/*! ADPCM (G.726, 32kbps, AAL2 codeword packing) */
	IAX2_FORMAT_G726_AAL2 =	 (1 << 4),
	/*! ADPCM (IMA) */
	IAX2_FORMAT_ADPCM =      (1 << 5),
	/*! Raw 16-bit Signed Linear (8000 Hz) PCM */
	IAX2_FORMAT_SLINEAR =    (1 << 6),
	/*! LPC10, 180 samples/frame */
	IAX2_FORMAT_LPC10 =      (1 << 7),
	/*! G.729A audio */
	IAX2_FORMAT_G729A =      (1 << 8),
	/*! SpeeX Free Compression */
	IAX2_FORMAT_SPEEX =      (1 << 9),
	/*! iLBC Free Compression */
	IAX2_FORMAT_ILBC =       (1 << 10),
	/*! ADPCM (G.726, 32kbps, RFC3551 codeword packing) */
	IAX2_FORMAT_G726 =       (1 << 11),
	/*! G.722 */
	IAX2_FORMAT_G722 =       (1 << 12),
	/*! Maximum audio format */
	IAX2_FORMAT_MAX_AUDIO =  (1 << 15),
	/*! Maximum audio mask */
	IAX2_FORMAT_AUDIO_MASK = ((1 << 16)-1),
	/*! JPEG Images */
	IAX2_FORMAT_JPEG =       (1 << 16),
	/*! PNG Images */
	IAX2_FORMAT_PNG =        (1 << 17),
	/*! H.261 Video */
	IAX2_FORMAT_H261 =       (1 << 18),
	/*! H.263 Video */
	IAX2_FORMAT_H263 =       (1 << 19),
	/*! H.263+ Video */
	IAX2_FORMAT_H263_PLUS =  (1 << 20),
	/*! H.264 Video */
	IAX2_FORMAT_H264 =       (1 << 21),
	/*! Maximum video format */
	IAX2_FORMAT_MAX_VIDEO =  (1 << 24),
	IAX2_FORMAT_VIDEO_MASK = (((1 << 25)-1) & ~(IAX2_FORMAT_AUDIO_MASK))
};

/*!
 * \brief Authentication methods
 */
enum iax2_auth_methods {
	/*! Plaintext authentication */
	IAX2_AUTH_PLAINTEXT = (1 << 0),
	/*! MD5 Challenge/Response authentication */
	IAX2_AUTH_MD5       = (1 << 1),
	/*! RSA authentication */
	IAX2_AUTH_RSA       = (1 << 2)
};

/*!
 * \brief Direction of an iax2_frame
 */
enum iax2_frame_direction {
	/*! Unknown, or not yet set */
	IAX2_DIRECTION_UNKNOWN,
	/*! Inbound frame */
	IAX2_DIRECTION_IN,
	/*! Outbound frame */
	IAX2_DIRECTION_OUT,
};

/*!
 * \brief An IAX2 information element
 * 
 * This is the structure used to hold the exact representation of an
 * IAX2 information element.  IEs are used with FULL frames of type IAX2.
 */
struct iax2_ie {
	/*! Get the string representation of the IE type */
	const char *type2str(void) const;
	/*! The numeric value that identifies this IE, as defined in the spec */
	enum iax2_ie_type type:8;
	/*! The number of bytes in the payload of this IE */
	unsigned char datalen;
	/*! The payload data for this IE */
	unsigned char data[0];
} __attribute__ ((packed));

/*! The maximum data length for IAX2 IEs */
#define IAX2_IE_MAX_DATALEN 255

/*!
 * \brief An IAX2 frame
 *
 * This class represents an IAX2 network frame.  It can be used to set up the
 * parameters for a frame, and then it will handle the bitwise encoding for
 * sending over the network.  For the other direction, there is a constructor
 * that can be passed a buffer of a raw IAX2 frame that came from the network,
 * and the frame will be parsed and the fields of this data structure will be
 * filled to represent the frame that was received.
 */
class iax2_frame {
public:
	iax2_frame(void);
	iax2_frame(const unsigned char *buf, size_t buflen);
	~iax2_frame(void);

	/*!
	 * \brief Prepare and deliver this frame
	 *
	 * \param sin The address and port to send the frame to
	 * \param sockfd The socket to use to send the frame
	 *
	 * \retval 0 success
	 * \retval non-zero failure
	 *
	 * After setting the parameters for this frame, this function is called to
	 * send the frame over the network.  The bitwise encoding of the frame is
	 * handled internally using the fields of this data structure to fill in
	 * the frame.
	 */
	int send(const struct sockaddr_in *sin, const int sockfd);

	/*!
	 * \brief Print contents of the frame
	 *
	 * This function will print the contents and direction of this frame.  It
	 * is mostly for debugging purposes.
	 */
	void print(const struct sockaddr_in *);

	/*!
	 * \brief Add an information element to the frame
	 *
	 * \param type the type of IAX2 Information Element
	 * \param data the data portion of the Information Element
	 * \param datalen the length of the data portion in bytes
	 *
	 * \retval 0 Success
	 * \retval non-zero Failure
	 *
	 * \note Information Elements are only valid for Full Frames of type IAX2
	 */
	iax2_frame &add_ie(enum iax2_ie_type type, const void *data, unsigned char datalen);

	/*!
	 * \brief Add an information element that contains a string
	 *
	 * \param type the type of IAX2 information Element
	 * \param str The string that will be the data portion of the 
	 *        Information Element
	 *
	 * \retval 0 Success
	 * \retval non-zero Failure
	 *
	 * \note Information Elements are only valid for Full Frames of type IAX2
	 */
	iax2_frame &add_ie_string(enum iax2_ie_type type, const char *str);
	int add_ie_string(const char *type, const char *str);
	
	/*!
	 * \brief Add an information element that contains an unsigned short integer
	 *
	 * \param type the type of IAX2 information Element
	 * \param num The number that will be the data portion of the 
	 *        Information Element
	 *
	 * \retval 0 Success
	 * \retval non-zero Failure
	 *
	 * \note Information Elements are only valid for Full Frames of type IAX2
	 */
	iax2_frame &add_ie_unsigned_short(enum iax2_ie_type type, u_int16_t num);
	int add_ie_unsigned_short(const char *type, uint16_t num);

	/*!
	 * \brief Add an information element that contains an unsigned short integer
	 *
	 * \param type the type of IAX2 information Element
	 * \param num The number that will be the data portion of the 
	 *        Information Element
	 *
	 * \retval 0 Success
	 * \retval non-zero Failure
	 *
	 * \note Information Elements are only valid for Full Frames of type IAX2
	 */
	iax2_frame &add_ie_unsigned_long(enum iax2_ie_type type, u_int32_t num);
	int add_ie_unsigned_long(const char *type, uint32_t num);

	/*!
	 * \brief Get an information element of the given type with a string
	 *
	 * \param type the information element type to retrieve from this frame
	 *
	 * \return the information element data as a string
	 */
	const char *get_ie_string(enum iax2_ie_type type) const;

	/*!
	 * \brief Get an information element of the given type with a 32 bit uint
	 *
	 * \param type the information element type to retrieve from this frame
	 *
	 * \return the information element data as a u_int32_t
	 */
	u_int32_t get_ie_unsigned_long(enum iax2_ie_type type) const;

	inline enum iax2_frame_direction get_direction(void) const
		{ return direction; }
	inline iax2_frame &set_direction(enum iax2_frame_direction d)
		{ direction = d; return *this; }
	
	inline enum iax2_frame_shell get_shell(void) const
		{ return shell; }
	inline iax2_frame &set_shell(enum iax2_frame_shell s)
		{ shell = s; return *this; }
	/*!
	 * \brief Set the frame shell by a string
	 * \arg val one of FULL, MINI, or META
	 * \retval 0 success
	 * \retval -1 error
	 * \sa iax2_frame_shell
	 */
	int set_shell(const char *val);

	inline enum iax2_frame_type get_type(void) const
		{ return type; }
	inline iax2_frame &set_type(enum iax2_frame_type t)
		{ type = t; return *this; }
	/*!
	 * \brief Set the type by a string
	 * \arg val the type
	 * \retval 0 success
	 * \retval -1 error
	 * \sa iax2_frame_type
	 */
	int set_type(const char *val);

	inline enum iax2_meta_type get_meta_type(void) const
		{ return meta_type; }
	inline iax2_frame &set_meta_type(enum iax2_meta_type mt)
		{ meta_type = mt; return *this; }
	/*!
	 * \brief Set the meta type by a string
	 * \retval 0 success
	 * \retval -1 error
	 * \sa iax2_meta_type
	 */
	int set_meta_type(const char *val);

	// XXX These functions need to be updated to handle a coded subclass	
	inline unsigned int get_subclass(void) const
		{ return subclass; }
	inline iax2_frame &set_subclass(unsigned int sc)
		{ subclass = sc; return *this; }
	/*!
	 * \brief Set the subclass by a string
	 * \retval 0 success
	 * \retval -1 error
	 */
	int set_subclass(const char *);

	inline unsigned short get_source_call_num(void) const
		{ return source_call_num; }
	inline iax2_frame &set_source_call_num(unsigned short num)
		{ source_call_num = num; return *this; }
	
	inline unsigned short get_dest_call_num(void) const
		{ return dest_call_num; }
	inline iax2_frame &set_dest_call_num(unsigned short num)
		{ dest_call_num = num; return *this; }

	inline unsigned char get_out_seq_num(void) const
		{ return out_seq_num; }
	inline iax2_frame &set_out_seq_num(unsigned char num)
		{ out_seq_num = num; return *this; }

	inline unsigned char get_in_seq_num(void) const
		{ return in_seq_num; }
	inline iax2_frame &set_in_seq_num(unsigned char num)
		{ in_seq_num = num; return *this; }

	inline unsigned int get_timestamp(void) const
		{ return timestamp; }
	inline iax2_frame &set_timestamp(unsigned int ts)
		{ timestamp = ts; return *this; }

	inline bool get_retransmission(void) const
		{ return retransmission; }
	inline iax2_frame &set_retransmission(bool retrans)
		{ retransmission = retrans; return *this; }

	inline const void *get_raw_data(void) const
		{ return raw_data; }
	inline unsigned int get_raw_data_len(void) const
		{ return raw_data_len; }

	iax2_frame &set_raw_data(const void *data, unsigned int data_len);
	
private:
	void print_ies(void) const;
	void print_full_frame(const struct sockaddr_in *) const;
	void print_mini_frame(const struct sockaddr_in *) const;
	void print_meta_frame(const struct sockaddr_in *) const;
	void parse_full_frame(const unsigned char *buf, size_t buflen);
	void parse_mini_frame(const unsigned char *buf, size_t buflen);
	void parse_meta_frame(const unsigned char *buf, size_t buflen);
	void parse_meta_video_frame(const unsigned char *buf, size_t buflen);
	int send_full_frame(const struct sockaddr_in *sin, const int sockfd);
	int send_meta_frame(const struct sockaddr_in *sin, const int sockfd);
	int send_meta_video_frame(const struct sockaddr_in *sin, const int sockfd);
	int send_mini_frame(const struct sockaddr_in *sin, const int sockfd);
	size_t total_ie_len(void) const;
	const char *type2str(void) const;
	const char *iax2subclass2str(void) const;
	const char *subclass2str(void) const;
	const char *meta_type2str(void) const;

	/*! Inbound or Outbound frame */
	enum iax2_frame_direction direction:2;
	/*! Full, Mini, or Meta frame */
	enum iax2_frame_shell shell;
	/*! The type for a full frame */
	enum iax2_frame_type type;
	/*! Source call number */
	unsigned short source_call_num;
	/*! Destination call number */
	unsigned short dest_call_num;
	/*! Timestamp */
	unsigned int timestamp;
	/*! Outbound sequence number */
	unsigned char out_seq_num;
	/*! Inbound sequence number */
	unsigned char in_seq_num;
	/*! This frame is a retransmission */
	bool retransmission;
	/*! if the subclass is coded as a power of 2 */
	bool subclass_coded;
	/*! Frame type subclass */
	unsigned int subclass:7;

	/*! Information Elements */
	list<iax2_ie *> ies;
	typedef list<iax2_ie *>::const_iterator iax2_ie_iterator;

	enum iax2_meta_type meta_type;

	void *raw_data;
	unsigned int raw_data_len;
};

/*!
 * \brief A literal IAX2 full frame
 *
 * This structure is used internally to iax2_frame for encoding a frame to be
 * sent over the network.
 */
struct iax2_full_header {
	/*! Source call number -- high bit must be 1 */
	uint16_t scallno;
	/*! Destination call number -- high bit is 1 if retransmission */
	uint16_t dcallno;
	/*! 32-bit timestamp in milliseconds (from 1st transmission) */
	uint32_t ts;
	/*! Packet number (outgoing) */
	uint8_t oseqno;
	/*! Packet number (next incoming expected) */
	uint8_t iseqno;
	/*! Frame type */
	uint8_t type;
	/*! Compressed subclass */
	uint8_t csub;
	/*! Information Elements */
	uint8_t iedata[0];
} __attribute__ ((__packed__));

/*!
 * \brief A literal IAX2 meta frame
 *
 * This struct is used for preparing or parsing a meta frame to
 * or from the network.
 */
struct iax2_meta_header {
	/*! Zeros field -- must be zero */
    uint16_t zeros;
	/*! Meta command */
    uint8_t metacmd;
	/*! Command Data */
    uint8_t cmddata;
	/*! Frame data */
    uint8_t data[0];
} __attribute__ ((__packed__)); 

/*!
 * \brief A literal IAX2 meta video frame
 *
 * This struct is used for preparing or parsing a frame to
 * or from the network that contains video media.
 */
struct iax2_meta_video_header {
	/*! Zeros field -- must be zero */
    uint16_t zeros;
	/*! Video call number */
    uint16_t callno;
	/*! Timestamp and mark if present */
    uint16_t ts;
	/*! Video Frame data */
    uint8_t data[0];
} __attribute__ ((__packed__));

/*!
 * \brief A literal IAX2 mini frame
 *
 * This struct is used for preparing or parsing a mini frame to
 * or from the network.
 *
 * \note The frame type is implicitly audio and the subclass is implicit 
 *       from last FULL frame.
 */
struct iax2_mini_header {
	/*! Source call number -- high bit must be 0, rest must be non-zero */
	uint16_t callno;
	/*! 16-bit Timestamp (high 16 bits from last ast_iax2_full_hdr) */
	uint16_t ts;
	/*! The payload (media) */
	uint8_t data[0];
} __attribute__ ((__packed__));

#endif /* IAX2_FRAME_H */
