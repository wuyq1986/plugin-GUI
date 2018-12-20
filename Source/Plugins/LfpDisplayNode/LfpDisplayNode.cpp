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

#include "LfpDisplayNode.h"
#include "LfpDisplayCanvas.h"
#include <stdio.h>

LfpDisplayNode::LfpDisplayNode()
    : GenericProcessor("LFP Viewer"),
	displayGain(1), /*bufferLength(5.0f), */
	/*abstractFifo(100), */displaySampleRate(10000), displayMaxSaveSeconds(20*60) //用来保存历史数据的 每秒保存10000个采样点，保存20分钟
{
    //std::cout << " LFPDisplayNodeConstructor" << std::endl;
    displayBuffer = new AudioSampleBuffer(8, 100);

	arrayOfOnes.malloc(5000);

    for (int n = 0; n < 5000; n++)
    {
        arrayOfOnes[n] = 1;
    }

}

LfpDisplayNode::~LfpDisplayNode()
{

}

AudioProcessorEditor* LfpDisplayNode::createEditor()
{

    editor = new LfpDisplayEditor(this, true);
    return editor;

}

void LfpDisplayNode::updateSettings()
{
    std::cout << "Setting num inputs on LfpDisplayNode to " << getNumInputs() << std::endl;

    channelForEventSource.clear();
    eventSourceNodes.clear();
    ttlState.clear();

    for (int i = 0; i < eventChannels.size(); i++)
    {
        if (!eventSourceNodes.contains(eventChannels[i]->sourceNodeId) && eventChannels[i]->type == EVENT_CHANNEL)
        {
            eventSourceNodes.add(eventChannels[i]->sourceNodeId);

        }
    }

    numEventChannels = eventSourceNodes.size();

    std::cout << "Found " << numEventChannels << " event channels." << std::endl;

    for (int i = 0; i < eventSourceNodes.size(); i++)
    {
        std::cout << "Adding channel " << getNumInputs() + i << " for event source node " << eventSourceNodes[i] << std::endl;
        channelForEventSource[eventSourceNodes[i]] = getNumInputs() + i;
        ttlState[eventSourceNodes[i]] = 0;
        Channel* eventChan = new Channel(this, getNumInputs() + i, EVENT_CHANNEL);
        eventChan->sourceNodeId = eventSourceNodes[i];
        channels.add(eventChan); // add a channel for event data for each source node
    }

    displayBufferStartIndex.clear();
    displayBufferStartIndex.insertMultiple(0, 0, getNumInputs() + numEventChannels);
	displayBufferEndIndex.clear();
	displayBufferEndIndex.insertMultiple(0, 0, getNumInputs() + numEventChannels);
	lastRemain.clear();
	lastRemain.insertMultiple(0, 0, getNumInputs() + numEventChannels);
	//wuyq

}

bool LfpDisplayNode::resizeBuffer()
{
	int nSamples = (int)displaySampleRate*displayMaxSaveSeconds;
    int nInputs = getNumInputs();

    std::cout << "Resizing buffer. Samples: " << nSamples << ", Inputs: " << nInputs << std::endl;

    if (nSamples > 0 && nInputs > 0)
    {
       // abstractFifo.setTotalSize(nSamples);
        displayBuffer->setSize(nInputs + numEventChannels, nSamples); // add extra channels for TTLs

        return true;
    }
    else
    {
        return false;
    }

}

bool LfpDisplayNode::enable()
{

    if (resizeBuffer())
    {
        LfpDisplayEditor* editor = (LfpDisplayEditor*) getEditor();
        editor->enable();
        return true;
    }
    else
    {
        return false;
    }

}

bool LfpDisplayNode::disable()
{
    LfpDisplayEditor* editor = (LfpDisplayEditor*) getEditor();
    editor->disable();
    return true;
}

void LfpDisplayNode::setParameter(int parameterIndex, float newValue)
{
    editor->updateParameterButtons(parameterIndex);
    //Sets Parameter in parameters array for processor
    Parameter* parameterPointer = parameters.getRawDataPointer();
    parameterPointer = parameterPointer+parameterIndex;
    parameterPointer->setValue(newValue, currentChannel);

    //std::cout << "Saving Parameter from " << currentChannel << ", channel ";

    LfpDisplayEditor* ed = (LfpDisplayEditor*) getEditor();
    if (ed->canvas != 0)
        ed->canvas->setParameter(parameterIndex, newValue);
}

void LfpDisplayNode::handleEvent(int eventType, MidiMessage& event, int sampleNum)
{
    if (eventType == TTL)
    {
        const uint8* dataptr = event.getRawData();

        //int eventNodeId = *(dataptr+1);
        int eventId = *(dataptr+2);
        int eventChannel = *(dataptr+3);
        int eventTime = event.getTimeStamp();

        int eventSourceNodeId = *(dataptr+5);

        int nSamples = numSamples.at(eventSourceNodeId);

        int samplesToFill = nSamples - eventTime;

        //	std::cout << "Received event from " << eventSourceNode << ", channel "
        //	          << eventChannel << ", with ID " << eventId << ", copying to "
         //            << channelForEventSource[eventSourceNode] << std::endl;
        ////
        int bufferIndex = (displayBufferEndIndex[channelForEventSource[eventSourceNodeId]] + eventTime - nSamples) % displayBuffer->getNumSamples();
        
        bufferIndex = bufferIndex >= 0 ? bufferIndex : displayBuffer->getNumSamples() + bufferIndex;


        if (eventId == 1)
        {
            ttlState[eventSourceNodeId] |= (1L << eventChannel);
        }
        else
        {
            ttlState[eventSourceNodeId] &= ~(1L << eventChannel);
        }
	
		
        if (samplesToFill + bufferIndex < displayBuffer->getNumSamples())
        {

            //std::cout << bufferIndex << " " << samplesToFill << " " << ttlState[eventSourceNode] << std::endl;

            displayBuffer->copyFrom(channelForEventSource[eventSourceNodeId],  // destChannel
                                    bufferIndex,		// destStartSample
                                    arrayOfOnes, 		// source
                                    samplesToFill, 		// numSamples
                                    float(ttlState[eventSourceNodeId]));   // gain
        }
        else
        {

            int block2Size = (samplesToFill + bufferIndex) % displayBuffer->getNumSamples();
            int block1Size = samplesToFill - block2Size;

            //std::cout << "OVERFLOW." << std::endl;

            //std::cout << bufferIndex << " " << block1Size << " " << ttlState << std::endl;

            displayBuffer->copyFrom(channelForEventSource[eventSourceNodeId],  // destChannel
                                    bufferIndex,		// destStartSample
                                    arrayOfOnes, 		// source
                                    block1Size, 		// numSamples
                                    float(ttlState[eventSourceNodeId]));   // gain
            //std::cout << 0 << " " << block2Size << " " << ttlState << std::endl;

            displayBuffer->copyFrom(channelForEventSource[eventSourceNodeId],  // destChannel
                                    0,		                        // destStartSample
                                    arrayOfOnes, 		// source
                                    block2Size, 		// numSamples
                                    float(ttlState[eventSourceNodeId]));   // gain


        }
		

        // 	std::cout << "ttlState: " << ttlState << std::endl;

        // std::cout << "Received event from " << eventNodeId <<
        //              " on channel " << eventChannel <<
        //             " with value " << eventId <<
        //             " at timestamp " << event.getTimeStamp() << std::endl;


    }

}

void LfpDisplayNode::initializeEventChannels()
{

    for (int i = 0; i < eventSourceNodes.size(); i++)
    {

        int chan = channelForEventSource[eventSourceNodes[i]];
        int index = displayBufferEndIndex[chan];

		fillDisplayBuffer(chan, arrayOfOnes, numSamples.at(eventSourceNodes[i]), float(ttlState[eventSourceNodes[i]]));
        //std::cout << "Event source node " << i << ", channel " << chan << std::endl;
		/*
        int samplesLeft = displayBuffer->getNumSamples() - index;

        int nSamples = numSamples.at(eventSourceNodes[i]);



        if (nSamples < samplesLeft)
        {

            //	std::cout << getNumInputs()+1 << " " << displayBufferIndex << " " << totalSamples << " " << ttlState << std::endl;
            //
            displayBuffer->copyFrom(chan,  // destChannel
                                    index,		// destStartSample
                                    arrayOfOnes, 		// source
                                    nSamples, 		// numSamples
                                    float(ttlState[eventSourceNodes[i]]));   // gain
            displayBufferIndex.set(chan, index + nSamples);
        }
        else
        {

            int extraSamples = nSamples - samplesLeft;

            // std::cout << "OVERFLOW." << std::endl;
            // std::cout << bufferIndex << " " << block1Size << " " << ttlState << std::endl;

            displayBuffer->copyFrom(chan,    // destChannel
                                    index,		// destStartSample
                                    arrayOfOnes, 		// source
                                    samplesLeft, 		// numSamples
                                    float(ttlState[eventSourceNodes[i]]));   // gain
            // std::cout << 0 << " " << block2Size << " " << ttlState << std::endl;
            displayBuffer->copyFrom(chan,  // destChannel
                                    0,		// destStartSample
                                    arrayOfOnes, 		// source
                                    extraSamples, 		// numSamples
                                    float(ttlState[eventSourceNodes[i]]));   // gain

            displayBufferIndex.set(chan, extraSamples);
        }
		*/
    }   
}

void LfpDisplayNode::process(AudioSampleBuffer& buffer, MidiBuffer& events)
{
    // 1. place any new samples into the displayBuffer
    //std::cout << "Display node sample count: " << nSamples << std::endl; ///buffer.getNumSamples() << std::endl;

    initializeEventChannels();

    checkForEvents(events); // see if we got any TTL events

	ScopedLock displayLock(displayMutex);

    for (int chan = 0; chan < buffer.getNumChannels(); chan++)
    {
         //int samplesLeft = displayBuffer->getNumSamples() - displayBufferEndIndex[chan];
         int nSamples = getNumSamples(chan);

		 fillDisplayBuffer(chan, buffer.getReadPointer(chan, 0), nSamples, 1);
		 /*
        if (nSamples < samplesLeft)
        {

            displayBuffer->copyFrom(chan,  			// destChannel
                                    displayBufferIndex[chan], // destStartSample
                                    buffer, 			// source
                                    chan, 				// source channel
                                    0,					// source start sample
                                    nSamples); 			// numSamples
        
            displayBufferIndex.set(chan, displayBufferIndex[chan] + nSamples);
        }
        else
        {

            int extraSamples = nSamples - samplesLeft;

			displayBuffer->copyFrom(chan,  				// destChannel
				displayBufferIndex[chan], // destStartSample
				buffer, 			// source
				chan, 				// source channel
				0,					// source start sample
				samplesLeft); 		// numSamples

			displayBuffer->copyFrom(chan,
				0,
				buffer,
				chan,
				samplesLeft,
				extraSamples);

            displayBufferIndex.set(chan, extraSamples);
        }
		*/
    }

}



void LfpDisplayNode::fillDisplayBuffer(int channel, const float*source, int numSamples, float gain)
{
	int maxSamplas = displayMaxSaveSeconds * displaySampleRate;
	int rate = getSampleRate() / displaySampleRate;  //比例  降低收集比例，增加保存时间
	int samples = floor(numSamples / rate);

	const float* startPointer = source;
	if (lastRemain[channel] > 0) {
		startPointer = startPointer + rate - lastRemain[channel];
		samples = floor((numSamples - rate + lastRemain[channel]) / rate);
	}

	int endIndex = displayBufferEndIndex[channel];
	if (endIndex + samples <= maxSamplas)
	{
		float *writePointer = displayBuffer->getWritePointer(channel, endIndex);
		for (int i = 0; i < samples; i++)
		{
			writePointer[i] = startPointer[3 * i] * gain;
			//writePointer[i] = Random().nextInt(1000) - 500;
			
		}
		displayBufferEndIndex.set(channel, endIndex + samples);
		if (endIndex < displayBufferStartIndex[channel])
		{
			displayBufferStartIndex.set(channel, endIndex + samples + 1);
		}
	}
	else
	{
		int size1 = maxSamplas - endIndex;
		if (size1 > 0)
		{
			float *writePointer = displayBuffer->getWritePointer(channel, endIndex);
			for (int i = 0; i < size1; i++)
			{
				writePointer[i] = startPointer[3 * i] * gain;
				//writePointer[i] = Random().nextInt(1000) - 500;
			}
		}
		
		int size2 = samples - size1;
		float *writePointer2 = displayBuffer->getWritePointer(channel, 0);
		for (int i = 0; i < size2; i++)
		{
			writePointer2[i] = startPointer[3 * (i + size1)] * gain;
			//writePointer[i] = Random().nextInt(1000) - 500;
		}
		displayBufferStartIndex.set(channel, size2 + 1);
		displayBufferEndIndex.set(channel, size2);
	}


	lastRemain.set(channel, (numSamples + rate - lastRemain[channel]) % rate);
}


void LfpDisplayNode::readDisplayBuffer(int channel, int requiredEndIndex, int maxCount, int *start1, int *size1, int *start2, int *size2)
{
	int startIndex = displayBufferStartIndex[channel];
	int endIndex = displayBufferEndIndex[channel];
	*start1 = 0;
	*size1 = 0;
	*start2 = 0;
	*size2 = 0;

	if (startIndex < endIndex) 
	{
		*start1 = requiredEndIndex - maxCount;
		if (*start1 < 0)
		{
			*start1 = 0;
		}
		*size1 = requiredEndIndex - *start1;
	}
	else if ( startIndex > endIndex)
	{
		int maxSamplas = displayMaxSaveSeconds * displaySampleRate;
		*size2 = requiredEndIndex;
		if (*size2 < maxCount) 
		{
			*size1 = maxCount - *size2;
			*start1 = maxSamplas - *size1;
		}
		else
		{
			*start2 = *size2 - maxCount;
		}
		
	}
}