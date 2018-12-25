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

#include "LfpDisplayCanvas.h"

#include <math.h>

LfpDisplayCanvas::LfpDisplayCanvas(LfpDisplayNode* processor_) :
     timebase(1.0f), displayGain(1.0f),   timeOffset(0.0f),
    processor(processor_), selectedChannelType(HEADSTAGE_CHANNEL)
{

    nChans = processor->getNumInputs();
    std::cout << "Setting num inputs on LfpDisplayCanvas to " << nChans << std::endl;

    displayBuffer = processor->getDisplayBufferAddress();
    displayBufferSize = displayBuffer->getNumSamples();
    std::cout << "Setting displayBufferSize on LfpDisplayCanvas to " << displayBufferSize << std::endl;

	historySeconds = floor(maxHistorySamples / (nChans+1));
	if (historySeconds > maxHistorySeconds)
	{
		historySeconds = maxHistorySeconds;
	}
	historySamples = historySeconds * historySampleRate;
	screenBuffer = new AudioSampleBuffer((nChans + 1), historySamples);
    screenBuffer->clear();

    viewport = new LfpViewport(this);
    lfpDisplay = new LfpDisplay(this, viewport);
    timescale = new LfpTimescale(this);

    timescale->setTimebase(timebase);

    viewport->setViewedComponent(lfpDisplay, false);
    viewport->setScrollBarsShown(true, false);

    scrollBarThickness = viewport->getScrollBarThickness();

    isChannelEnabled.insertMultiple(0,true,10000); // max 10k channels

    //viewport->getVerticalScrollBar()->addListener(this->scrollBarMoved(viewport->getVerticalScrollBar(), 1.0));

    UtilityButton* tbut;

    addAndMakeVisible(viewport);
    addAndMakeVisible(timescale);

    //Ranges for neural data
     voltageRanges[HEADSTAGE_CHANNEL].add("25");
    voltageRanges[HEADSTAGE_CHANNEL].add("50");
    voltageRanges[HEADSTAGE_CHANNEL].add("100");
    voltageRanges[HEADSTAGE_CHANNEL].add("250");
    voltageRanges[HEADSTAGE_CHANNEL].add("400");
    voltageRanges[HEADSTAGE_CHANNEL].add("500");
    voltageRanges[HEADSTAGE_CHANNEL].add("750");
    voltageRanges[HEADSTAGE_CHANNEL].add("1000");
    voltageRanges[HEADSTAGE_CHANNEL].add("2000");
    voltageRanges[HEADSTAGE_CHANNEL].add("5000");
    voltageRanges[HEADSTAGE_CHANNEL].add("10000");
    selectedVoltageRange[HEADSTAGE_CHANNEL] = 8;
    rangeGain[HEADSTAGE_CHANNEL] = 1; //uV
    rangeSteps[HEADSTAGE_CHANNEL] = 10;
    rangeUnits.add("uV");
    typeNames.add("DATA");

    tbut = new UtilityButton("DATA",Font("Small Text", 9, Font::plain));
    tbut->setEnabledState(true);
    tbut->setCorners(false,false,false,false);
    tbut->addListener(this);
    tbut->setClickingTogglesState(true);
    tbut->setRadioGroupId(100,dontSendNotification);
    tbut->setToggleState(true,dontSendNotification);
    addAndMakeVisible(tbut);
    typeButtons.add(tbut);
    
    //Ranges for AUX/accelerometer data
    voltageRanges[AUX_CHANNEL].add("25");
    voltageRanges[AUX_CHANNEL].add("50");
    voltageRanges[AUX_CHANNEL].add("100");
    voltageRanges[AUX_CHANNEL].add("250");
    voltageRanges[AUX_CHANNEL].add("400");
    voltageRanges[AUX_CHANNEL].add("500");
    voltageRanges[AUX_CHANNEL].add("750");
    voltageRanges[AUX_CHANNEL].add("1000");
    voltageRanges[AUX_CHANNEL].add("2000");
    //voltageRanges[AUX_CHANNEL].add("5000");
    selectedVoltageRange[AUX_CHANNEL] = 9;
    rangeGain[AUX_CHANNEL] = 0.001; //mV
    rangeSteps[AUX_CHANNEL] = 10;
    rangeUnits.add("mV");
    typeNames.add("AUX");

    tbut = new UtilityButton("AUX",Font("Small Text", 9, Font::plain));
    tbut->setEnabledState(true);
    tbut->setCorners(false,false,false,false);
    tbut->addListener(this);
    tbut->setClickingTogglesState(true);
    tbut->setRadioGroupId(100,dontSendNotification);
    tbut->setToggleState(false,dontSendNotification);
    addAndMakeVisible(tbut);
    typeButtons.add(tbut);

    //Ranges for ADC data
     voltageRanges[ADC_CHANNEL].add("0.01");
    voltageRanges[ADC_CHANNEL].add("0.05");
    voltageRanges[ADC_CHANNEL].add("0.1");
    voltageRanges[ADC_CHANNEL].add("0.5");
    voltageRanges[ADC_CHANNEL].add("1.0");
    voltageRanges[ADC_CHANNEL].add("2.0");
    voltageRanges[ADC_CHANNEL].add("5.0");
    voltageRanges[ADC_CHANNEL].add("10.0");
    selectedVoltageRange[ADC_CHANNEL] = 8;
    rangeGain[ADC_CHANNEL] = 1; //V
    rangeSteps[ADC_CHANNEL] = 0.1; //in V
    rangeUnits.add("V");
    typeNames.add("ADC");

    tbut = new UtilityButton("ADC",Font("Small Text", 9, Font::plain));
    tbut->setEnabledState(true);
    tbut->setCorners(false,false,false,false);
    tbut->addListener(this);
    tbut->setClickingTogglesState(true);
    tbut->setRadioGroupId(100,dontSendNotification);
    tbut->setToggleState(false,dontSendNotification);
    addAndMakeVisible(tbut);
    typeButtons.add(tbut);

    selectedVoltageRangeValues[HEADSTAGE_CHANNEL] = voltageRanges[HEADSTAGE_CHANNEL][selectedVoltageRange[HEADSTAGE_CHANNEL]-1];
    selectedVoltageRangeValues[AUX_CHANNEL] = voltageRanges[AUX_CHANNEL][selectedVoltageRange[AUX_CHANNEL]-1];
    selectedVoltageRangeValues[ADC_CHANNEL] = voltageRanges[ADC_CHANNEL][selectedVoltageRange[ADC_CHANNEL]-1];

	timebases.add("0.001");
	timebases.add("0.01");
    timebases.add("0.25");
    timebases.add("0.5");
    timebases.add("1.0");
    timebases.add("2.0");
    timebases.add("3.0");
    timebases.add("4.0");
    timebases.add("5.0");
    timebases.add("10.0");
    timebases.add("20.0");
    selectedTimebase = 6;
    selectedTimebaseValue = timebases[selectedTimebase-1];

    spreads.add("10");
    spreads.add("20");
    spreads.add("30");
    spreads.add("40");
    spreads.add("50");
    spreads.add("60");
    spreads.add("70");
    spreads.add("80");
    spreads.add("90");
    spreads.add("100");
    selectedSpread = 5;
    selectedSpreadValue = spreads[selectedSpread-1];

    colorGroupings.add("1");
    colorGroupings.add("2");
    colorGroupings.add("4");
    colorGroupings.add("8");
    colorGroupings.add("16");


    rangeSelection = new ComboBox("Voltage range");
    rangeSelection->addItemList(voltageRanges[HEADSTAGE_CHANNEL], 1);
    rangeSelection->setSelectedId(selectedVoltageRange[HEADSTAGE_CHANNEL], sendNotification);
    rangeSelection->setEditableText(true);
    rangeSelection->addListener(this);
    addAndMakeVisible(rangeSelection);


    timebaseSelection = new ComboBox("Timebase");
    timebaseSelection->addItemList(timebases, 1);
    timebaseSelection->setSelectedId(selectedTimebase, sendNotification);
    timebaseSelection->setEditableText(true);
    timebaseSelection->addListener(this);
    addAndMakeVisible(timebaseSelection);


    spreadSelection = new ComboBox("Spread");
    spreadSelection->addItemList(spreads, 1);
    spreadSelection->setSelectedId(selectedSpread,sendNotification);
    spreadSelection->addListener(this);
    spreadSelection->setEditableText(true);
    addAndMakeVisible(spreadSelection);

    colorGroupingSelection = new ComboBox("Color Grouping");
    colorGroupingSelection->addItemList(colorGroupings, 1);
    colorGroupingSelection->setSelectedId(1,sendNotification);
    colorGroupingSelection->addListener(this);
    addAndMakeVisible(colorGroupingSelection);

    invertInputButton = new UtilityButton("Invert", Font("Small Text", 13, Font::plain));
    invertInputButton->setRadius(5.0f);
    invertInputButton->setEnabledState(true);
    invertInputButton->setCorners(true, true, true, true);
    invertInputButton->addListener(this);
    invertInputButton->setClickingTogglesState(true);
    invertInputButton->setToggleState(false, sendNotification);
    //addAndMakeVisible(invertInputButton);

    //button for controlling drawing algorithm - old line-style or new per-pixel style
    drawMethodButton = new UtilityButton("DrawMethod", Font("Small Text", 13, Font::plain));
    drawMethodButton->setRadius(5.0f);
    drawMethodButton->setEnabledState(true);
    drawMethodButton->setCorners(true, true, true, true);
    drawMethodButton->addListener(this);
    drawMethodButton->setClickingTogglesState(true);
    drawMethodButton->setToggleState(false, sendNotification);
    //addAndMakeVisible(drawMethodButton);

    //button for pausing the display - works by skipping buffer updates. This way scrolling etc still works
    pauseButton = new UtilityButton("Pause", Font("Small Text", 13, Font::plain));
    pauseButton->setRadius(5.0f);
    pauseButton->setEnabledState(true);
    pauseButton->setCorners(true, true, true, true);
    pauseButton->addListener(this);
    pauseButton->setClickingTogglesState(true);
    pauseButton->setToggleState(false, sendNotification);
    //addAndMakeVisible(pauseButton);


    lfpDisplay->setNumChannels(nChans);
    lfpDisplay->setRange(voltageRanges[HEADSTAGE_CHANNEL][selectedVoltageRange[HEADSTAGE_CHANNEL]-1].getFloatValue()*rangeGain[HEADSTAGE_CHANNEL]
        ,HEADSTAGE_CHANNEL);
	lfpDisplay->setRange(voltageRanges[ADC_CHANNEL][selectedVoltageRange[ADC_CHANNEL] - 1].getFloatValue()*rangeGain[ADC_CHANNEL]
		, ADC_CHANNEL);
	lfpDisplay->setRange(voltageRanges[AUX_CHANNEL][selectedVoltageRange[AUX_CHANNEL] - 1].getFloatValue()*rangeGain[AUX_CHANNEL]
		, AUX_CHANNEL);

    // add event display-specific controls (currently just an enable/disable button)
    for (int i = 0; i < 8; i++)
    {

        EventDisplayInterface* eventOptions = new EventDisplayInterface(lfpDisplay, this, i);
        eventDisplayInterfaces.add(eventOptions);
        addAndMakeVisible(eventOptions);
        eventOptions->setBounds(500+(floor(i/2)*20), getHeight()-20-(i%2)*20, 40, 20);

        lfpDisplay->setEventDisplayState(i,true);

    }

    TopLevelWindow::getTopLevelWindow(0)->addKeyListener(this);

	
	xFangda = new ActionButton(this, "Xfangda.png");
	addAndMakeVisible(xFangda);
	xFangda->addListener(this);

	xSuoxiao = new ActionButton(this, "Xsuoxiao.png");
	addAndMakeVisible(xSuoxiao);
	xSuoxiao->addListener(this);

	yFangda = new ActionButton(this, "Yfangda.png");
	addAndMakeVisible(yFangda);
	yFangda->addListener(this);
	yFangda->setVisible(false);

	ySuoxiao = new ActionButton(this, "Ysuoxiao.png");
	addAndMakeVisible(ySuoxiao);
	ySuoxiao->addListener(this);
	ySuoxiao->setVisible(false);

	for (int i = 0; i < CHANNEL_TYPES; i++)
	{
		customRange[i] = Range<float>(0, 0);
	}
}

LfpDisplayCanvas::~LfpDisplayCanvas()
{

    deleteAndZero(screenBuffer);

    TopLevelWindow::getTopLevelWindow(0)->removeKeyListener(this);
}

void LfpDisplayCanvas::resized()
{

    timescale->setBounds(leftmargin,0,getWidth()-scrollBarThickness-leftmargin,30);
    viewport->setBounds(0,30,getWidth(),getHeight()-90);

    if (lfpDisplay->getSingleChannelState())
        lfpDisplay->setChannelHeight(viewport->getHeight(),false);

    lfpDisplay->setBounds(0,0,getWidth()-scrollBarThickness, lfpDisplay->getChannelHeight()*nChans);

    rangeSelection->setBounds(5,getHeight()-30,100,25);
    timebaseSelection->setBounds(175,getHeight()-30,100,25);
    spreadSelection->setBounds(345,getHeight()-30,100,25);
    colorGroupingSelection->setBounds(620,getHeight()-30,100,25);

    //invertInputButton->setBounds(750,getHeight()-50,100,22);
    //drawMethodButton->setBounds(750,getHeight()-25,100,22);
    //pauseButton->setBounds(880,getHeight()-50,50,44);
	xFangda->setBounds(750, getHeight() - 40, 30, 30);
	xSuoxiao->setBounds(785, getHeight() - 40, 30, 30);
	yFangda->setBounds(820, getHeight() - 40, 30, 30);
	ySuoxiao->setBounds(855, getHeight() - 40, 30, 30);

    for (int i = 0; i < 8; i++)
    {
        eventDisplayInterfaces[i]->setBounds(500+(floor(i/2)*20), getHeight()-40+(i%2)*20, 40, 20); // arrange event channel buttons in two rows
        eventDisplayInterfaces[i]->repaint();
    }

    int bh = 25/typeButtons.size();
    for (int i = 0; i < typeButtons.size(); i++)
    {
        typeButtons[i]->setBounds(110,getHeight()-30+i*bh,50,bh);
    }
    // std::cout << "Canvas thinks LfpDisplay should be this high: "
    //  << lfpDisplay->getTotalHeight() << std::endl;

}

void LfpDisplayCanvas::beginAnimation()
{
    std::cout << "Beginning animation." << std::endl;

    displayBufferSize = displayBuffer->getNumSamples();

    startCallbacks();
}

void LfpDisplayCanvas::endAnimation()
{
    std::cout << "Ending animation." << std::endl;

    stopCallbacks();
}

void LfpDisplayCanvas::update()
{
    nChans = jmax(processor->getNumInputs(),1);

    sampleRate.clear();
    screenBufferStartIndex.clear();
	screenBufferEndIndex.clear();
    displayBufferIndex.clear();

    for (int i = 0; i <= nChans; i++) // extra channel for events
    {
		if (processor->getNumInputs() > 0)
		{
			if (i < nChans)
				sampleRate.add(processor->channels[i]->sampleRate);
			else
				sampleRate.add(processor->channels[i - 1]->sampleRate); // for event channel (IT'S A HACK -- BE CAREFUL!)
		}
		else
			sampleRate.add(30000);
        
       // std::cout << "Sample rate for ch " << i << " = " << sampleRate[i] << std::endl; 
        displayBufferIndex.add(0);
        screenBufferStartIndex.add(0);
		screenBufferEndIndex.add(0);
    }



    if (nChans != lfpDisplay->getNumChannels())
    {
        //std::cout << "Setting num inputs on LfpDisplayCanvas to " << nChans << std::endl;

        refreshScreenBuffer();

        lfpDisplay->setNumChannels(nChans); // add an extra channel for events

        // update channel names
        for (int i = 0; i < processor->getNumInputs(); i++)
        {

            String chName = processor->channels[i]->getName();

            //std::cout << chName << std::endl;

            lfpDisplay->channelInfo[i]->setName(chName);
            lfpDisplay->enableChannel(isChannelEnabled[i], i);

        }

        lfpDisplay->setBounds(0,0,getWidth()-scrollBarThickness*2, lfpDisplay->getTotalHeight());

        resized();
    }
    else
    {
        for (int i = 0; i < processor->getNumInputs(); i++)
        {
            lfpDisplay->channels[i]->updateType();
            lfpDisplay->channelInfo[i]->updateType();
        }
        
    }

}

void LfpDisplayCanvas::buttonClicked(Button* b)
{
    if (b == invertInputButton)
    {
        lfpDisplay->setInputInverted(b->getToggleState());
        return;
    }
    if (b == drawMethodButton)
    {
        lfpDisplay->setDrawMethod(b->getToggleState());
        return;
    }
    if (b == pauseButton)
    {
        lfpDisplay->isPaused = b->getToggleState();
        return;
	}
	else if (b == xFangda)
	{
		//放大尺寸
		/*customTimebase = customTimebase == -1 ? timebase* 0.1f : customTimebase * 0.1f;
		timebaseSelection->setText(String(customTimebase, 2), dontSendNotification);
		timescale->setTimebase(customTimebase);*/
		//xFangda->setColour(,Colours::yellow);
		buttonState = 1;
		xFangda->setState(1);
		yFangda->setState(-1);
		return;
	}
	else if (b == xSuoxiao)
	{
		//缩小到X轴  //还是缩小到原来的尺寸?
		//customTimebase = customTimebase == -1 ? timebase* 10.0f : customTimebase * 10.0f;
		setCustomTimeBase(timebase);
		buttonState = -1;
		xFangda->setState(-1);
		yFangda->setState(-1);
		return;
	}
	else if (b == yFangda)
	{
		//放大尺寸
		buttonState = 2;
		yFangda->setState(1);
		xFangda->setState(-1);
		return;
	}
	else if (b == ySuoxiao)
	{
		//缩小到原来的尺寸
		//TODO
		setCustomVoltageRange(lfpDisplay->getSelectedChannelType(), 0, 0);
		buttonState = -1;
		xFangda->setState(-1);
		yFangda->setState(-1);
		viewport->repaint();
		return;
	}

    int idx = typeButtons.indexOf((UtilityButton*)b);
    if ((idx >= 0) && (b->getToggleState()))
    {
        for (int i = 0; i < processor->getNumInputs(); i++)
        {
            if (lfpDisplay->channels[i]->getSelected())
            {
                lfpDisplay->channels[i]->deselect();
                lfpDisplay->channels[i]->repaint();
            }
        }  
        setSelectedType((ChannelType) idx, false);
    }

}

void LfpDisplayCanvas::setCustomVoltageRange(int type, float start, float end)
{
	customRange[type] = Range<float>(start, end);
}

void LfpDisplayCanvas::comboBoxChanged(ComboBox* cb)
{

    if (cb == timebaseSelection)
    {
		customTimebase = -1;
        if (cb->getSelectedId())
        {
            timebase = timebases[cb->getSelectedId()-1].getFloatValue();
        }
        else
        {
            timebase = cb->getText().getFloatValue();
            if (timebase)
            {
                if (timebase < timebases[0].getFloatValue())
                {
                    cb->setSelectedId(1,dontSendNotification);
                    timebase = timebases[0].getFloatValue();
                }
                /*else if (timebase > timebases[timebases.size()-1].getFloatValue())
                {
                    cb->setSelectedId(timebases.size(),dontSendNotification);
                    timebase = timebases[timebases.size()-1].getFloatValue();
                }*/
				else if (timebase > historySeconds)
				{
					cb->setText(String(historySeconds), dontSendNotification);
				}
                else
                    cb->setText(String(timebase,2),dontSendNotification);
            }
            else
            {
                if (selectedSpread == 0)
                {
                    cb->setText(selectedTimebaseValue,dontSendNotification);
                    timebase = selectedTimebaseValue.getFloatValue();
                }
                else
                {
                    cb->setSelectedId(selectedTimebase,dontSendNotification);
                    timebase = timebases[selectedTimebase-1].getFloatValue();
                }

            }
        }
    }
    else if (cb == rangeSelection)
    {
        if (cb->getSelectedId())
        {
        lfpDisplay->setRange(voltageRanges[selectedChannelType][cb->getSelectedId()-1].getFloatValue()*rangeGain[selectedChannelType]
            ,selectedChannelType);
        }
        else
        {
            float vRange = cb->getText().getFloatValue();
            if (vRange)
            {
                if (vRange < voltageRanges[selectedChannelType][0].getFloatValue())
                {
                    cb->setSelectedId(1,dontSendNotification);
                    vRange = voltageRanges[selectedChannelType][0].getFloatValue();
                }
                else if (vRange > voltageRanges[selectedChannelType][voltageRanges[selectedChannelType].size()-1].getFloatValue())
                {
                    cb->setSelectedId(voltageRanges[selectedChannelType].size(),dontSendNotification);
                    vRange = voltageRanges[selectedChannelType][voltageRanges[selectedChannelType].size()-1].getFloatValue();
                }
                else
                {
                    if (rangeGain[selectedChannelType] > 1)
                        cb->setText(String(vRange,1),dontSendNotification);
                    else
                        cb->setText(String(vRange),dontSendNotification);
                }
                lfpDisplay->setRange(vRange*rangeGain[selectedChannelType],selectedChannelType);
            }
            else
            {
                if (selectedVoltageRange[selectedChannelType])
                    cb->setText(selectedVoltageRangeValues[selectedChannelType],dontSendNotification);
                else
                    cb->setSelectedId(selectedVoltageRange[selectedChannelType],dontSendNotification);
            }
        }
        selectedVoltageRange[selectedChannelType] = cb->getSelectedId();
        selectedVoltageRangeValues[selectedChannelType] = cb->getText();
        //std::cout << "Setting range to " << voltageRanges[cb->getSelectedId()-1].getFloatValue() << std::endl;
    }
    else if (cb == spreadSelection)
    {
        if (cb->getSelectedId())
        {
            lfpDisplay->setChannelHeight(spreads[cb->getSelectedId()-1].getIntValue());
            resized();
        }
        else
        {
            int spread = cb->getText().getIntValue();
            if (spread)
            {
                if (spread < spreads[0].getFloatValue())
                {
                    cb->setSelectedId(1,dontSendNotification);
                    spread = spreads[0].getFloatValue();
                }
                else if (spread > spreads[spreads.size()-1].getFloatValue())
                {
                    cb->setSelectedId(spreads.size(),dontSendNotification);
                    spread = spreads[spreads.size()-1].getFloatValue();
                }
                else
                {
                    cb->setText(String(spread),dontSendNotification);
                }
                lfpDisplay->setChannelHeight(spread);
                resized();
            }
            else
            {
                if (selectedSpread == 0)
                    cb->setText(selectedSpreadValue,dontSendNotification);
                else
                    cb->setSelectedId(selectedSpread,dontSendNotification);
            }
        }
        selectedSpread = cb->getSelectedId();
        selectedSpreadValue = cb->getText();
        //std::cout << "Setting spread to " << spreads[cb->getSelectedId()-1].getFloatValue() << std::endl;
    }
    else if (cb == colorGroupingSelection)
    {
        // set color grouping hre

        lfpDisplay->setColorGrouping(colorGroupings[cb->getSelectedId()-1].getIntValue());// so that channel colors get re-assigned

    }

    timescale->setTimebase(timebase);
}

int LfpDisplayCanvas::getChannelHeight()
{
    //return spreads[spreadSelection->getSelectedId()-1].getIntValue(); // is not set correctly after the mouse wheel was used to change hieghts
    return (int)spreadSelection->getText().getIntValue(); // instead, get value from textbox directly
}

void LfpDisplayCanvas::setParameter(int param, float val)
{
    // not used for anything, since LfpDisplayCanvas is not a processor
}

void LfpDisplayCanvas:: setRangeSelection(float range, bool canvasMustUpdate)
{
    if (canvasMustUpdate)
    {
        rangeSelection->setText(String(range/rangeGain[selectedChannelType]), sendNotification); 
    }
    else
    {
        rangeSelection->setText(String(range/rangeGain[selectedChannelType]),dontSendNotification);
        selectedVoltageRange[selectedChannelType]=rangeSelection->getSelectedId();
        selectedVoltageRangeValues[selectedChannelType]=rangeSelection->getText();

        repaint();
        refresh();
    }

}

void LfpDisplayCanvas:: setSpreadSelection(int spread, bool canvasMustUpdate)
{
    if (canvasMustUpdate)
    {
        spreadSelection->setText(String(spread),sendNotification);
    }
    else
    {
        spreadSelection->setText(String(spread),dontSendNotification);
        selectedSpread=spreadSelection->getSelectedId();
        selectedSpreadValue=spreadSelection->getText();

        repaint();
        refresh();
    }
}

void LfpDisplayCanvas::refreshState()
{
    // called when the component's tab becomes visible again

    for (int i = 0; i <= displayBufferIndex.size(); i++) // include event channel
    {

        displayBufferIndex.set(i, processor->getDisplayBufferIndex(i));
        screenBufferStartIndex.set(i,0);
		screenBufferEndIndex.set(i, 0);
    }

}

void LfpDisplayCanvas::refreshScreenBuffer()
{
    screenBuffer->clear();
	historySeconds = floor(maxHistorySamples / (nChans + 1));
	if (historySeconds > maxHistorySeconds)
	{
		historySeconds = maxHistorySeconds;
	}
	historySamples = historySeconds * historySampleRate;
	screenBuffer->setSize((nChans + 1), historySamples);
	screenBufferStartIndex.clear();
	screenBufferStartIndex.insertMultiple(0, 0, (nChans + 1));

	screenBufferEndIndex.clear();
	screenBufferEndIndex.insertMultiple(0, 0, (nChans + 1));
    

}

void LfpDisplayCanvas::updateScreenBuffer()
{

    // copy new samples from the displayBuffer into the screenBuffer
	int rate = processor->getSampleRate() / historySampleRate;  //比例  降低收集比例，增加保存时间

	ScopedLock displayLock(*processor->getMutex());

    for (int channel = 0; channel <= nChans; channel++) // pull one extra channel for event display
    {
		int from = displayBufferIndex[channel];
		int to = processor->getDisplayBufferIndex(channel);
		int nSamples = to - from;
		if (nSamples < 0)
		{
			nSamples = (displayBufferSize - from) + to;
		}
		int needSamples = floor(nSamples / rate);

		if (needSamples > 1)
		{
			int sourceIndex = displayBufferIndex[channel];
			int oriEndIndex = screenBufferEndIndex[channel];
			int endIndex = screenBufferEndIndex[channel];
			bool needResetStartIndex = false;
			for (int i = 1; i <= needSamples; i++)
			{
				sourceIndex = (sourceIndex + rate) % displayBufferSize;
				endIndex++;
				if (endIndex >= historySamples)
				{
					needResetStartIndex = true;
					endIndex = 0;
				}
				screenBuffer->copyFrom(channel, 
					endIndex,
					displayBuffer->getReadPointer(channel, sourceIndex),
					1);
			}
			displayBufferIndex.set(channel, sourceIndex);
			screenBufferEndIndex.set(channel, endIndex);

			int startIndex = screenBufferStartIndex[channel];
			if (needResetStartIndex || oriEndIndex < startIndex)
			{
				screenBufferStartIndex.set(channel, (endIndex + 1) % historySamples);
			}
		}
    }

}
/*
const float LfpDisplayCanvas::getXCoord(int chan, int samp)
{
    return samp;
}*/

int LfpDisplayCanvas::getNumChannels()
{
    return nChans;
}
/*
const float LfpDisplayCanvas::getYCoord(int chan, int samp)
{
    return *screenBuffer->getReadPointer(chan, samp);
}


const float LfpDisplayCanvas::getYCoordMean(int chan, int samp)
{
    return *screenBufferMean->getReadPointer(chan, samp);
}
const float LfpDisplayCanvas::getYCoordMin(int chan, int samp)
{
    return *screenBufferMin->getReadPointer(chan, samp);
}
const float LfpDisplayCanvas::getYCoordMax(int chan, int samp)
{
    return *screenBufferMax->getReadPointer(chan, samp);
}*/


bool LfpDisplayCanvas::getInputInvertedState()
{
    return invertInputButton->getToggleState();
}

bool LfpDisplayCanvas::getDrawMethodState()
{
    return drawMethodButton->getToggleState();
}

void LfpDisplayCanvas::paint(Graphics& g)
{

    //std::cout << "Painting" << std::endl;
    g.setColour(Colour(0,18,43)); //background color
    g.fillRect(0, 0, getWidth(), getHeight());

    g.setGradientFill(ColourGradient(Colour(50,50,50),0,0,
                                     Colour(25,25,25),0,30,
                                     false));

    g.fillRect(0, 0, getWidth()-scrollBarThickness, 30);

    g.setColour(Colours::black);

    g.drawLine(0,30,getWidth()-scrollBarThickness,30);

    g.setColour(Colour(25,25,60)); // timing grid color

    int w = getWidth()-scrollBarThickness-leftmargin;

    for (int i = 0; i < 10; i++)
    {
        if (i == 5 || i == 0)
            g.drawLine(w/10*i+leftmargin,0,w/10*i+leftmargin,getHeight()-60,3.0f);
        else
            g.drawLine(w/10*i+leftmargin,0,w/10*i+leftmargin,getHeight()-60,1.0f);
    }

    g.drawLine(0,getHeight()-60,getWidth(),getHeight()-60,3.0f);

    g.setFont(Font("Default", 16, Font::plain));

    g.setColour(Colour(100,100,100));

    g.drawText("Voltage range ("+ rangeUnits[selectedChannelType] +")",5,getHeight()-55,300,20,Justification::left, false);
    g.drawText("Timebase (s)",175,getHeight()-55,300,20,Justification::left, false);
    g.drawText("Spread (px)",345,getHeight()-55,300,20,Justification::left, false);
    g.drawText("Color grouping",620,getHeight()-55,300,20,Justification::left, false);

    //g.drawText(typeNames[selectedChannelType],110,getHeight()-30,50,20,Justification::centredLeft,false);

    g.drawText("Event disp.",500,getHeight()-55,300,20,Justification::left, false);



}

void LfpDisplayCanvas::refresh()
{
    updateScreenBuffer();

    lfpDisplay->refresh(); // redraws only the new part of the screen buffer

    //getPeer()->performAnyPendingRepaintsNow();

}

bool LfpDisplayCanvas::keyPressed(const KeyPress& key)
{
    if (key.getKeyCode() == key.spaceKey)
    {
        pauseButton->setToggleState(!pauseButton->getToggleState(), sendNotification);
        return true;
    }

    return false;
}

bool LfpDisplayCanvas::keyPressed(const KeyPress& key, Component* orig)
{
    if (getTopLevelComponent() == orig && isVisible())
    {
        return keyPressed(key);
    }
    return false;
}

void LfpDisplayCanvas::saveVisualizerParameters(XmlElement* xml)
{

    XmlElement* xmlNode = xml->createNewChildElement("LFPDISPLAY");


    xmlNode->setAttribute("Range",selectedVoltageRangeValues[0]+","+selectedVoltageRangeValues[1]+
        ","+selectedVoltageRangeValues[2]);
    xmlNode->setAttribute("Timebase",timebaseSelection->getText());
    xmlNode->setAttribute("Spread",spreadSelection->getText());
    xmlNode->setAttribute("colorGrouping",colorGroupingSelection->getSelectedId());
    xmlNode->setAttribute("isInverted",invertInputButton->getToggleState());
    xmlNode->setAttribute("drawMethod",drawMethodButton->getToggleState());

    int eventButtonState = 0;

    for (int i = 0; i < 8; i++)
    {
        if (lfpDisplay->eventDisplayEnabled[i])
        {
            eventButtonState += (1 << i);
        }
    }

    xmlNode->setAttribute("EventButtonState", eventButtonState);

    String channelDisplayState = "";

    for (int i = 0; i < nChans; i++)
    {
        if (lfpDisplay->getEnabledState(i))
        {
            channelDisplayState += "1";
        }
        else
        {
            channelDisplayState += "0";
        }
    }

    xmlNode->setAttribute("ChannelDisplayState", channelDisplayState);

    xmlNode->setAttribute("ScrollX",viewport->getViewPositionX());
    xmlNode->setAttribute("ScrollY",viewport->getViewPositionY());
}


void LfpDisplayCanvas::loadVisualizerParameters(XmlElement* xml)
{
    forEachXmlChildElement(*xml, xmlNode)
    {
        if (xmlNode->hasTagName("LFPDISPLAY"))
        {
            StringArray ranges;
            ranges.addTokens(xmlNode->getStringAttribute("Range"),",",String::empty);
            selectedVoltageRangeValues[0] = ranges[0];
            selectedVoltageRangeValues[1] = ranges[1];
            selectedVoltageRangeValues[2] = ranges[2];
            selectedVoltageRange[0] = voltageRanges[0].indexOf(ranges[0])+1;
            selectedVoltageRange[1] = voltageRanges[1].indexOf(ranges[1])+1;
            selectedVoltageRange[2] = voltageRanges[2].indexOf(ranges[2])+1;
            rangeSelection->setText(ranges[0]);

            timebaseSelection->setText(xmlNode->getStringAttribute("Timebase"));
            spreadSelection->setText(xmlNode->getStringAttribute("Spread"));
            if (xmlNode->hasAttribute("colorGrouping"))
            {
                colorGroupingSelection->setSelectedId(xmlNode->getIntAttribute("colorGrouping"));
            }
            else
            {
                colorGroupingSelection->setSelectedId(1);
            }

            invertInputButton->setToggleState(xmlNode->getBoolAttribute("isInverted", true), sendNotification);

            drawMethodButton->setToggleState(xmlNode->getBoolAttribute("drawMethod", true), sendNotification);

            viewport->setViewPosition(xmlNode->getIntAttribute("ScrollX"),
                                      xmlNode->getIntAttribute("ScrollY"));

            int eventButtonState = xmlNode->getIntAttribute("EventButtonState");

            for (int i = 0; i < 8; i++)
            {
                lfpDisplay->eventDisplayEnabled[i] = (eventButtonState >> i) & 1;

                eventDisplayInterfaces[i]->checkEnabledState();
            }

            String channelDisplayState = xmlNode->getStringAttribute("ChannelDisplayState");

            for (int i = 0; i < channelDisplayState.length(); i++)
            {

                if (channelDisplayState.substring(i,i+1).equalsIgnoreCase("1"))
                {
                    //std::cout << "LfpDisplayCanvas enabling channel " << i << std::endl;
                    lfpDisplay->enableChannel(true, i);
                    isChannelEnabled.set(i,true); //lfpDisplay->enableChannel(true, i);
                }
                else
                {
                    //std::cout << "LfpDisplayCanvas disabling channel " << i << std::endl;
                    lfpDisplay->enableChannel(false, i);
                    isChannelEnabled.set(i,false);
                }


            }
        }
    }

}

ChannelType LfpDisplayCanvas::getChannelType(int n)
{
	if (n < processor->getNumInputs())
		return processor->channels[n]->getType();
	else
		return HEADSTAGE_CHANNEL;
}

ChannelType LfpDisplayCanvas::getSelectedType()
{
    return selectedChannelType;
}

void LfpDisplayCanvas::setSelectedType(ChannelType type, bool toggleButton)
{
    if (selectedChannelType == type)
        return; //Nothing to do here
    selectedChannelType = type;
    rangeSelection->clear(dontSendNotification);
rangeSelection->addItemList(voltageRanges[type], 1);
int id = selectedVoltageRange[type];
if (id)
rangeSelection->setSelectedId(id, sendNotification);
else
rangeSelection->setText(selectedVoltageRangeValues[selectedChannelType], dontSendNotification);
repaint(5, getHeight() - 55, 300, 100);

if (toggleButton)
typeButtons[type]->setToggleState(true, dontSendNotification);
}

String LfpDisplayCanvas::getTypeName(ChannelType type)
{
	return typeNames[type];
}

int LfpDisplayCanvas::getRangeStep(ChannelType type)
{
	return rangeSteps[type];
}

void LfpDisplayCanvas::refreshActionButton()
{
	if (lfpDisplay->getSingleChannelState())
	{
		yFangda->setVisible(true);
		ySuoxiao->setVisible(true);
	}
	else
	{
		yFangda->setVisible(false);
		ySuoxiao->setVisible(false);
	}
}
int LfpDisplayCanvas::getActionButtonState()
{
	return buttonState;
}
void LfpDisplayCanvas::setActionBottonState(int state)
{
	buttonState = state;

	if (state == 1)
	{
		xFangda->setState(1);
		yFangda->setState(-1);
	}
	else if (state == 2)
	{
		xFangda->setState(-1);
		yFangda->setState(1);
	}
	else
	{
		xFangda->setState(-1);
		yFangda->setState(-1);
	}
}

void LfpDisplayCanvas::setCustomTimeBase(float c)
{
	if (c < timebases[0].getFloatValue())
	{
		customTimebase = timebases[0].getFloatValue();
	}
	else if (c > historySeconds)
	{
		c = historySeconds;
	}
	else
	{
		customTimebase = c;
	}

	timebaseSelection->setText(String(customTimebase, 2), dontSendNotification);
	timescale->setTimebase(customTimebase);
}

float LfpDisplayCanvas::getTimeBase()
{
	return customTimebase > 0.0f ? customTimebase : timebase;
}
Range<float> LfpDisplayCanvas::getVoltageRange(int type)
{
	if (customRange[type].getStart() == 0 && customRange[type].getEnd() == 0)
	{
		float range = voltageRanges[type][selectedVoltageRange[type] - 1].getFloatValue()*rangeGain[type];
		return Range<float>(range / -2.0, range / 2.0);
	}
	else
	{
		return customRange[type];
	}
}

void LfpDisplayCanvas::readScreenBuffer(int channel, int unit, int maxCount, int *start1, int *size1, int *start2, int *size2)
{
	int requiredEndIndex = screenBufferEndIndex[channel];
	int startIndex = screenBufferStartIndex[channel];
	int endIndex = screenBufferEndIndex[channel];

	if (showScreenBufferIndex > 0)
	{
		if (startIndex < endIndex)
		{
			requiredEndIndex = screenBufferEndIndex[channel] + showScreenBufferIndex - (endIndex - startIndex + 1);
			if (requiredEndIndex > screenBufferEndIndex[channel])
			{
				requiredEndIndex = screenBufferEndIndex[channel];
			}
		}
		else
		{
			requiredEndIndex = screenBufferEndIndex[channel] + showScreenBufferIndex - historySamples;
			if (requiredEndIndex < 0)
			{
				requiredEndIndex += historySamples;
			}
		}
	}
	requiredEndIndex = requiredEndIndex - requiredEndIndex % unit;


	
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
	else if (startIndex > endIndex)
	{
		/**size2 = requiredEndIndex;
		if (*size2 < maxCount)
		{
			*size1 = maxCount - *size2;
			*start1 = historySamples - *size1;
		}
		else
		{
			*start2 = *size2 - maxCount;
		}*/
		*start2 = requiredEndIndex - maxCount;
		if (*start2 > 0)
		{
			*size2 = maxCount;
		}
		else
		{
			*start2 = 0;
			*size2 = requiredEndIndex;
			*size1 = maxCount - *size2;
			*start1 = historySamples - *size1 - 1;
		}

	}
}
// -------------------------------------------------------------

LfpTimescale::LfpTimescale(LfpDisplayCanvas* c) : canvas(c)
{

    font = Font("Default", 16, Font::plain);
}

LfpTimescale::~LfpTimescale()
{

}

void LfpTimescale::paint(Graphics& g)
{



    g.setFont(font);

    g.setColour(Colour(100,100,100));

    g.drawText("ms:",5,0,100,getHeight(),Justification::left, false);

    for (int i = 1; i < 10; i++)
    {
        if (i == 5)
            g.drawLine(getWidth()/10*i,0,getWidth()/10*i,getHeight(),3.0f);
        else
            g.drawLine(getWidth()/10*i,0,getWidth()/10*i,getHeight(),1.0f);

        g.drawText(labels[i-1],getWidth()/10*i+3,0,100,getHeight(),Justification::left, false);
    }

}

void LfpTimescale::setTimebase(float t)
{
    timebase = t;

    labels.clear();

    for (float i = 1.0f; i < 10.0; i++)
    {
        String labelString = String(timebase/10.0f*1000.0f*i);

        labels.add(labelString.substring(0,6));
    }

    repaint();

}


// ---------------------------------------------------------------

LfpDisplay::LfpDisplay(LfpDisplayCanvas* c, Viewport* v) :
    singleChan(-1), canvas(c), viewport(v)
{
    totalHeight = 0;
    colorGrouping=1;

    range[0] = 1000;
    range[1] = 500;
    range[2] = 500000;

    addMouseListener(this, true);

    // hue cycle
    //for (int i = 0; i < 15; i++)
    //{
    //    channelColours.add(Colour(float(sin((3.14/2)*(float(i)/15))),float(1.0),float(1),float(1.0)));
    //}

    //hand-built palette
    channelColours.add(Colour(224,185,36));
    channelColours.add(Colour(214,210,182));
    channelColours.add(Colour(243,119,33));
    channelColours.add(Colour(186,157,168));
    channelColours.add(Colour(237,37,36));
    channelColours.add(Colour(179,122,79));
    channelColours.add(Colour(217,46,171));
    channelColours.add(Colour(217, 139,196));
    channelColours.add(Colour(101,31,255));
    channelColours.add(Colour(141,111,181));
    channelColours.add(Colour(48,117,255));
    channelColours.add(Colour(184,198,224));
    channelColours.add(Colour(116,227,156));
    channelColours.add(Colour(150,158,155));
    channelColours.add(Colour(82,173,0));
    channelColours.add(Colour(125,99,32));

    isPaused=false;

}

LfpDisplay::~LfpDisplay()
{
    deleteAllChildren();
}



int LfpDisplay::getNumChannels()
{
    return numChans;
}



int LfpDisplay::getColorGrouping()
{
    return colorGrouping;
}

void LfpDisplay::setColorGrouping(int i)
{
    colorGrouping=i;
    setColors(); // so that channel colors get re-assigned

}


void LfpDisplay::setNumChannels(int numChannels)
{
    numChans = numChannels;

    deleteAllChildren();

    channels.clear();
    channelInfo.clear();

    totalHeight = 0;

    for (int i = 0; i < numChans; i++)
    {

        //std::cout << "Adding new display for channel " << i << std::endl;

        LfpChannelDisplay* lfpChan = new LfpChannelDisplay(canvas, this, i);

        //lfpChan->setColour(channelColours[i % channelColours.size()]);
        lfpChan->setRange(range[canvas->getChannelType(i)]);
        lfpChan->setChannelHeight(canvas->getChannelHeight());

        addAndMakeVisible(lfpChan);

        channels.add(lfpChan);

        LfpChannelDisplayInfo* lfpInfo = new LfpChannelDisplayInfo(canvas, this, i);

        //lfpInfo->setColour(channelColours[i % channelColours.size()]);
        lfpInfo->setRange(range[canvas->getChannelType(i)]);
        lfpInfo->setChannelHeight(canvas->getChannelHeight());

        addAndMakeVisible(lfpInfo);

        channelInfo.add(lfpInfo);

		savedChannelState.add(true);

        totalHeight += lfpChan->getChannelHeight();

    }

    setColors();

    //std::cout << "TOTAL HEIGHT = " << totalHeight << std::endl;

    // // this doesn't seem to do anything:
    //canvas->fullredraw = true;
    //refresh();

}

void LfpDisplay::setColors()
{
    for (int i = 0; i < numChans; i++)
    {

        channels[i]->setColour(channelColours[(int(i/colorGrouping)+1) % channelColours.size()]);
        channelInfo[i]->setColour(channelColours[(int(i/colorGrouping)+1)  % channelColours.size()]);
    }

}


int LfpDisplay::getTotalHeight()
{
    return totalHeight;
}

void LfpDisplay::resized()
{

    int totalHeight = 0;

    for (int i = 0; i < channels.size(); i++)
    {

        LfpChannelDisplay* disp = channels[i];

        disp->setBounds(canvas->leftmargin,
                        totalHeight-disp->getChannelOverlap()/2,
                        getWidth(),
                        disp->getChannelHeight()+disp->getChannelOverlap());

        LfpChannelDisplayInfo* info = channelInfo[i];

        info->setBounds(0,
                        totalHeight-disp->getChannelHeight()/4,
                        canvas->leftmargin,
                        disp->getChannelHeight());

        totalHeight += disp->getChannelHeight();

    }

    canvas->fullredraw = true; //issue full redraw
    if (singleChan != -1)
        viewport->setViewPosition(Point<int>(0,singleChan*getChannelHeight()));

    refresh();

    // std::cout << "Total height: " << totalHeight << std::endl;

}

void LfpDisplay::paint(Graphics& g)
{
}

void LfpDisplay::refresh()
{


    int topBorder = viewport->getViewPositionY();
    int bottomBorder = viewport->getViewHeight() + topBorder;

    // ensure that only visible channels are redrawn
    for (int i = 0; i < numChans; i++)
    {

        int componentTop = channels[i]->getY();
        int componentBottom = channels[i]->getHeight() + componentTop;

        if ((topBorder <= componentBottom && bottomBorder >= componentTop))
        {
			channels[i]->repaint();
			channelInfo[i]->repaint();
            /*if (canvas->fullredraw)
            {
                channels[i]->fullredraw = true;
                channels[i]->repaint();
                channelInfo[i]->repaint();

            }
            else
            {
                channels[i]->repaint(canvas->lastScreenBufferIndex[i]-2, 0, (canvas->screenBufferIndex[i]-canvas->lastScreenBufferIndex[i])+3, getChildComponent(i)->getHeight());  //repaint only the updated portion
                // we redraw from -2 to +1 relative to the real redraw window, the -2 makes sure that the lines join nicely, and the +1 draws the vertical update line
            }*/
            //std::cout << i << std::endl;
        }

    }

    canvas->fullredraw = false;
}

void LfpDisplay::setRange(float r, ChannelType type)
{
    range[type] = r;

    for (int i = 0; i < numChans; i++)
    {
        if (channels[i]->getType() == type)
            channels[i]->setRange(range[type]);
    }
    canvas->fullredraw = true; //issue full redraw
}

int LfpDisplay::getRange()
{
    return getRange(canvas->getSelectedType());
}

int LfpDisplay::getRange(ChannelType type)
{
    for (int i=0; i < numChans; i++)
    {
        if (channels[i]->getType() == type)
            return channels[i]->getRange();
    }
    return 0;
}


void LfpDisplay::setChannelHeight(int r, bool resetSingle)
{

    for (int i = 0; i < numChans; i++)
    {
        channels[i]->setChannelHeight(r);
        channelInfo[i]->setChannelHeight(r);
    }
    if (resetSingle && singleChan != -1)
    {
        setSize(getWidth(),numChans*getChannelHeight());
        viewport->setScrollBarsShown(true,false);
        viewport->setViewPosition(Point<int>(0,singleChan*r));
        singleChan = -1;
        for (int n = 0; n < numChans; n++)
        {
			channelInfo[n]->setEnabledState(savedChannelState[n]);
        }
    }

    resized();

}

void LfpDisplay::setInputInverted(bool isInverted)
{

    for (int i = 0; i < numChans; i++)
    {
        channels[i]->setInputInverted(isInverted);
    }

    resized();

}

void LfpDisplay::setDrawMethod(bool isDrawMethod)
{
    for (int i = 0; i < numChans; i++)
    {
        channels[i]->setDrawMethod(isDrawMethod);
    }
    resized();

}


int LfpDisplay::getChannelHeight()
{
    return channels[0]->getChannelHeight();
}



void LfpDisplay::mouseWheelMove(const MouseEvent&  e, const MouseWheelDetails&   wheel)
{

    //std::cout << "Mouse wheel " <<  e.mods.isCommandDown() << "  " << wheel.deltaY << std::endl;
    //TODO Changing ranges with the wheel is currently broken. With multiple ranges, most
    //of the wheel range code needs updating
    if (e.mods.isCommandDown())  // CTRL + scroll wheel -> change channel spacing
    {
        int h = getChannelHeight();
        int hdiff=0;
        
        std::cout << wheel.deltaY << std::endl;
        
        if (wheel.deltaY > 0)
        {
            hdiff = 2;
        }
        else
        {
            if (h > 5)
                hdiff = -2;
        }

        if (abs(h) > 100) // accelerate scrolling for large ranges
            hdiff *= 3;

        setChannelHeight(h+hdiff);
        int oldX=viewport->getViewPositionX();
        int oldY=viewport->getViewPositionY();

        setBounds(0,0,getWidth()-0, getChannelHeight()*canvas->nChans); // update height so that the scrollbar is correct

        int mouseY=e.getMouseDownY(); // should be y pos relative to inner viewport (0,0)
        int scrollBy = (mouseY/h)*hdiff*2;// compensate for motion of point under current mouse position
        viewport->setViewPosition(oldX,oldY+scrollBy); // set back to previous position plus offset

        canvas->setSpreadSelection(h+hdiff); // update combobox

    }
    else
    {
        if (e.mods.isShiftDown())  // SHIFT + scroll wheel -> change channel range
        {
            int h = getRange();
            int step = canvas->getRangeStep(canvas->getSelectedType());
            
            std::cout << wheel.deltaY << std::endl;
            
            if (wheel.deltaY > 0)
            {
                setRange(h+step,canvas->getSelectedType());
            }
            else
            {
                if (h > step+1)
                    setRange(h-step,canvas->getSelectedType());
            }

            canvas->setRangeSelection(h); // update combobox

        }
        else    // just scroll
        {
            //  passes the event up to the viewport so the screen scrolls
            if (viewport != nullptr && e.eventComponent == this) // passes only if it's not a listening event
                viewport->mouseWheelMove(e.getEventRelativeTo(canvas), wheel);

        }
    }

    canvas->fullredraw = true;//issue full redraw
    refresh();

}

void LfpDisplay::toggleSingleChannel(int chan)
{
    //std::cout << "Toggle channel " << chan << std::endl;

    if (chan != singleChan)
    {
        singleChan = chan;
        int newHeight = viewport->getHeight();
		channelInfo[chan]->setEnabledState(true);
        setChannelHeight(newHeight, false);
        setSize(getWidth(), numChans*getChannelHeight());
        viewport->setScrollBarsShown(false,false);
        viewport->setViewPosition(Point<int>(0,chan*newHeight));
        for (int n = 0; n < numChans; n++)
        {
			savedChannelState.set(n, channels[n]->getEnabledState());
            if (n != chan) channelInfo[n]->setEnabledState(false);
        }

    }
    else
    {
        setChannelHeight(canvas->getChannelHeight());
    }

}

bool LfpDisplay::getSingleChannelState()
{
    if (singleChan < 0) return false;
    else return true;
}
int LfpDisplay::getSelectedChannelType()
{
	return channels[singleChan]->getType();
}

void LfpDisplay::mouseUp(const MouseEvent& event)
{
	//std::cout << "mouseUp" << std::endl;

	if (canvas->startPoint != 0 && canvas->endPoint != 0)
	{
		if (canvas->getActionButtonState() == 1)
		{

			//1.确定X轴放大比例

			float timebase = canvas->getTimeBase();

			float newTimebase = timebase * abs(canvas->endPoint->x - canvas->startPoint->x) / (getWidth() - canvas->leftmargin);
			canvas->setCustomTimeBase(newTimebase);


			//2.确定查看历史的时间段
			//TODO

			canvas->startPoint = 0;
			canvas->endPoint = 0;
			canvas->setActionBottonState(-1);
			viewport->repaint();
		}
		else if (canvas->getActionButtonState() == 2)
		{
			//1.确定Y轴放大比例
			//2.确定Y轴坐标起止
			Range<float> rangeValue = canvas->getVoltageRange(getSelectedChannelType());

			float startY = jmax(canvas->startPoint->y, canvas->endPoint->y);
			float endY = jmin(canvas->startPoint->y, canvas->endPoint->y);

			int height = viewport->getHeight();
			float startValue = rangeValue.getEnd() - (rangeValue.getEnd() - rangeValue.getStart()) * startY / height;
			float endValue = rangeValue.getEnd() - (rangeValue.getEnd() - rangeValue.getStart()) * endY / height;
			canvas->setCustomVoltageRange(getSelectedChannelType(), startValue, endValue);
			canvas->startPoint = 0;
			canvas->endPoint = 0;
			canvas->setActionBottonState(-1);
			viewport->repaint();
		}
	}
}
void LfpDisplay::mouseDrag(const MouseEvent& event)
{
	//处理X轴，Y轴放大事件
	if (canvas->getActionButtonState() != -1)
	{
		if (canvas->startPoint != NULL)
		{
			//std::cout << "mouseMove,x:" << event.getPosition().x << "    y:" << event.getPosition().y << std::endl;
			canvas->endPoint = new Point<float>(event.getEventRelativeTo(viewport).getPosition().x, 
				event.getEventRelativeTo(viewport).getPosition().y);
			viewport->repaint();
			
		}
		return;
	}
}

void LfpDisplay::mouseDown(const MouseEvent& event)
{
    //处理X轴，Y轴放大事件
	if (canvas->getActionButtonState() != -1)
	{
		//std::cout << "mouseDown,x:" << event.getPosition().x << "    y:" << event.getPosition().y << std::endl;
		canvas->startPoint = new Point<float>(event.getEventRelativeTo(viewport).getPosition().x, 
			event.getEventRelativeTo(viewport).getPosition().y);
		return;
	}
	//int y = event.getMouseDownY(); //relative to each channel pos
	MouseEvent canvasevent = event.getEventRelativeTo(viewport);
	int y = canvasevent.getMouseDownY() + viewport->getViewPositionY(); // need to account for scrolling

    int dist = 0;
    int mindist = 10000;
    int closest = 5;
    for (int n = 0; n < numChans; n++) // select closest instead of relying on eventComponent
    {
        channels[n]->deselect();

        int cpos = (channels[n]->getY() + (channels[n]->getHeight()/2));
        dist = int(abs(y - cpos));

        //std::cout << "Mouse down at " << y << " pos is "<< cpos << "n:" << n << "  dist " << dist << std::endl;

        if (dist < mindist)
        {
            mindist = dist-1;
            closest = n;
        }
    }
    
	channels[closest]->select();
	canvas->setSelectedType(channels[closest]->getType());

	if (event.getNumberOfClicks() == 2)
	{//没有在 X轴，Y轴放大的环境中
		toggleSingleChannel(closest);
		canvas->refreshActionButton(); //显示/隐藏 Y轴放大缩小按钮
	}
       

    if (event.mods.isRightButtonDown())
    {
        PopupMenu channelMenu = channels[closest]->getOptions();
        const int result = channelMenu.show();
        channels[closest]->changeParameter(result);
    }

    canvas->fullredraw = true;//issue full redraw

    refresh();

}


bool LfpDisplay::setEventDisplayState(int ch, bool state)
{
    eventDisplayEnabled[ch] = state;
    return eventDisplayEnabled[ch];
}


bool LfpDisplay::getEventDisplayState(int ch)
{
    return eventDisplayEnabled[ch];
}

void LfpDisplay::enableChannel(bool state, int chan)
{

    if (chan < numChans)
    {
        channelInfo[chan]->setEnabledState(state);
        canvas->isChannelEnabled.set(chan, state);
    }
}

void LfpDisplay::setEnabledState(bool state, int chan)
{

    if (chan < numChans)
    {
        channels[chan]->setEnabledState(state);
        canvas->isChannelEnabled.set(chan, state);
    }
}

bool LfpDisplay::getEnabledState(int chan)
{
    if (chan < numChans)
    {
        return channels[chan]->getEnabledState();
    }

    return false;
}


// ------------------------------------------------------------------

LfpChannelDisplay::LfpChannelDisplay(LfpDisplayCanvas* c, LfpDisplay* d, int channelNumber) :
    canvas(c), display(d), isSelected(false), chan(channelNumber),
    channelOverlap(300), channelHeight(40), range(1000.0f),
    isEnabled(true), inputInverted(false), canBeInverted(true), drawMethod(false)
{


    name = String(channelNumber+1); // default is to make the channelNumber the name


    channelHeightFloat = (float) channelHeight;

    channelFont = Font("Default", channelHeight*0.6, Font::plain);

    lineColour = Colour(255,255,255);

    type = c->getChannelType(channelNumber);
    typeStr = c->getTypeName(type);

}

LfpChannelDisplay::~LfpChannelDisplay()
{

}

void LfpChannelDisplay::updateType()
{
    type = canvas->getChannelType(chan);
    typeStr = canvas->getTypeName(type);
}

void LfpChannelDisplay::setEnabledState(bool state)
{

    //if (state)
    //std::cout << "Setting channel " << name << " to true." << std::endl;
    //else
    //std::cout << "Setting channel " << name << " to false." << std::endl;

    isEnabled = state;

}

void LfpChannelDisplay::paint(Graphics& g)
{

    //g.fillAll(Colours::grey);

    //g.setColour(Colours::yellow);   // draw most recent drawn sample position
    //g.drawLine(canvas->screenBufferIndex[chan]+1, 0, canvas->screenBufferIndex[chan]+1, getHeight());


    //g.setColour(Colours::red); // draw oldest drawn sample position
    //g.drawLine(canvas->lastScreenBufferIndex, 0, canvas->lastScreenBufferIndex, getHeight()-channelOverlap);

    if (isEnabled)
    {

        int center = getHeight()/2;

        if (isSelected)
        {

            g.setColour(Colours::lightgrey);
            g.fillRect(0,center-channelHeight/2,10,channelHeight);
            g.drawLine(0,center+channelHeight/2,getWidth(),center+channelHeight/2);
            g.drawLine(0,center-channelHeight/2,getWidth(),center-channelHeight/2);

            if (!display->getSingleChannelState())
            {
                g.setColour(Colour(25,25,25));
                g.drawLine(0,center+channelHeight/4,10,center+channelHeight/4);
                g.drawLine(0,center-channelHeight/4,10,center-channelHeight/4);
            }
            else
            {
                g.setColour(Colour(70,70,70));
                g.drawLine(0,center+channelHeight/4,getWidth(),center+channelHeight/4);
                g.drawLine(0,center-channelHeight/4,getWidth(),center-channelHeight/4);
            }


        }

        if (display->getSingleChannelState())
        {

            int leftEdge = 150;

            //float r = range;

            g.setColour(Colours::lightgrey);
            g.setFont(channelFont);
            g.setFont(20);
            String unitString;
            if (getType() == ADC_CHANNEL)
            {
                unitString = " V";
                //r = range / 1000.0f;
            } else {
                unitString = " uV";
            }
			Range<float> rangeValue = canvas->getVoltageRange(type);
			float start = rangeValue.getStart();
			float end = rangeValue.getEnd();
			float step = (end - start) / 4;
	
			g.drawText(String(start + step * 2.0) + unitString, 20, center, leftEdge, 25, Justification::left, false);
			g.drawText(String(end) + unitString, 20, center-channelHeight/2, leftEdge, 25, Justification::left, false);
			g.drawText(String(start) + unitString, 20, center+channelHeight/2-25, leftEdge, 25, Justification::left, false);
			g.drawText(String(end - step) + unitString, 20, center-channelHeight/4, leftEdge, 25, Justification::left, false);
			g.drawText(String(start + step) + unitString, 20, center+channelHeight/4, leftEdge, 25, Justification::left, false);

            //g.drawText(String(0) + unitString, 20, center, leftEdge, 25, Justification::left, false);
            //g.drawText(String(r/2) + unitString, 20, center-channelHeight/2, leftEdge, 25, Justification::left, false);
            //g.drawText(String(-r/2) + unitString, 20, center+channelHeight/2-25, leftEdge, 25, Justification::left, false);
            //g.drawText(String(r/4) + unitString, 20, center-channelHeight/4, leftEdge, 25, Justification::left, false);
            //g.drawText(String(-r/4) + unitString, 20, center+channelHeight/4, leftEdge, 25, Justification::left, false);

        }


        g.setColour(Colour(40,40,40));
        g.drawLine(0, getHeight()/2, getWidth(), getHeight()/2);

		int sampleCount = canvas->getTimeBase() * canvas->historySampleRate;
		int showPixels = jmin((int)(canvas->getTimeBase() * canvas->historySampleRate), 600);  //一共显示多少点
		int unit = sampleCount / showPixels;//一个点多少个采样点
		if (unit < 1)
		{
			unit = 1;
		}
		int count = showPixels;
		int start1, start2, size1, size2;
		
		canvas->readScreenBuffer(chan, unit, count*unit, &start1, &size1, &start2, &size2);
		float preX = -1;
		float preY = -1;
		Range<float> rangValue = canvas->getVoltageRange(type);
		if (size1 > 0)
		{
			const float *source = canvas->screenBuffer->getReadPointer(chan, start1);
			const float *eventSource = canvas->screenBuffer->getReadPointer(canvas->getNumChannels(), start1);
			for (int i = 0; i < size1 / unit; i++)
			{
				float x = i * getWidth() / (float)showPixels;
				float y2 = source[i*unit] / range * channelHeightFloat + center;
				//float y =  (0 - channelHeightFloat) * (rangValue.getEnd() - source[i*unit]) / (rangValue.getEnd() - rangValue.getStart());
				float y = (source[i*unit] - (rangValue.getEnd() + rangValue.getStart()) / 2.0) / (rangValue.getEnd() - rangValue.getStart()) * channelHeightFloat + center;
				int rawEventState = eventSource[i*unit];
				for (int ev_ch = 0; ev_ch < 8; ev_ch++) // for all event channels
				{
					if (display->getEventDisplayState(ev_ch))  // check if plotting for this channel is enabled
					{
						if (rawEventState & (1 << ev_ch))    // events are  representet by a bit code, so we have to extract the individual bits with a mask
						{
							//std::cout << "Drawing event." << std::endl;
							g.setColour(display->channelColours[ev_ch * 2]); // get color from lfp color scheme
							g.setOpacity(0.35f);
							g.drawLine(x, center - channelHeight / 2, x, center + channelHeight / 2);
						}
					}
				}
				if (preX >= 0)
				{
					g.setOpacity(1.0f);
					g.setColour(lineColour);
					g.drawLine(preX, preY, x, y);
				}
				preX = x;
				preY = y;
			}
		}
		if (size2 > 0)
		{
			const float *source = canvas->screenBuffer->getReadPointer(chan, start2);
			const float *eventSource = canvas->screenBuffer->getReadPointer(canvas->getNumChannels(), start2);
			for (int i = 0; i < size2 / unit; i++)
			{
				float x = (i + size1 / unit) * getWidth() / (float)showPixels;
				//float y = source[i*unit] / range * channelHeightFloat + center;
				float y = (source[i*unit] - (rangValue.getEnd() + rangValue.getStart()) / 2.0) / (rangValue.getEnd() - rangValue.getStart()) * channelHeightFloat + center;
				int rawEventState = eventSource[i*unit];
				//int y = (Random().nextInt(500) - 500) / range * channelHeightFloat + center;
				for (int ev_ch = 0; ev_ch < 8; ev_ch++) // for all event channels
				{
					if (display->getEventDisplayState(ev_ch))  // check if plotting for this channel is enabled
					{
						if (rawEventState & (1 << ev_ch))    // events are  representet by a bit code, so we have to extract the individual bits with a mask
						{
							//std::cout << "Drawing event." << std::endl;
							g.setColour(display->channelColours[ev_ch * 2]); // get color from lfp color scheme
							g.setOpacity(0.35f);
							g.drawLine(x, center - channelHeight / 2, x, center + channelHeight / 2);
						}
					}
				}
				if (preX >= 0)
				{
					g.setColour(lineColour);
					g.setOpacity(1.0f);
					g.drawLine(preX, preY, x, y);
				}
				preX = x;
				preY = y;
				/*int y = topMargin + h / 2.0 - source2[i*unit] / range * h;
				int x = leftMargin + w * (i + size1 / unit) * 1.0f / (float)showPixels;
				if (preX > 0)
				{
					g.drawLine(preX, preY, x, y);
				}
				preX = x;
				preY = y;*/
			}
		}
    }

    // g.setColour(lineColour.withAlpha(0.7f)); // alpha on seems to decrease draw speed
    // g.setFont(channelFont);
    //  g.setFont(channelHeightFloat*0.6);

    // g.drawText(String(chan+1), 10, center-channelHeight/2, 200, channelHeight, Justification::left, false);


}


PopupMenu LfpChannelDisplay::getOptions()
{

    PopupMenu menu;
    menu.addItem(1, "Invert signal", true, inputInverted);

    return menu;
}

void LfpChannelDisplay::changeParameter(int id)
{
    switch (id)
    {
        case 1:
            setInputInverted(!inputInverted);
        default:
            break;
    }
}

void LfpChannelDisplay::setRange(float r)
{
    
    range = r;

    //std::cout << "Range: " << r << std::endl;
}

int LfpChannelDisplay::getRange()
{
    return range;
}


void LfpChannelDisplay::select()
{
    isSelected = true;
}

void LfpChannelDisplay::deselect()
{
    isSelected = false;
}

bool LfpChannelDisplay::getSelected()
{
   return isSelected;
}

void LfpChannelDisplay::setColour(Colour c)
{
    lineColour = c;
}


void LfpChannelDisplay::setChannelHeight(int c)
{
    channelHeight = c;

    channelHeightFloat = (float) channelHeight;

    if (!inputInverted)
        channelHeightFloat = -channelHeightFloat;

    channelOverlap = channelHeight*5;
}

int LfpChannelDisplay::getChannelHeight()
{

    return channelHeight;
}

void LfpChannelDisplay::setChannelOverlap(int overlap)
{
    channelOverlap = overlap;
}


int LfpChannelDisplay::getChannelOverlap()
{
    return channelOverlap;
}

void LfpChannelDisplay::setCanBeInverted(bool _canBeInverted)
{
    canBeInverted = _canBeInverted;
}

void LfpChannelDisplay::setInputInverted(bool isInverted)
{
    if (canBeInverted)
    {
        inputInverted = isInverted;
        setChannelHeight(channelHeight);
    }
}

void LfpChannelDisplay::setDrawMethod(bool isDrawMethod)
{

    drawMethod = isDrawMethod;

}


void LfpChannelDisplay::setName(String name_)
{
    name = name_;
}

ChannelType LfpChannelDisplay::getType()
{
    return type;
}

// -------------------------------

LfpChannelDisplayInfo::LfpChannelDisplayInfo(LfpDisplayCanvas* canvas_, LfpDisplay* display_, int ch)
    : LfpChannelDisplay(canvas_, display_, ch)
{

    chan = ch;

    enableButton = new UtilityButton(String(ch+1), Font("Small Text", 13, Font::plain));
    enableButton->setRadius(5.0f);

    enableButton->setEnabledState(true);
    enableButton->setCorners(true, true, true, true);
    enableButton->addListener(this);
    enableButton->setClickingTogglesState(true);
    enableButton->setToggleState(true, dontSendNotification);

    addAndMakeVisible(enableButton);

}

void LfpChannelDisplayInfo::updateType()
{
    type = canvas->getChannelType(chan);
    typeStr = canvas->getTypeName(type);
    repaint();
}

void LfpChannelDisplayInfo::buttonClicked(Button* button)
{

    bool state = button->getToggleState();

    display->setEnabledState(state, chan);

    //UtilityButton* b = (UtilityButton*) button;

    // if (state)
    // {
    //  b->setLabel("ON");
    // } else {
    //  b->setLabel("OFF");
    // }

    //std::cout << "Turn channel " << chan << " to " << button->getToggleState() << std::endl;

}

void LfpChannelDisplayInfo::setEnabledState(bool state)
{
    enableButton->setToggleState(state, sendNotification);
}

void LfpChannelDisplayInfo::paint(Graphics& g)
{

    int center = getHeight()/2;

    g.setColour(lineColour);

    //if (chan > 98)
    //  g.fillRoundedRectangle(5,center-8,51,22,8.0f);
    //else
    g.fillRoundedRectangle(5,center-8,41,22,8.0f);

      g.setFont(Font("Small Text", 13, Font::plain));
      g.drawText(typeStr,5,center+16,41,10,Justification::centred,false);
    // g.setFont(channelHeightFloat*0.3);

    //  g.drawText(name, 10, center-channelHeight/2, 200, channelHeight, Justification::left, false);

}

void LfpChannelDisplayInfo::resized()
{

    int center = getHeight()/2;

    //if (chan > 98)
    //  enableButton->setBounds(8,center-5,45,16);
    //else
    enableButton->setBounds(8,center-5,35,16);
}


// Event display Options --------------------------------------------------------------------

EventDisplayInterface::EventDisplayInterface(LfpDisplay* display_, LfpDisplayCanvas* canvas_, int chNum):
    isEnabled(true), display(display_), canvas(canvas_)
{

    channelNumber = chNum;

    chButton = new UtilityButton(String(channelNumber+1), Font("Small Text", 13, Font::plain));
    chButton->setRadius(5.0f);
    chButton->setBounds(4,4,14,14);
    chButton->setEnabledState(true);
    chButton->setCorners(true, false, true, false);
    //chButton.color = display->channelColours[channelNumber*2];
    chButton->addListener(this);
    addAndMakeVisible(chButton);


    checkEnabledState();

}

EventDisplayInterface::~EventDisplayInterface()
{

}

void EventDisplayInterface::checkEnabledState()
{
    isEnabled = display->getEventDisplayState(channelNumber);

    //repaint();
}

void EventDisplayInterface::buttonClicked(Button* button)
{
    checkEnabledState();
    if (isEnabled)
    {
        display->setEventDisplayState(channelNumber, false);
    }
    else
    {
        display->setEventDisplayState(channelNumber, true);
    }

    repaint();

}


void EventDisplayInterface::paint(Graphics& g)
{

    checkEnabledState();

    if (isEnabled)
    {
        g.setColour(display->channelColours[channelNumber*2]);
        g.fillRoundedRectangle(2,2,18,18,5.0f);
    }


    //g.drawText(String(channelNumber), 8, 2, 200, 15, Justification::left, false);

}

// Lfp Viewport -------------------------------------------

LfpViewport::LfpViewport(LfpDisplayCanvas *canvas)
    : Viewport()
{
    this->canvas = canvas;
}

void LfpViewport::visibleAreaChanged(const Rectangle<int>& newVisibleArea)
{
    canvas->fullredraw = true;
    canvas->refresh();
}

void LfpViewport::paint(Graphics& g)
{
	if (canvas->startPoint != 0 && canvas->endPoint != 0 )
	{
		g.setColour(Colours::white);
		float dashLine[] = { 3, 3 };
		if (canvas->getActionButtonState() == 1)
		{
			int x = canvas->startPoint->x > canvas->endPoint->x ? canvas->endPoint->x : canvas->startPoint->x;
			int y = 1;
			int width = abs(canvas->endPoint->x - canvas->startPoint->x);
			int height = getHeight() - 2;

			g.drawDashedLine(Line<float>(x, y, x, y + height), &dashLine[0], 2);
			g.drawDashedLine(Line<float>(x, y, x + width, y), &dashLine[0], 2);
			g.drawDashedLine(Line<float>(x + width, y, x + width, y + height), &dashLine[0], 2);
			g.drawDashedLine(Line<float>(x, y + height, x + width, y + height), &dashLine[0], 2);
		}
		else if (canvas->getActionButtonState() == 2)
		{
			int x = canvas->leftmargin + 1;
			int y = canvas->startPoint->y > canvas->endPoint->y ? canvas->endPoint->y : canvas->startPoint->y;
			int width = getWidth() - canvas->leftmargin - 2;
			int height = abs(canvas->endPoint->y - canvas->startPoint->y);

			g.drawDashedLine(Line<float>(x, y, x, y + height), &dashLine[0], 2);
			g.drawDashedLine(Line<float>(x, y, x + width, y), &dashLine[0], 2);
			g.drawDashedLine(Line<float>(x + width, y, x + width, y + height), &dashLine[0], 2);
			g.drawDashedLine(Line<float>(x, y + height, x + width, y + height), &dashLine[0], 2);
		}
	}
}

ActionButton::ActionButton(LfpDisplayCanvas* canvas_, String name) : ImageButton(), canvas(canvas_)
{
	File rootPath = File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile).getParentDirectory();
	Image image = ImageFileFormat::loadFrom(*rootPath.getChildFile(name).createInputStream());
	setImages(false, true, true, image, 1, Colours::white,
		image, 1.0f, Colours::yellow, image, 1.0f, Colours::yellow);
}

void ActionButton::paintButton(Graphics&g, bool isMouseOver, bool isButtonDown)
{
	if (!isMouseOver && !isButtonDown && state == 1)
	{
		ImageButton::paintButton(g, false, true);
	} 
	else
	{
		ImageButton::paintButton(g, isMouseOver, isButtonDown);
	}
	
}