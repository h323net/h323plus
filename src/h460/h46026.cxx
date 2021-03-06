//
// h46026.cxx
//
// Code automatically generated by asnparse.
//

#ifdef P_USE_PRAGMA
#pragma implementation "h46026.h"
#endif

#include <h323.h>
#include "h460/h46026.h"

#define new PNEW


#if ! H323_DISABLE_H46026


#ifndef PASN_NOPRINTON
const static PASN_Names Names_H46026_FrameData[]={
      {"rtp",0}
     ,{"rtcp",1}
};
#endif
//
// FrameData
//

H46026_FrameData::H46026_FrameData(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 2, FALSE
#ifndef PASN_NOPRINTON
    ,(const PASN_Names *)Names_H46026_FrameData,2
#endif
)
{
}


PBoolean H46026_FrameData::CreateObject()
{
  switch (tag) {
    case e_rtp :
      choice = new PASN_OctetString();
      choice->SetConstraints(PASN_Object::FixedConstraint, 12, 1500);
      return TRUE;
    case e_rtcp :
      choice = new PASN_OctetString();
      choice->SetConstraints(PASN_Object::FixedConstraint, 1, 1500);
      return TRUE;
  }

  choice = NULL;
  return FALSE;
}


PObject * H46026_FrameData::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H46026_FrameData::Class()), PInvalidCast);
#endif
  return new H46026_FrameData(*this);
}


//
// ArrayOf_FrameData
//

H46026_ArrayOf_FrameData::H46026_ArrayOf_FrameData(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Array(tag, tagClass)
{
}


PASN_Object * H46026_ArrayOf_FrameData::CreateObject() const
{
  return new H46026_FrameData;
}


H46026_FrameData & H46026_ArrayOf_FrameData::operator[](PINDEX i) const
{
  return (H46026_FrameData &)array[i];
}


PObject * H46026_ArrayOf_FrameData::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H46026_ArrayOf_FrameData::Class()), PInvalidCast);
#endif
  return new H46026_ArrayOf_FrameData(*this);
}


//
// UDPFrame
//

H46026_UDPFrame::H46026_UDPFrame(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Sequence(tag, tagClass, 0, FALSE, 0)
{
  m_sessionId.SetConstraints(PASN_Object::FixedConstraint, 0, 255);
}


#ifndef PASN_NOPRINTON
void H46026_UDPFrame::PrintOn(ostream & strm) const
{
  int indent = strm.precision() + 2;
  strm << "{\n";
  strm << setw(indent+12) << "sessionId = " << setprecision(indent) << m_sessionId << '\n';
  strm << setw(indent+12) << "dataFrame = " << setprecision(indent) << m_dataFrame << '\n';
  strm << setw(indent+8) << "frame = " << setprecision(indent) << m_frame << '\n';
  strm << setw(indent-1) << setprecision(indent-2) << "}";
}
#endif


PObject::Comparison H46026_UDPFrame::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H46026_UDPFrame), PInvalidCast);
#endif
  const H46026_UDPFrame & other = (const H46026_UDPFrame &)obj;

  Comparison result;

  if ((result = m_sessionId.Compare(other.m_sessionId)) != EqualTo)
    return result;
  if ((result = m_dataFrame.Compare(other.m_dataFrame)) != EqualTo)
    return result;
  if ((result = m_frame.Compare(other.m_frame)) != EqualTo)
    return result;

  return PASN_Sequence::Compare(other);
}


PINDEX H46026_UDPFrame::GetDataLength() const
{
  PINDEX length = 0;
  length += m_sessionId.GetObjectLength();
  length += m_dataFrame.GetObjectLength();
  length += m_frame.GetObjectLength();
  return length;
}


PBoolean H46026_UDPFrame::Decode(PASN_Stream & strm)
{
  if (!PreambleDecode(strm))
    return FALSE;

  if (!m_sessionId.Decode(strm))
    return FALSE;
  if (!m_dataFrame.Decode(strm))
    return FALSE;
  if (!m_frame.Decode(strm))
    return FALSE;

  return UnknownExtensionsDecode(strm);
}


void H46026_UDPFrame::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);

  m_sessionId.Encode(strm);
  m_dataFrame.Encode(strm);
  m_frame.Encode(strm);

  UnknownExtensionsEncode(strm);
}


PObject * H46026_UDPFrame::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(H46026_UDPFrame::Class()), PInvalidCast);
#endif
  return new H46026_UDPFrame(*this);
}


#endif // if ! H323_DISABLE_H46026


// End of h46026.cxx
