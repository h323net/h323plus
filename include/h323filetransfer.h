/*
 * h323filetransfer.h
 *
 * H323 File Transfer class.
 *
 * h323plus library
 *
 * Copyright (c) 2008 ISVO (Asia) Pte. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the General Public License (the  "GNU License"), in which case the
 * provisions of GNU License are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GNU License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GNU License. If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the GNU License."
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Id: h323filetransfer.h,v 1.6.2.1 2015/10/10 08:53:40 shorne Exp $
 *
 */


#ifdef H323_FILE

#include <list>

//////////////////////////////////////////////////////////////////////////////////

class H323File
{
  public:
	PString    m_Filename;
	PString    m_Directory;
	long       m_Filesize;
};

class H323FileTransferList : public list<H323File>
{ 
  public:
    H323FileTransferList();
	PINDEX GetSize();
	H323File * GetAt(PINDEX i);
    void Add(const PString & filename, const PDirectory & directory, long filesize);
    void SetSaveDirectory(const PString directory);
	const PDirectory & GetSaveDirectory();
 
	void SetDirection(H323Channel::Directions _direction);
	H323Channel::Directions GetDirection();

	void SetMaster(PBoolean state);
	PBoolean IsMaster();

  protected:
	H323Channel::Directions direction;
	PDirectory saveDirectory;
	PBoolean master;
};


//////////////////////////////////////////////////////////////////////////////////
/**This class describes the File Transfer logical channel.
 */
class H323FileTransferHandler;
class H323FileTransferChannel : public H323Channel
{
    PCLASSINFO(H323FileTransferChannel, H323Channel);
  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323FileTransferChannel(
      H323Connection & connection,          ///<  Connection to endpoint for channel
      const H323Capability & capability,    ///<  Capability channel is using
      Directions theDirection,              ///<  Direction of channel
	  RTP_UDP & rtp,                        ///<  RTP Session for channel
      unsigned theSessionID,                ///<  Session ID for channel
	  const H323FileTransferList & list		///<  List of files to Request/Send
    );

    ~H323FileTransferChannel();
  //@}

  /**@name Overrides from class H323Channel */
  //@{

   /**Indicate the direction of the channel.
       Return if the channel is bidirectional, or unidirectional, and which
       direction for the latter case.
     */
    virtual H323Channel::Directions GetDirection() const;

    /**Set the initial bandwidth for the channel.
       This calculates the initial bandwidth required by the channel and
       returns TRUE if the connection can support this bandwidth.

       The default behaviour does nothing.
     */
    virtual PBoolean SetInitialBandwidth();

    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      H245_OpenLogicalChannel & openPDU  ///<  Open PDU to send. 
    ) const;

    virtual PBoolean OnSendingPDU(H245_H2250LogicalChannelParameters & param) const;

    virtual void OnSendOpenAck(const H245_OpenLogicalChannel & openPDU, 
							 H245_OpenLogicalChannelAck & ack) const;

    virtual void OnSendOpenAck(H245_H2250LogicalChannelAckParameters & param) const;


    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default makes sure the parameters are compatible and passes on
       the PDU to the rtp session.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_OpenLogicalChannel & pdu,    ///<  Open PDU
      unsigned & errorCode                    ///<  Error code on failure
    );

    virtual PBoolean OnReceivedPDU(const H245_H2250LogicalChannelParameters & param,
							 unsigned & errorCode);

    virtual PBoolean OnReceivedAckPDU(const H245_OpenLogicalChannelAck & pdu);

    virtual PBoolean OnReceivedAckPDU(const H245_H2250LogicalChannelAckParameters & param);


    /**Handle channel data reception.

       This is called by the thread started by the Start() function and is
       a loop reading from the transport and calling HandlePacket() for each
       PDU read.
      */
    virtual void Receive();

    /**Handle channel data transmission.

       This is called by the thread started by the Start() function and is
       typically a loop reading from the codec and writing to the transport
       (eg an RTP_session).
      */
    virtual void Transmit();


    virtual PBoolean Open();

    virtual PBoolean Start();

    virtual void Close();

    virtual PBoolean SetDynamicRTPPayloadType(int newType);
    RTP_DataFrame::PayloadTypes GetDynamicRTPPayloadType() const { return rtpPayloadType; }
	
    H323FileTransferHandler * GetHandler() const { return fileHandler; }

  //@}

  protected:
    PBoolean ExtractTransport(const H245_TransportAddress & pdu,
			     PBoolean isDataPort,
			     unsigned & errorCode
						 );

    PBoolean RetreiveFileInfo(const H245_GenericInformation & info, 
			     H323FileTransferList & filelist
						 );

    void SetFileList(H245_OpenLogicalChannel & open, H323FileTransferList flist) const;
    PBoolean GetFileList(const H245_OpenLogicalChannel & open);

    unsigned sessionID;
    Directions direction;
    RTP_UDP & rtpSession;
    H323_RTP_Session & rtpCallbacks;
    H323FileTransferHandler *fileHandler;
	H323FileTransferList filelist;
    RTP_DataFrame::PayloadTypes rtpPayloadType;

};

////////////////////////////////////////////////////////////////////

class H323FileTransferCapability : public H323DataCapability
{
  PCLASSINFO(H323FileTransferCapability, H323DataCapability);

  public:
  /**@name Construction */
  //@{
    /**Create a new File Transfer capability with default values.
      */
    H323FileTransferCapability();

    /**Create a new File Transfer capability with defined value.
      */
    H323FileTransferCapability(unsigned maxBitRate, unsigned maxBlockSize);
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;


	/**Clone the File Transfer Capability
	  */
	PObject * Clone() const;
  //@}

  /**@name Identification functions */
  //@{

    virtual PString GetFormatName() const
    { return "TFTP over RTP"; }

    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns one of the four possible combinations of mode and speed
       using the enum values of the protocol ASN H245_AudioCapability class.
     */
	virtual unsigned GetSubType() const;

    /**Get the default RTP session.
       This function gets the default RTP session ID for the capability
       type. 
       The default behaviour returns zero, indicating it is not an RTP
       based capability.
      */
    virtual unsigned GetDefaultSessionID() const;
  //@}
    
  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour sets the pdu and calls OnSendingPDU with a
       H245_DataProtocolCapability parameter.
     */
    virtual PBoolean OnSendingPDU(
      H245_DataApplicationCapability & pdu
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour does nothing.
     */

    virtual PBoolean OnReceivedPDU(
      const H245_DataApplicationCapability & cap  ///< PDU to get information from
    );

    /**This function is called whenever and outgoing RequestMode
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour sets the PDUs tag according to the GetSubType()
       function (translated to different enum).
     */
    virtual PBoolean OnSendingPDU(
      H245_DataMode & pdu  ///< PDU to set information on
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour does nothing.
     */
    PBoolean OnReceivedPDU(const H245_GenericCapability & pdu);

    /**This function is called whenever and outgoing TerminalCapabilitySet
       PDU is being constructed for the control channel. It allows the
       capability to set the PDU fields from information in members specific
       to the class.

       The default behaviour calls the OnSendingPDU() function with a more
       specific PDU type.
     */
    PBoolean OnSendingPDU(H245_GenericCapability & pdu) const;

	//@}

	enum blockSizes {
		e_RFC1350   = 512,
		e_1024      = 1024,
		e_1428      = 1428,
		e_2048      = 2048,
		e_4096      = 4096,
		e_8192      = 8192
	};

  /**@name Operations */
  //@{
    /**Create the channel instance, allocating resources as required.
     */
    virtual H323Channel * CreateChannel(
      H323Connection & connection,          ///< Owner connection for channel
      H323Channel::Directions direction,    ///< Direction of channel
      unsigned sessionID,                   ///< Session ID for RTP channel
      const H245_H2250LogicalChannelParameters * param
                                            ///< Parameters for channel
    ) const;
  //@}

  /**@name Utilities */
  //@{
    /**Get the BlockSize of data to be sent.
     */
	unsigned GetBlockSize() const  { return m_blockSize; }

    /**Get the BlockSize of data to be sent (as Capability Parameter).
     */
	unsigned GetBlockRate() const  { return maxBitRate / m_blockOctets; }

    /**Get the BlockSize of data to be sent (as Octets size).
     */
	unsigned GetOctetSize() const  { return m_blockOctets; }

    /**Get the Transfer Mode.
     */
	unsigned GetTransferMode() const  { return m_transferMode; }

	/**Set the List of files to send
	 */
	void SetFileTransferList(const H323FileTransferList & list);
  //@}

  protected:
    unsigned				m_blockSize;          ///< Size indicator in capability negotiation
	unsigned				m_blockOctets;        ///< Block Octet size
	unsigned				m_transferMode;       ///< Mode of transfer Raw Tftp or RTP encaptulated
	H323FileTransferList	m_filelist;           ///< File list to Request/Send

};

class H323FilePacket : public PBYTEArray
{
  PCLASSINFO(H323FilePacket, PBYTEArray);

public:
  enum opcodes {
	  e_PROB,
	  e_RRQ,
	  e_WRQ,
	  e_DATA,
	  e_ACK,
	  e_ERROR
  };

  enum errCodes {
      ErrNotDefined,
	  ErrFileNotFound,
	  ErrAccessViolation,
	  ErrDiskFull,
	  ErrIllegalOperation,
	  ErrUnknownID,
	  ErrFileExists,
	  ErrNoSuchUser
  }; 

  void BuildPROB();
  void BuildRequest(opcodes code, const PString & filename, int filesize, int blocksize);
  void BuildData(int blockid, int size);
  void BuildACK(int blockid,int filesize = 0);
  void BuildError(int errorcode,PString errmsg);

  H323FilePacket::opcodes GetPacketType() const;

  // for RRQ/WRQ Only
  PString GetFileName() const;
  unsigned GetFileSize() const;
  unsigned GetBlockSize() const;

  // for DATA only
  unsigned GetDataSize() const;
  BYTE * GetDataPtr();
  int GetBlockNo() const;

  // For ACK
  int GetACKBlockNo() const;

  // for ERROR messages
  void GetErrorInformation(int & ErrCode, PString & ErrStr) const;

  protected:
	void attach(PString & data);

};

class H323FileIOChannel : public PIndirectChannel
{
  public:

	  enum fileError {
		  e_OK,
		  e_NotFound,
		  e_AccessDenied,
		  e_FileExists = 6
	  };

    H323FileIOChannel(PFilePath _file, PBoolean read);
	~H323FileIOChannel();

	PBoolean IsError(fileError & err);

	PBoolean Open();
	PBoolean Close();

    PBoolean Read(void * buffer, PINDEX & amount);
    PBoolean Write(const void * buf, PINDEX amount);

	unsigned GetFileSize();

  protected:
	PBoolean CheckFile(PFilePath _file, PBoolean read, fileError & errCode);

    PMutex chanMutex;
	PBoolean fileopen;
	unsigned filesize;
	fileError IOError;
};


//////////////////////////////////////////////////////////////////////////////
// FileTransferHandler
// Derive you class from this to set and collect events associated with
// the file transfer
//
class H323FileTransferHandler : public PObject
{
  PCLASSINFO(H323FileTransferHandler, PObject);
	
public:
	
  H323FileTransferHandler(H323Connection & connection, 
	                    unsigned sessionID,
			    H323Channel::Directions dir,
			    H323FileTransferList & filelist
			  );

  ~H323FileTransferHandler();

  enum transferState {
	  e_probing,         ///< Probing for connectivity
	  e_connect,         ///< Received a probe packet and confirm
	  e_waiting,         ///< Waiting for RRQ/WRQ command
	  e_sending,         ///< Sending Data
	  e_receiving,       ///< Receiving Data
	  e_completed,       ///< Transfer Completed
	  e_error            ///< Error Received
  };

  enum receiveStates {
	  recOK,             ///< received block OK
	  recPartial,        ///< Only partial block received
	  recComplete,       ///< File was fully received
	  recIncomplete,     ///< Incomplete receival of File.
	  recTimeOut,        ///< TimeOut waiting for packet.
	  recReady           ///< Ready to receive files
  };

//////////////////////////
// User override to custom set blocksize and transmission rate

  virtual void SetBlockSize(H323FileTransferCapability::blockSizes size);
  virtual void SetMaxBlockRate(unsigned rate);

// User override to get events

  virtual void OnStateChange(transferState newState) {};
  virtual void OnFileStart(const PString & filename, unsigned filesize, PBoolean transmit) {};
  virtual void OnFileOpenError(const PString & filename, H323FileIOChannel::fileError _err) {};
  virtual void OnError(const PString ErrMsg) {};
  virtual void OnFileComplete(const PString & filename) {};
  virtual void OnFileProgress(const PString & filename, int Blockno, unsigned OctetsSent, PBoolean transmit) {};
  virtual void OnFileError(const PString & filename, int Blockno, PBoolean transmit) {};
  virtual void OnTransferComplete(PBoolean master) {};

//////////////////////////


  H323FileTransferCapability::blockSizes GetBlockSize()  
        { return (H323FileTransferCapability::blockSizes)blockSize; }
  unsigned GetBlockRate()  
        { return blockRate; }

  PBoolean Start(H323Channel::Directions direction);
  PBoolean Stop(H323Channel::Directions direction);

  void SetPayloadType(RTP_DataFrame::PayloadTypes _type);

  void SetMaster(PBoolean newVal) 
	        { master = newVal; }

protected:

  PBoolean TransmitFrame(H323FilePacket & buffer, PBoolean final);
  PBoolean ReceiveFrame(H323FilePacket & buffer, PBoolean & final);

  void ChangeState(transferState newState);
  void SetBlockState(receiveStates state);

  H323FileTransferList filelist;

  PThread * TransmitThread;
  PThread * ReceiveThread;

  PDECLARE_NOTIFIER(PThread, H323FileTransferHandler, Transmit);
  PDECLARE_NOTIFIER(PThread, H323FileTransferHandler, Receive);

  PBoolean transmitRunning;
  PBoolean receiveRunning;
  PSyncPointAck        exitTransmit;
  PSyncPointAck        exitReceive;
  PTimer               shutdownTimer;

  RTP_DataFrame transmitFrame;   ///< Template RTP Frame for sending/receiving

  RTP_Session * session;         ///< RTP Session

  PSyncPoint probMutex;    ///< probing Mutex
  PMutex transferMutex;     ///< Mutex for read/write operation
  PMutex stateMutex;        ///< Mutex for state change
  PSyncPoint nextFrame;     ///< Wait for confirmation before sending the next block
  PINDEX responseTimeOut;   ///< Time to wait for a response (set at 1.5 sec)

  PTime *StartTime;
  PBoolean master;          ///< Was Initiator of the channel

private:
  RTP_DataFrame::PayloadTypes rtpPayloadType;  ///< Payload Type should be dynamically allocated
  int msBetweenBlocks;                         ///< milliseconds between sending next block of data
  PAdaptiveDelay sendwait;                     ///< Wait before sending the next block of data
  unsigned timestamp;                          ///< TimeStamp of last packet received
  unsigned blockSize;                          ///< Size of the Blocks to transmit
  unsigned blockRate;                          ///< Rate of Transmission in bits/sec
  
  // Transfer states
  H323FileIOChannel * curFile;                 ///< Current File being sent
  H323FileIOChannel::fileError ioerr;          ///< Last IO error
  transferState currentState;                  ///< Current state of Transmission
  receiveStates blockState;                    ///< State of Received block          
  int lastBlockNo;                             ///< Last Block No sent
  unsigned lastBlockSize;                      ///< Last Block Size sent
  PString curFileName;						   ///< Current FileName being sent/received
  unsigned curFileSize;                        ///< Current File being Transmitted size
  unsigned curBlockSize;                       ///< Block size of current transmittion
  unsigned curProgSize;						   ///< Current amount of data sent/received
};

#endif
