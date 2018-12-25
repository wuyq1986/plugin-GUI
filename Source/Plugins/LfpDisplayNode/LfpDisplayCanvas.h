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
#ifndef __LFPDISPLAYCANVAS_H_B711873A__
#define __LFPDISPLAYCANVAS_H_B711873A__

#include <VisualizerWindowHeaders.h>
#include "LfpDisplayNode.h"

#define CHANNEL_TYPES 3

class LfpDisplayNode;

class LfpTimescale;
class LfpDisplay;
class LfpChannelDisplay;
class LfpChannelDisplayInfo;
class EventDisplayInterface;
class LfpViewport;
class ActionButton;

/**

  Displays multiple channels of continuous data.

  @see LfpDisplayNode, LfpDisplayEditor

*/

class LfpDisplayCanvas : public Visualizer,
    public ComboBox::Listener,
    public Button::Listener,
    public KeyListener

{
public:
    LfpDisplayCanvas(LfpDisplayNode* n);
    ~LfpDisplayCanvas();

    void beginAnimation();
    void endAnimation();

    void refreshState();
    void update();

    void setParameter(int, float);
    void setParameter(int, int, int, float) {}

    void setRangeSelection(float range, bool canvasMustUpdate = false); // set range selection combo box to correct value if it has been changed by scolling etc.
    void setSpreadSelection(int spread, bool canvasMustUpdate = false); // set spread selection combo box to correct value if it has been changed by scolling etc.

    void paint(Graphics& g);

    void refresh();

    void resized();

    int getChannelHeight();

    int getNumChannels();
    bool getInputInvertedState();
    bool getDrawMethodState();

    //const float getXCoord(int chan, int samp);
    //const float getYCoord(int chan, int samp);

    //const float getYCoordMin(int chan, int samp);
    //const float getYCoordMean(int chan, int samp);
    //const float getYCoordMax(int chan, int samp);

	int historySampleRate = 10000; //采样率 1秒采集多少样本
	int historySeconds;
	int historySamples;
	int maxHistorySeconds = 10*60;  //最大历史保存时间
	int maxHistorySamples = 10 * 60 * 10000 * 35; 
	

    void comboBoxChanged(ComboBox* cb);
    void buttonClicked(Button* button);

    void saveVisualizerParameters(XmlElement* xml);
    void loadVisualizerParameters(XmlElement* xml);

    bool keyPressed(const KeyPress& key);
    bool keyPressed(const KeyPress& key, Component* orig);

    ChannelType getChannelType(int n);
    ChannelType getSelectedType();
    String getTypeName(ChannelType type);
    int getRangeStep(ChannelType type);

    void setSelectedType(ChannelType type, bool toggleButton = true);

    //void scrollBarMoved(ScrollBar *scrollBarThatHasMoved, double newRangeStart);

    bool fullredraw; // used to indicate that a full redraw is required. is set false after each full redraw, there is a similar switch for ach ch display;
    static const int leftmargin=50; // left margin for lfp plots (so the ch number text doesnt overlap)

    Array<bool> isChannelEnabled;

    int nChans;

	void readScreenBuffer(int channel, int unit, int maxCount, int *start1, int *size1, int *start2, int *size2);
	
	AudioSampleBuffer* screenBuffer;
	//保存历史数据的结构是一个环形结构，需要知道哪里是开始，哪里是结束
	Array<int> screenBufferStartIndex;
	Array<int> screenBufferEndIndex;
	int showScreenBufferIndex = 25000;  //左右滑动查看历史数据时Index   0 --------- screenBuffer的最大samples

	void setCustomVoltageRange(int type,float start, float end);
	Range<float> getVoltageRange(int type);
	float getTimeBase();
	void setCustomTimeBase(float c);
	void refreshActionButton();
	int getActionButtonState();
	void setActionBottonState(int state);

	//X轴，Y轴放大
	ScopedPointer<Point<float>> startPoint = 0;
	ScopedPointer<Point<float>> endPoint = 0;
private:

    Array<float> sampleRate;
    float displayGain;
    float timeOffset;
    //int spread ; // vertical spacing between channels


    static const int MAX_N_CHAN = 2048;  // maximum number of channels
    static const int MAX_N_SAMP = 5000; // maximum display size in pixels
    //float waves[MAX_N_CHAN][MAX_N_SAMP*2]; // we need an x and y point for each sample

    LfpDisplayNode* processor;
    AudioSampleBuffer* displayBuffer; // LfpDisplayNode中收集的 sample wise data buffer for display
    // 本文件中保存的 subsampled buffer- one int per pixel

    //'define 3 buffers for min mean and max for better plotting of spikes
    // not pretty, but 'AudioSampleBuffer works only for channels X samples
    //AudioSampleBuffer* screenBufferMin; // like screenBuffer but holds min/mean/max values per pixel
    //AudioSampleBuffer* screenBufferMean; // like screenBuffer but holds min/mean/max values per pixel
    //AudioSampleBuffer* screenBufferMax; // like screenBuffer but holds min/mean/max values per pixel

    MidiBuffer* eventBuffer;

    ScopedPointer<LfpTimescale> timescale;
    ScopedPointer<LfpDisplay> lfpDisplay;
    ScopedPointer<LfpViewport> viewport;

    ScopedPointer<ComboBox> timebaseSelection;
    ScopedPointer<ComboBox> rangeSelection;
    ScopedPointer<ComboBox> spreadSelection;
    ScopedPointer<ComboBox> colorGroupingSelection;
    ScopedPointer<UtilityButton> invertInputButton;
    ScopedPointer<UtilityButton> drawMethodButton;
    ScopedPointer<UtilityButton> pauseButton;
    OwnedArray<UtilityButton> typeButtons;

    StringArray voltageRanges[CHANNEL_TYPES];
    StringArray timebases;
    StringArray spreads; // option for vertical spacing between channels
	float timebase;
    StringArray colorGroupings; // option for coloring every N channels the same

    ChannelType selectedChannelType;
    int selectedVoltageRange[CHANNEL_TYPES];
    String selectedVoltageRangeValues[CHANNEL_TYPES];
    float rangeGain[CHANNEL_TYPES];
    StringArray rangeUnits;
    StringArray typeNames;
    int rangeSteps[CHANNEL_TYPES];

    int selectedSpread;
    String selectedSpreadValue;

    int selectedTimebase;
    String selectedTimebaseValue;

    OwnedArray<EventDisplayInterface> eventDisplayInterfaces;

    void refreshScreenBuffer();
    void updateScreenBuffer();

    Array<int> displayBufferIndex;
    int displayBufferSize;

    int scrollBarThickness;

	//X轴放大，缩小，Y轴放大，缩小按钮
	float customTimebase = -1;
	Range<float> customRange[CHANNEL_TYPES];
	int buttonState = -1; //-1 啥事都没有   //1  X轴按钮放大   //2 Y轴按钮方法
	ScopedPointer<ActionButton> xFangda;
	ScopedPointer<ActionButton> xSuoxiao;
	ScopedPointer<ActionButton> yFangda;
	ScopedPointer<ActionButton> ySuoxiao;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfpDisplayCanvas);

};

class LfpTimescale : public Component
{
public:
    LfpTimescale(LfpDisplayCanvas*);
    ~LfpTimescale();

    void paint(Graphics& g);

    void setTimebase(float t);

private:

    LfpDisplayCanvas* canvas;

    float timebase;

    Font font;

    StringArray labels;

};

class LfpDisplay : public Component
{
public:
    LfpDisplay(LfpDisplayCanvas*, Viewport*);
    ~LfpDisplay();

    void setNumChannels(int numChannels);
    int getNumChannels();

    int getTotalHeight();

    void paint(Graphics& g);

    void refresh();

    void resized();

	void mouseUp(const MouseEvent& event);
	void mouseDown(const MouseEvent& event);
	void mouseDrag(const MouseEvent& event);
    void mouseWheelMove(const MouseEvent&  event, const MouseWheelDetails&   wheel) ;


    void setRange(float range, ChannelType type);
    
    //Withouth parameters returns selected type
    int getRange();
    int getRange(ChannelType type);

    void setChannelHeight(int r, bool resetSingle = true);
    int getChannelHeight();
    void setInputInverted(bool);
    void setDrawMethod(bool);

    void setColors();

    bool setEventDisplayState(int ch, bool state);
    bool getEventDisplayState(int ch);

    int getColorGrouping();
    void setColorGrouping(int i);

    void setEnabledState(bool, int);
    bool getEnabledState(int);
    void enableChannel(bool, int);

    bool getSingleChannelState();
	int getSelectedChannelType();


    Array<Colour> channelColours;

    Array<LfpChannelDisplay*> channels;
    Array<LfpChannelDisplayInfo*> channelInfo;

    bool eventDisplayEnabled[8];
    bool isPaused; // simple pause function, skips screen bufer updates

private:
    void toggleSingleChannel(int chan);
    int singleChan;
	Array<bool> savedChannelState;

    int numChans;

    int totalHeight;

    int colorGrouping;

    LfpDisplayCanvas* canvas;
    Viewport* viewport;

    float range[3];


};

class LfpChannelDisplay : public Component
{
public:
    LfpChannelDisplay(LfpDisplayCanvas*, LfpDisplay*, int channelNumber);
    ~LfpChannelDisplay();

    void paint(Graphics& g);

    void select();
    void deselect();

    bool getSelected();

    void setName(String);

    void setColour(Colour c);

    void setChannelHeight(int);
    int getChannelHeight();

    void setChannelOverlap(int);
    int getChannelOverlap();

    void setRange(float range);
    int getRange();

    void setInputInverted(bool);
    void setCanBeInverted(bool);

    void setDrawMethod(bool);

    PopupMenu getOptions();
    void changeParameter(const int id);

    void setEnabledState(bool);
    bool getEnabledState()
    {
        return isEnabled;
    }

    ChannelType getType();
    void updateType();

    bool fullredraw; // used to indicate that a full redraw is required. is set false after each full redraw

protected:

    LfpDisplayCanvas* canvas;
    LfpDisplay* display;

    bool isSelected;

    int chan;

    String name;

    Font channelFont;

    Colour lineColour;

    int channelOverlap;
    int channelHeight;
    float channelHeightFloat;

    float range;

    bool isEnabled;
    bool inputInverted;
    bool canBeInverted;
    bool drawMethod;

    ChannelType type;
    String typeStr;

};

class LfpChannelDisplayInfo : public LfpChannelDisplay,
    public Button::Listener
{
public:
    LfpChannelDisplayInfo(LfpDisplayCanvas*, LfpDisplay*, int channelNumber);

    void paint(Graphics& g);

    void buttonClicked(Button* button);

    void resized();

    void setEnabledState(bool);
    void updateType();

private:

    ScopedPointer<UtilityButton> enableButton;

};

class EventDisplayInterface : public Component,
    public Button::Listener
{
public:
    EventDisplayInterface(LfpDisplay*, LfpDisplayCanvas*, int chNum);
    ~EventDisplayInterface();

    void paint(Graphics& g);

    void buttonClicked(Button* button);

    void checkEnabledState();

    bool isEnabled;

private:

    int channelNumber;

    LfpDisplay* display;
    LfpDisplayCanvas* canvas;

    ScopedPointer<UtilityButton> chButton;

};

class LfpViewport : public Viewport
{
public:
    LfpViewport(LfpDisplayCanvas* canvas);
    void visibleAreaChanged(const Rectangle<int>& newVisibleArea);
	void paint(Graphics& g);
private:
    LfpDisplayCanvas* canvas;
};

class ActionButton : public ImageButton
{
public:
	ActionButton(LfpDisplayCanvas* canvas,String name);
	void setState(int state_)
	{
		state = state_;
		repaint();
	}
	void paintButton(Graphics&, bool isMouseOver, bool isButtonDown) override;
private:
	LfpDisplayCanvas* canvas;
	int state = -1;
};

#endif  // __LFPDISPLAYCANVAS_H_B711873A__
