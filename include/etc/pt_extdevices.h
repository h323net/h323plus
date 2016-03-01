/*
* pt_extdevices.h
*
* Copyright (c) 2015 Spranto International Pte Ltd. All Rights Reserved.
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, you can obtain one at http://mozilla.org/MPL/2.0/
*
* Software distributed under the License is distributed on an "AS IS"
* basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
* the License for the specific language governing rights and limitations
* under the License.
*
* Contributor(s): ______________________________________.
*
*
*/


#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Call back

class H323_MediaCallBack : public PObject
{

};


////////////////////////////////////////////////////////////////////////////////////////////////////////
// SoundChannel

class PSoundChannel_External : public PSoundChannel
{
public:
    /**@name Construction */
    //@{
    /** Initialise with no device
    */
    PSoundChannel_External();

    /** Initialise and open device
    */
    PSoundChannel_External(const PString &device,
        PSoundChannel::Directions dir,
        unsigned numChannels,
        unsigned sampleRate,
        unsigned bitsPerSample);

    ~PSoundChannel_External();
    //@}

    /** Provides a list of detected devices human readable names
    Returns the names array of enumerated devices as PStringArray
    */
    static PStringArray GetDeviceNames(PSoundChannel::Directions);

    /** Open a device with format specifications
    Device name corresponds to Multimedia name (first 32 characters)
    Device is prepared for operation, but not activated yet (no I/O
    buffer - call SetBuffers for that).
    Or you can use PlaySound or PlayFile - they call SetBuffers themselves)
    */
    PBoolean Open(const PString & device,
        Directions dir,
        unsigned numChannels,
        unsigned sampleRate,
        unsigned bitsPerSample);

    PString GetName() const;

    PBoolean IsOpen() const;

    /** Stop I/O and destroy I/O buffer
    */
    PBoolean Abort();

    /** Destroy device
    */
    PBoolean Close();

    /** Change the audio format
    Resets I/O
    */
    PBoolean SetFormat(unsigned numChannels,
        unsigned sampleRate,
        unsigned bitsPerSample);

    unsigned GetChannels() const;
    unsigned GetSampleRate() const;
    unsigned GetSampleSize() const;

    /** Configure the device's transfer buffers.
    No audio can be played or captured until after this method is set!
    (PlaySound and PlayFile can be used though - they call here.)
    Read and write functions wait for input or space (blocking thread)
    in increments of buffer size.
    Best to make size the same as the len to be given to Read or Write.
    Best performance requires count of 4
    Resets I/O
    */
    PBoolean SetBuffers(PINDEX size, PINDEX count);
    PBoolean GetBuffers(PINDEX & size, PINDEX & count);

    /** Write specified number of bytes from buf to playback device
    Blocks thread until all bytes have been transferred to device
    */
    PBoolean Write(const void * buf, PINDEX len);

    PINDEX GetLastReadCount() const;

    /** Read specified number of bytes from capture device into buf
    Number of bytes actually read is a multiple of format frame size
    Blocks thread until number of bytes have been received
    */
    PBoolean Read(void * buf, PINDEX len);

    /** Resets I/O, changes audio format to match sound and configures the
    device's transfer buffers into one huge buffer, into which the entire
    sound is loaded and started playing.
    Returns immediately when wait is false, so you can do other stuff while
    sound plays.
    */
    PBoolean PlaySound(const PSound & sound, PBoolean wait);

    /** Resets I/O, changes audio format to match file and reconfigures the
    device's transfer buffers. Accepts .wav files. Plays audio from file in
    1/2 second chunks. Wait refers to waiting for completion of last chunk.
    */
    PBoolean PlayFile(const PFilePath & filename, PBoolean wait);

    /** Checks space available for writing audio to play.
    Returns true if space enough for one buffer as set by SetBuffers.
    Sets 'available' member for use by Write.
    */
    PBoolean IsPlayBufferFree();

    /** Repeatedly checks until there's space to fit buffer.
    Yields thread between checks.
    Loop can be ended by calling Abort()
    */
    PBoolean WaitForPlayBufferFree();

    // all below are untested

    PBoolean HasPlayCompleted();
    PBoolean WaitForPlayCompletion();

    PBoolean RecordSound(PSound & sound);
    PBoolean RecordFile(const PFilePath & filename);
    PBoolean StartRecording();
    PBoolean IsRecordBufferFull();
    PBoolean AreAllRecordBuffersFull();
    PBoolean WaitForRecordBufferFull();
    PBoolean WaitForAllRecordBuffersFull();

    PBoolean SetVolume(unsigned);
    PBoolean GetVolume(unsigned &);

private:

    H323_MediaCallBack*     m_mediaCallback;
    PBoolean                m_loaded;       // Audio Object loaded
    PBoolean                m_running;     // Audio Object running
    Directions              m_dir;
    PINDEX lastReadCount;

    PAdaptiveDelay sendwait;                     ///< Wait before sending the next block of data

};
