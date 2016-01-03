/*
 * mediafmt.h
 *
 * Media Format descriptions
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Id: mediafmt.h,v 1.17 2014/08/27 02:18:26 shorne Exp $
 *
 */

#ifndef __OPAL_MEDIAFMT_H
#define __OPAL_MEDIAFMT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include "rtp.h"

#include <limits>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

class OpalMediaFormat;


///////////////////////////////////////////////////////////////////////////////

/**Base class for options attached to an OpalMediaFormat.
  */
class OpalMediaOption : public PObject
{
    PCLASSINFO(OpalMediaOption, PObject);
  public:
    enum MergeType {
      NoMerge,
      MinMerge,
      MaxMerge,
      EqualMerge,
      NotEqualMerge,
      AlwaysMerge,

      // Synonyms
      AndMerge = MaxMerge,
      OrMerge  = MinMerge,
      XorMerge = NotEqualMerge,
      NotXorMerge = EqualMerge
    };

  protected:
    OpalMediaOption(
      const char * name,
      bool readOnly,
      MergeType merge
    );

  public:
    virtual Comparison Compare(const PObject & obj) const;

    bool Merge(
      const OpalMediaOption & option
    );
    virtual Comparison CompareValue(
      const OpalMediaOption & option
    ) const = 0;
    virtual void Assign(
      const OpalMediaOption & option
    ) = 0;

    PString AsString() const;
    bool FromString(const PString & value);

    const PString & GetName() const { return m_name; }

    bool IsReadOnly() const { return m_readOnly; }
    void SetReadOnly(bool readOnly) { m_readOnly = readOnly; }

    MergeType GetMerge() const { return m_merge; }
    void SetMerge(MergeType merge) { m_merge = merge; }

    const PString & GetFMTPName() const { return m_FMTPName; }
    void SetFMTPName(const char * name) { m_FMTPName = name; }

    const PString & GetFMTPDefault() const { return m_FMTPDefault; }
    void SetFMTPDefault(const char * value) { m_FMTPDefault = value; }

    struct H245GenericInfo {
      unsigned ordinal:16;
      enum Modes {
        None,
        Collapsing,
        NonCollapsing
      } mode:3;
      enum IntegerTypes {
        UnsignedInt,
        Unsigned32,
        BooleanArray
      } integerType:3;
      bool excludeTCS:1;
      bool excludeOLC:1;
      bool excludeReqMode:1;
    };

    const H245GenericInfo & GetH245Generic() const { return m_H245Generic; }
    void SetH245Generic(const H245GenericInfo & gen) { m_H245Generic = gen; }

  protected:
    PCaselessString m_name;
    bool            m_readOnly;
    MergeType       m_merge;
    PCaselessString m_FMTPName;
    PString         m_FMTPDefault;
    H245GenericInfo m_H245Generic;
};

#if PTLIB_VER < 2110
#ifndef __USE_STL__
__inline istream & operator>>(istream & strm, bool& b)
{
   int i;strm >> i;b = i; return strm;
}
#endif
#endif

template <typename T>
class OpalMediaOptionValue : public OpalMediaOption
{
    PCLASSINFO(OpalMediaOptionValue, OpalMediaOption);
  public:
    OpalMediaOptionValue(
      const char * name,
      bool readOnly,
      MergeType merge = MinMerge,
      T value = 0,
      T minimum = std::numeric_limits<T>::min(),
      T maximum = std::numeric_limits<T>::max()
    ) : OpalMediaOption(name, readOnly, merge),
        m_value(value),
        m_minimum(minimum),
        m_maximum(maximum)
    { }

    virtual PObject * Clone() const
    {
      return new OpalMediaOptionValue(*this);
    }

    virtual void PrintOn(ostream & strm) const
    {
      strm << m_value;
    }

    virtual void ReadFrom(istream & strm)
    {
      T temp;
      strm >> temp;
      if (temp >= m_minimum && temp <= m_maximum)
        m_value = temp;
      else {
#if PTLIB_VER >= 2110 || defined(__USE_STL__)
	   strm.setstate(ios::badbit);
#else
	   strm.setf(ios::badbit , ios::badbit);
#endif
       }
    }

    virtual Comparison CompareValue(const OpalMediaOption & option) const {
	  if (!PIsDescendant(&option, OpalMediaOptionValue)) {
		  PTRACE(6,"MediaOpt\t" << option.GetName() << " not compared! Not descendent of OpalMediaOptionValue");
		  return GreaterThan;
	  }
      const OpalMediaOptionValue * otherOption = PDownCast(const OpalMediaOptionValue, &option);
      if (otherOption == NULL)
        return GreaterThan;
      if (m_value < otherOption->m_value || otherOption->m_value == 0)
        return LessThan;
      if (m_value > otherOption->m_value)
        return GreaterThan;
      return EqualTo;
    }

    virtual void Assign(
      const OpalMediaOption & option
    ) {
	  if (!PIsDescendant(&option, OpalMediaOptionValue)) {
		  PTRACE(6,"MediaOpt\t" << option.GetName() << " not assigned! Not descendent of OpalMediaOptionValue");
		  return;
	  }
      const OpalMediaOptionValue * otherOption = PDownCast(const OpalMediaOptionValue, &option);
      if (otherOption != NULL)
        m_value = otherOption->m_value;
    }

    T GetValue() const { return m_value; }
    void SetValue(T value) { m_value = value; }

  protected:
    T m_value;
    T m_minimum;
    T m_maximum;
};


typedef OpalMediaOptionValue<bool>     OpalMediaOptionBoolean;
typedef OpalMediaOptionValue<int>      OpalMediaOptionInteger;
typedef OpalMediaOptionValue<unsigned> OpalMediaOptionUnsigned;
typedef OpalMediaOptionValue<double>   OpalMediaOptionReal;


class OpalMediaOptionEnum : public OpalMediaOption
{
    PCLASSINFO(OpalMediaOptionEnum, OpalMediaOption);
  public:
    OpalMediaOptionEnum(
      const char * name,
      bool readOnly,
      const char * const * enumerations,
      PINDEX count,
      MergeType merge = EqualMerge,
      PINDEX value = 0
    );

    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

    virtual Comparison CompareValue(const OpalMediaOption & option) const;
    virtual void Assign(const OpalMediaOption & option);

    PINDEX GetValue() const { return m_value; }
    void SetValue(PINDEX value);

  protected:
    PStringArray m_enumerations;
    PINDEX       m_value;
};


class OpalMediaOptionString : public OpalMediaOption
{
    PCLASSINFO(OpalMediaOptionString, OpalMediaOption);
  public:
    OpalMediaOptionString(
      const char * name,
      bool readOnly
    );
    OpalMediaOptionString(
      const char * name,
      bool readOnly,
      const PString & value
    );

    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

    virtual Comparison CompareValue(const OpalMediaOption & option) const;
    virtual void Assign(const OpalMediaOption & option);

    const PString & GetValue() const { return m_value; }
    void SetValue(const PString & value);

  protected:
    PString m_value;
};


class OpalMediaOptionOctets : public OpalMediaOption
{
    PCLASSINFO(OpalMediaOptionOctets, OpalMediaOption);
  public:
    OpalMediaOptionOctets(
      const char * name,
      bool readOnly,
      bool base64
    );
    OpalMediaOptionOctets(
      const char * name,
      bool readOnly,
      bool base64,
      const PBYTEArray & value
    );
    OpalMediaOptionOctets(
      const char * name,
      bool readOnly,
      bool base64,
      const BYTE * data,
      PINDEX length
    );

    virtual PObject * Clone() const;
    virtual void PrintOn(ostream & strm) const;
    virtual void ReadFrom(istream & strm);

    virtual Comparison CompareValue(const OpalMediaOption & option) const;
    virtual void Assign(const OpalMediaOption & option);

    const PBYTEArray & GetValue() const { return m_value; }
    void SetValue(const PBYTEArray & value);
    void SetValue(const BYTE * data, PINDEX length);

  protected:
    PBYTEArray m_value;
    bool       m_base64;
};


///////////////////////////////////////////////////////////////////////////////

/**This class describes a media format as used in the OPAL system. A media
   format is the type of any media data that is trasferred between OPAL
   entities. For example an audio codec such as G.723.1 is a media format, a
   video codec such as H.261 is also a media format.
  */
class OpalMediaFormat : public PCaselessString
{
  PCLASSINFO(OpalMediaFormat, PCaselessString);

  public:
    PLIST(List, OpalMediaFormat);

    /**Default constructor creates a PCM-16 media format.
      */
    OpalMediaFormat();

    /**A constructor that only has a string name will search through the
       RegisteredMediaFormats list for the full specification so the other
       information fields can be set from the database.
      */
    OpalMediaFormat(
      const char * search,  ///<  Name to search for
      PBoolean exact = TRUE     ///<  Flag for if search is to match name exactly
    );

    /**Return TRUE if media format info is valid. This may be used if the
       single string constructor is used to check that it matched something
       in the registered media formats database.
      */
    PBoolean IsValid() const { return rtpPayloadType <= RTP_DataFrame::MaxPayloadType; }

    /**Copy a media format
      */
    OpalMediaFormat & operator=(
      const OpalMediaFormat & fmt ///<  other media format
    );

    /**Merge with another media format. This will alter and validate
       the options for this media format according to the merge rule for
       each option. The parameter is typically a "capability" while the
       current object isthe proposed channel format. This if the current
       object has a tx number of frames of 3, but the parameter has a value
       of 1, then the current object will be set to 1.

       Returns FALSE if the media formats are incompatible and cannot be
       merged.
      */
    virtual bool Merge(
      const OpalMediaFormat & mediaFormat
    );

    /**Get the RTP payload type that is to be used for this media format.
       This will either be an intrinsic one for the media format eg GSM or it
       will be automatically calculated as a dynamic media format that will be
       uniqueue amongst the registered media formats.
      */
    RTP_DataFrame::PayloadTypes GetPayloadType() const { return rtpPayloadType; }

    void SetPayloadType(RTP_DataFrame::PayloadTypes type) { rtpPayloadType = type; }
    enum {
      NonRTPSessionID           = 0,
      FirstSessionID            = 1,
      DefaultAudioSessionID     = 1,
      DefaultVideoSessionID     = 2,
      DefaultDataSessionID      = 3,
      DefaultH224SessionID      = 3,
      DefaultExtVideoSessionID  = 4,
      DefaultFileSessionID      = 5,
      LastSessionID             = 5
    };

    /**Get the default session ID for media format.
      */
    unsigned GetDefaultSessionID() const { return defaultSessionID; }

    /**Determine if the media format requires a jitter buffer. As a rule an
       audio codec needs a jitter buffer and all others do not.
      */
    PBoolean NeedsJitterBuffer() const { return needsJitter; }

    /**Get the average bandwidth used in bits/second.
      */
    unsigned GetBandwidth() const { return bandwidth; }

    /**Set the average bandwidth used in bits/second.
      */
    void SetBandwidth(unsigned newbandwidth) { bandwidth = newbandwidth; }

	/**Get the initial bandwidth set for the codec. Default is same as bandwidth
	  */
	virtual unsigned GetInitialBandwidth() const { return GetBandwidth(); }

    /**Get the maximum frame size in bytes. If this returns zero then the
       media format has no intrinsic maximum frame size, eg G.711 would 
       return zero but G.723.1 whoud return 24.
      */
    PINDEX GetFrameSize() const { return frameSize; }
	void SetFrameSize(PINDEX size) { frameSize = size; }

    /**Get the frame rate in RTP timestamp units. If this returns zero then
       the media format is not real time and has no intrinsic timing eg
      */
    unsigned GetFrameTime() const { return frameTime; }

	/**Set the frame rate in RTP timestamp units.
	  */
	void SetFrameTime(unsigned ft) { frameTime = ft; }

    /**Get the number of RTP timestamp units per millisecond.
      */
    virtual unsigned GetTimeUnits() const { return timeUnits; }
    virtual void SetTimeUnits(unsigned units) { timeUnits = units; }

    enum StandardTimeUnits {
      AudioTimeUnits = 8,  ///<  8kHz sample rate
      VideoTimeUnits = 90  ///<  90kHz sample rate
    };
 
    /**Get the list of media formats that have been registered.
      */
    static List GetRegisteredMediaFormats();
    static void GetRegisteredMediaFormats(List & list);

    friend class OpalStaticMediaFormat;

    /**This form of the constructor will register the full details of the
       media format into an internal database. This would typically be used
       as a static global. In fact it would be very dangerous for an instance
       to use this constructor in any other way, especially local variables.

       If the rtpPayloadType is RTP_DataFrame::DynamicBase, then the RTP
       payload type is actually set to teh first unused dynamic RTP payload
       type that is in the registers set of media formats.

       The frameSize parameter indicates that the media format has a maximum
       size for each data frame, eg G.723.1 frames are no more than 24 bytes
       long. If zero then there is no intrinsic maximum, eg G.711.
      */
    OpalMediaFormat(
      const char * fullName,  ///<  Full name of media format
      unsigned defaultSessionID,  ///<  Default session for codec type
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      PBoolean     needsJitter,   ///<  Indicate format requires a jitter buffer
      unsigned bandwidth,     ///<  Bandwidth in bits/second
      PINDEX   frameSize = 0, ///<  Size of frame in bytes (if applicable)
      unsigned frameTime = 0, ///<  Time for frame in RTP units (if applicable)
      unsigned timeUnits = 0, ///<  RTP units for frameTime (if applicable)
      time_t timeStamp = 0    ///<  timestamp (for versioning)

    );
    
    bool GetOptionValue(
      const PString & name,   ///<  Option name
      PString & value         ///<  String to receive option value
    ) const;

    /**Set the option value of the specified name as a string.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present.
      */
    bool SetOptionValue(
      const PString & name,   ///<  Option name
      const PString & value   ///<  New option value as string
    );

    /**Get the option value of the specified name as a boolean. The default
       value is returned if the option is not present.
      */
    bool GetOptionBoolean(
      const PString & name,   ///<  Option name
      bool dflt = FALSE       ///<  Default value if option not present
    ) const;

    /**Set the option value of the specified name as a boolean.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionBoolean(
      const PString & name,   ///<  Option name
      bool value              ///<  New value for option
    );

    /**Get the option value of the specified name as an integer. The default
       value is returned if the option is not present.
      */
    int GetOptionInteger(
      const PString & name,   ///<  Option name
      int dflt = 0            ///<  Default value if option not present
    ) const;

    /**Set the option value of the specified name as an integer.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present, not of the same type or
       is putside the allowable range.
      */
    bool SetOptionInteger(
      const PString & name,   ///<  Option name
      int value               ///<  New value for option
    );

    /**Get the option value of the specified name as a real. The default
       value is returned if the option is not present.
      */
    double GetOptionReal(
      const PString & name,   ///<  Option name
      double dflt = 0         ///<  Default value if option not present
    ) const;

    /**Set the option value of the specified name as a real.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionReal(
      const PString & name,   ///<  Option name
      double value            ///<  New value for option
    );

    /**Get the option value of the specified name as an index into an
       enumeration list. The default value is returned if the option is not
       present.
      */
    PINDEX GetOptionEnum(
      const PString & name,   ///<  Option name
      PINDEX dflt = 0         ///<  Default value if option not present
    ) const;

    /**Set the option value of the specified name as an index into an enumeration.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionEnum(
      const PString & name,   ///<  Option name
      PINDEX value            ///<  New value for option
    );

    /**Get the option value of the specified name as a string. The default
       value is returned if the option is not present.
      */
    PString GetOptionString(
      const PString & name,                   ///<  Option name
      const PString & dflt = PString::Empty() ///<  Default value if option not present
    ) const;

    /**Set the option value of the specified name as a string.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionString(
      const PString & name,   ///<  Option name
      const PString & value   ///<  New value for option
    );

    /**Get the option value of the specified name as an octet array.
       Returns FALSE if not present.
      */
    bool GetOptionOctets(
      const PString & name, ///<  Option name
      PBYTEArray & octets   ///<  Octets in option
    ) const;

    /**Set the option value of the specified name as an octet array.
       Note the option will not be added if it does not exist, the option
       must be explicitly added using AddOption().

       Returns false of the option is not present or is not of the same type.
      */
    bool SetOptionOctets(
      const PString & name,       ///<  Option name
      const PBYTEArray & octets   ///<  Octets in option
    );
    bool SetOptionOctets(
      const PString & name,       ///<  Option name
      const BYTE * data,          ///<  Octets in option
      PINDEX length               ///<  Number of octets
    );

    /**Set the options on the master format list entry.
       The media format must already be registered. Returns false if not.
      */
    static bool SetRegisteredMediaFormat(
      const OpalMediaFormat & mediaFormat  ///<  Media format to copy to master list
    );

    /**
      * Add a new option to this media format
      */
    bool AddOption(
      OpalMediaOption * option,
      PBoolean overwrite = FALSE
    );

    
    /** 
      * Remove all options
      */
    void RemoveAllOptions() 
    { options.RemoveAll(); }
    
    /**
      * Determine if media format has the specified option.
      */
    bool HasOption(const PString & name) const
    { return FindOption(name) != NULL; }

    /**
      * Get a pointer to the specified media format option.
      * Returns NULL if thee option does not exist.
      */
    OpalMediaOption * FindOption(
      const PString & name
    ) const;

	OpalMediaOption & GetOption(PINDEX i) const
	{ return options[i];  }

	PINDEX GetOptionCount() const
	{ return options.GetSize(); }

#if PTRACING
	static void DebugOptionList(const OpalMediaFormat & fmt);
#endif

  protected:
    RTP_DataFrame::PayloadTypes rtpPayloadType;
    unsigned defaultSessionID;
    PBoolean     needsJitter;
    unsigned bandwidth;
    PINDEX   frameSize;
    unsigned frameTime;
    unsigned timeUnits;
    PMutex   media_format_mutex;
    PSortedList<OpalMediaOption> options;
    time_t codecBaseTime;

};

#ifdef H323_VIDEO
class OpalVideoFormat : public OpalMediaFormat
{
  friend class OpalPluginCodecManager;
    PCLASSINFO(OpalVideoFormat, OpalMediaFormat);
  public:
    OpalVideoFormat(
      const char * fullName,    ///<  Full name of media format
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      unsigned frameWidth,      ///<  Width of video frame
      unsigned frameHeight,     ///<  Height of video frame
      unsigned frameRate,       ///<  Number of frames per second
      unsigned bitRate,         ///<  Maximum bits per second
      time_t timeStamp = 0        ///<  timestamp (for versioning)
    );

    virtual PObject * Clone() const;

	virtual unsigned GetInitialBandwidth() const;

    virtual bool Merge(const OpalMediaFormat & mediaFormat);

    static const char * const FrameWidthOption;
    static const char * const FrameHeightOption;
    static const char * const EncodingQualityOption;
    static const char * const TargetBitRateOption;
    static const char * const DynamicVideoQualityOption;
    static const char * const AdaptivePacketDelayOption;

    static const char * const NeedsJitterOption;
    static const char * const MaxBitRateOption;
    static const char * const MaxFrameSizeOption;
    static const char * const FrameTimeOption;
	static const char * const ClockRateOption;
    static const char * const EmphasisSpeedOption;
    static const char * const MaxPayloadSizeOption;

};
#endif
// List of known media formats

#define OPAL_PCM16         "PCM-16"
#define OPAL_G711_ULAW_64K "G.711-uLaw-64k"
#define OPAL_G711_ULAW_64K_20 "G.711-uLaw-64k-20"
#define OPAL_G711_ALAW_64K "G.711-ALaw-64k"
#define OPAL_G711_ALAW_64K_20 "G.711-ALaw-64k-20"
#define OPAL_G711_ULAW_56K "G.711-uLaw-56k"
#define OPAL_G711_ULAW_56K_20 "G.711-uLaw-56k-20"
#define OPAL_G711_ALAW_56K "G.711-ALaw-56k"
#define OPAL_G711_ALAW_56K_20 "G.711-ALaw-56k-20"
#define OPAL_G728          "G.728"
#define OPAL_G729          "G.729"
#define OPAL_G729A         "G.729A"
#define OPAL_G729B         "G.729B"
#define OPAL_G729AB        "G.729A/B"
#define OPAL_G7231         "G.723.1"
#define OPAL_G7231_6k3     OPAL_G7231
#define OPAL_G7231_5k3     "G.723.1(5.3k)"
#define OPAL_G7231A_6k3    "G.723.1A(6.3k)"
#define OPAL_G7231A_5k3    "G.723.1A(5.3k)"
#define OPAL_GSM0610       "GSM-06.10"

extern char OpalPCM16[];
extern char OpalG711uLaw64k[];
extern char OpalG711uLaw64k20[];
extern char OpalG711ALaw64k[];
extern char OpalG711ALaw64k20[];
extern char OpalG728[];
extern char OpalG729[];
extern char OpalG729A[];
extern char OpalG729B[];
extern char OpalG729AB[];
extern char OpalG7231_6k3[];
extern char OpalG7231_5k3[];
extern char OpalG7231A_6k3[];
extern char OpalG7231A_5k3[];
extern char OpalGSM0610[];

#define OpalG711uLaw      OpalG711uLaw64k
#define OpalG711ALaw      OpalG711ALaw64k
#define OpalG7231 OpalG7231_6k3

//
// Originally, the following inplace code was used instead of this macro:
//
// static PAbstractSingletonFactory<OpalMediaFormat, 
//     OpalStaticMediaFormatTemplate<
//          OpalPCM16,
//          OpalMediaFormat::DefaultAudioSessionID,
//          RTP_DataFrame::L16_Mono,
//          TRUE,   // Needs jitter
//          128000, // bits/sec
//          16, // bytes/frame
//          8, // 1 millisecond
//          OpalMediaFormat::AudioTimeUnits,
//          0
//     > 
// > opalPCM16Factory(OpalPCM16);
//
// This used the following macro:
//
//
//  template <
//        const char * _fullName,  /// Full name of media format
//        unsigned _defaultSessionID,  /// Default session for codec type
//        RTP_DataFrame::PayloadTypes _rtpPayloadType, /// RTP payload type code
//        PBoolean     _needsJitter,       /// Indicate format requires a jitter buffer
//        unsigned _bandwidth,         /// Bandwidth in bits/second
//        PINDEX   _frameSize,         /// Size of frame in bytes (if applicable)
//        unsigned _frameTime,         /// Time for frame in RTP units (if applicable)
//        unsigned _timeUnits,         /// RTP units for frameTime (if applicable)
//        time_t _timeStamp            /// timestamp (for versioning)
//  >
//  class OpalStaticMediaFormatTemplate : public OpalStaticMediaFormat
//  {
//    public:
//      OpalStaticMediaFormatTemplate()
//        : OpalStaticMediaFormat(_fullName, _defaultSessionID, _rtpPayloadType, _needsJitter, _bandwidth
//        , _frameSize, _frameTime, _timeUnits, _timeStamp )
//      { }
//  };
//
// Unfortauntely, MSVC 6 did not like this so this crappy macro has to be used instead of a template
//

typedef PFactory<OpalMediaFormat, std::string> OpalMediaFormatFactory;

#define OPAL_MEDIA_FORMAT_DECLARE(classname, _fullName, _defaultSessionID, _rtpPayloadType, _needsJitter,_bandwidth, _frameSize, _frameTime, _timeUnits, _timeStamp) \
class classname : public OpalMediaFormat \
{ \
  public: \
    classname() \
      : OpalMediaFormat(_fullName, _defaultSessionID, _rtpPayloadType, _needsJitter, _bandwidth, \
        _frameSize, _frameTime, _timeUnits, _timeStamp){} \
}; \
OpalMediaFormatFactory::Worker<classname> classname##Factory(_fullName, true); \


#endif  // __OPAL_MEDIAFMT_H


// End of File ///////////////////////////////////////////////////////////////
