/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2013 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __LFPDISPLAYNODE_H_D969A379__
#define __LFPDISPLAYNODE_H_D969A379__

#include <ProcessorHeaders.h>
#include "LfpDisplayEditor.h"

class DataViewport;

/**

  Holds data in a displayBuffer to be used by the LfpDisplayCanvas
  for rendering continuous data streams.

  @see GenericProcessor, LfpDisplayEditor, LfpDisplayCanvas

*/

class LfpDisplayNode :  public GenericProcessor

{
public:

    LfpDisplayNode();
    ~LfpDisplayNode();

    AudioProcessorEditor* createEditor();

    bool isSink()
    {
        return true;
    }

    void process(AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

    void setParameter(int, float);

    void updateSettings();

    bool enable();
    bool disable();

    void handleEvent(int, MidiMessage&, int);

    AudioSampleBuffer* getDisplayBufferAddress()
    {
        return displayBuffer;
    }
    int getDisplayBufferEndIndex(int chan)
    {
        return displayBufferEndIndex[chan];
    }

	CriticalSection* getMutex()
	{
		return &displayMutex;
	}
	int getDisplaySampleRate()
	{
		return displaySampleRate;
	}
	void fillDisplayBuffer(int channel, const float*source, int numSamples,float gain);
	void readDisplayBuffer(int channel, int requiredEndIndex, int maxCount, int *start1, int *size1, int *start2, int *size2);


private:

    void initializeEventChannels();

    ScopedPointer<AudioSampleBuffer> displayBuffer;
	int displaySampleRate;  //每秒保存的采样数，影响到能查看的最小显示单元
	int displayMaxSaveSeconds; //最大保持时间 秒
	Array<int> displayBufferStartIndex;
    Array<int> displayBufferEndIndex;  //每个channel的结束index
    Array<int> eventSourceNodes;
	Array<int> lastRemain;
    std::map<int, int> channelForEventSource;

    int numEventChannels;

    float displayGain; //
    //float bufferLength;

    //AbstractFifo abstractFifo;

    int64 bufferTimestamp;
    std::map<int, int> ttlState;
    HeapBlock<float> arrayOfOnes;
    int totalSamples;

    bool resizeBuffer();

	CriticalSection displayMutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfpDisplayNode);

};




#endif  // __LFPDISPLAYNODE_H_D969A379__
