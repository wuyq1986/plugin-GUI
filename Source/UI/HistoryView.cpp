#include "HistoryView.h"
#include "../Processors/RecordNode/OriginalRecording.h"

const int BUFFER_BLOCK_SIZE = 10 * 30000;
HistoryView::HistoryView() : Thread("historyView")
{
	const String filename = "";
	filenameComponent = new FilenameComponent("file selector", filename, true, false, false, "*.continuous", "", "");
	addAndMakeVisible(filenameComponent);
	filenameComponent->addListener(this);

	loadView = new LoadView();
	addAndMakeVisible(loadView);
	loadView->setVisible(false);
	displayBuffer = new AudioSampleBuffer(1, BUFFER_BLOCK_SIZE);

	xFangda = new ActionButton("Xfangda.png");
	addAndMakeVisible(xFangda);
	xFangda->addListener(this);

	xSuoxiao = new ActionButton("Xsuoxiao.png");
	addAndMakeVisible(xSuoxiao);
	xSuoxiao->addListener(this);

	yFangda = new ActionButton("Yfangda.png");
	addAndMakeVisible(yFangda);
	yFangda->addListener(this);

	ySuoxiao = new ActionButton("Ysuoxiao.png");
	addAndMakeVisible(ySuoxiao);
	ySuoxiao->addListener(this);

	initData();
}

HistoryView::~HistoryView()
{
	if (file != NULL)
	{
		fclose(file);
		file = NULL;
	}
	deleteAndZero(displayBuffer);
	stopThread(2);
}

void HistoryView::resized()
{
	filenameComponent->setBounds(0, 0, w - 200, 32);
	loadView->setBounds(w / 2.0 - 100, getHeight() / 2.0 - 50, 200, 100);

	xFangda->setBounds(w - 160, 3, 25, 25);
	xSuoxiao->setBounds(w - 130, 3, 25, 25);
	yFangda->setBounds(w - 100, 3, 25, 25);
	ySuoxiao->setBounds(w -70, 3, 25, 25);


	w = getWidth();
	h = getHeight() - y; //画曲线图的区域大小 

	xAxisLength = w - leftMargin - rightMargin;
	yAxisLength = h - topMargin - bottomMargin;
	repaint();

}

void HistoryView::initData()
{
	sampleIndex.clear();
	sampleCount.clear();
	totalSampleCount = 0;
	minVolt = LONG_MAX;
	maxVolt = LONG_MIN;

	fileLength = 0;
	bitVolts = 0;
	sampleRate = 0;

	startVolt = -500;
	endVolt = 500;

	sampleRate = 30000;
	startIndex = 0;  //X轴显示的开始，截至
	endIndex = 10 * sampleRate;

	file = NULL;


}
void HistoryView::filenameComponentChanged(FilenameComponent *fnc)
{
	while (isThreadRunning())
	{
		stopThread(500);
	}

	if (file != NULL)
	{
		fclose(file);
		file = NULL;
	}

	initData();
	loadView->setVisible(true);
	loadView->start();
	
	isProcessing = true;
	file = fopen(fnc->getCurrentFile().getFullPathName().toUTF8(), "rb");
	fseek(file, 0L, SEEK_END);
	fileLength = ftell(file) + 1;
	startThread(0);
}

void HistoryView::run()
{
	long offset = 0;
	char headInfo[1025];
	memset(headInfo, 0x00, sizeof(headInfo));
	fseek(file, 0L, SEEK_SET);
	fread(headInfo, HEADER_SIZE, 1, file);
	offset += HEADER_SIZE;
	String headStr(headInfo);
	int index = headStr.indexOf(0, "header.bitVolts = ");
	int index2 = headStr.indexOf(index, ";");
	bitVolts = headStr.substring(index + 18, index2).getFloatValue();

	index = headStr.indexOf(0, "header.sampleRate = ");
	index2 = headStr.indexOf(index, ";");
	sampleRate = headStr.substring(index + 20, index2).getIntValue();

	minShowSamples = sampleRate * 0.01;

	float *writePointer = displayBuffer->getWritePointer(0);
	//记录每段样本在文件中的位置，以及相应的大小，以便快速定位，显示
	while (offset < (fileLength - 1) && !threadShouldExit())
	{
		//wait(50);
		//each record contains 
		//one 64-bit timestamp, 
		//one 16-bit sample count (N), 
		//1 uint16 recordingNumber,
		//N 16-bit samples, 
		//and one 10-byte record marker (0 1 2 3 4 5 6 7 8 255)
		int64 ts = 0;
		uint16 samps = 0;
		int recordingNumber = 0;

		//fseek(file, offset, SEEK_SET);
		//fread(&ts, 8, 1, file);
		offset += 8;

		fseek(file, offset, SEEK_SET);
		fread(&samps, 2, 1, file);
		offset += 2;

		//fseek(file, offset, SEEK_SET);
		//fread(&recordingNumber, 2, 1, file);
		offset += 2;

		sampleIndex.push_back(offset);
		sampleCount.push_back(samps);
		if (totalSampleCount + samps > displayBuffer->getNumSamples())
		{
			displayBuffer->setSize(1, displayBuffer->getNumSamples() + BUFFER_BLOCK_SIZE, true, true);
			writePointer = displayBuffer->getWritePointer(0);
		}

		int16 *continuousDataIntegerBuffer = new int16[samps];
		float *continuousDataFloatBuffer = new float[samps];

		fseek(file, offset, SEEK_SET);
		fread(continuousDataIntegerBuffer, 2, samps, file);
		AudioDataConverters::convertInt16BEToFloat(continuousDataIntegerBuffer, continuousDataFloatBuffer, samps);
		for (int n = 0; n < samps; n++)
		{
			float value = *(continuousDataFloatBuffer + n) * float(0x7fff) * bitVolts;
			writePointer[totalSampleCount + n] = value;
			if (value < minVolt)
			{
				minVolt = value;
			}
			if (value > maxVolt)
			{
				maxVolt = value;
			}
		}
		totalSampleCount += samps;
		offset += samps * 2;
		delete continuousDataIntegerBuffer;
		delete continuousDataFloatBuffer;


		offset += 10;
	}
	startIndex = 0;
	endIndex = totalSampleCount - 1;
	endVolt = ceil(abs(jmax(abs(minVolt), abs(maxVolt))));
	startVolt = 0 - endVolt;

	const MessageManagerLock mml(Thread::getCurrentThread());
	if (!mml.lockWasGained())
	{
		return;
	}
	loadView->stop();
	loadView->setVisible(false);
	isProcessing = false;
	repaint();
}

void HistoryView::paint(Graphics &g)
{
	if (isProcessing)
	{
		return;
	}

	g.setColour(Colours::white);
	g.fillRect(x, y, w, h);

	float startSecond = (float)startIndex / (float)sampleRate;
	float endSecond = (float)endIndex / (float)sampleRate;

	g.setColour(Colours::black);
	g.drawLine(x + leftMargin, y + topMargin, x + leftMargin, y + topMargin + yAxisLength);
	g.drawLine(x + leftMargin, y + topMargin + yAxisLength, x + leftMargin + xAxisLength, y + topMargin + yAxisLength);

	//Y axis
	for (int i = 0; i < 101; i++)
	{
		if (i % 10 == 0)
		{
			g.drawText(String(endVolt - (endVolt - startVolt)*floor(i / 10) / 10.0) + "V",
				x, y + topMargin + yAxisLength  * i / 100.0 - 5,
				leftMargin, 10, Justification::centred, false);
		}
		if (i == 50)
		{
			g.setColour(Colours::lightgrey);
			g.drawLine(x + leftMargin, y + topMargin + yAxisLength / 2.0f, x + leftMargin + xAxisLength, y + topMargin + yAxisLength / 2.0);
			g.setColour(Colours::black);
		}
		if (i != 100)
		{
			g.drawLine(x + leftMargin, y + topMargin + yAxisLength  * i / 100.0,
				x + leftMargin + (i % 10 == 0 ? 10 : 5), y + topMargin + yAxisLength * i / 100.0,
				i % 10 == 0 ? 2 : 1);
		}
	}

	//X axis
	for (int i = 0; i < 101; i++)
	{
		if (i % 10 == 0)
		{
			String text = String((startIndex + (endIndex - startIndex)*floor(i / 10) / 10.0) / sampleRate, 4) + "s";
			g.drawText(text,
				x + leftMargin + xAxisLength * i / 100.0 - leftMargin, y + h - bottomMargin,
				leftMargin * 2, 20,
				Justification::centred, false);
		}
		if (i != 0)
		{
			g.drawLine(x + leftMargin + xAxisLength * i / 100.0, y + h - bottomMargin,
				x + leftMargin + xAxisLength * i / 100.0, y + h - bottomMargin - (i % 10 == 0 ? 10 : 5), i % 10 == 0 ? 2 : 1);
		}
	}


	//data
	g.setColour(Colours::blue);
	if (totalSampleCount > 0)
	{
		if (startPoint != 0 && endPoint != 0)
		{
			float dashLine[] = { 3, 3 };
			if (zoomMode == 1)
			{
				int x1 = jmin(startPoint->x, endPoint->x);
				int y1 = y + topMargin;
				int x2 = jmax(startPoint->x, endPoint->x);
				int y2 = y1 + yAxisLength - 1;
				g.setColour(Colours::blue);

				g.drawDashedLine(Line<float>(x1, y1, x1, y2), &dashLine[0], 2);
				g.drawDashedLine(Line<float>(x1, y1, x2, y1), &dashLine[0], 2);
				g.drawDashedLine(Line<float>(x2, y1, x2, y2), &dashLine[0], 2);
				g.drawDashedLine(Line<float>(x1, y2, x2, y2), &dashLine[0], 2);
			}
			else if (zoomMode == 2)
			{
				int x1 = x + leftMargin + 1;
				int y1 = jmin(startPoint->y, endPoint->y);
				int x2 = x1 + xAxisLength;
				int y2 = jmax(startPoint->y, endPoint->y);
				g.setColour(Colours::blue);

				g.drawDashedLine(Line<float>(x1, y1, x1, y2), &dashLine[0], 2);
				g.drawDashedLine(Line<float>(x1, y1, x2, y1), &dashLine[0], 2);
				g.drawDashedLine(Line<float>(x2, y1, x2, y2), &dashLine[0], 2);
				g.drawDashedLine(Line<float>(x1, y2, x2, y2), &dashLine[0], 2);
			}
		}
		g.setColour(Colours::black);
		float preX = -1;
		float preY = -1;
		const float *source = displayBuffer->getReadPointer(0, startIndex);
		int sampelCount = (endIndex - startIndex + 1);
		int showPixels = jmin(sampelCount, 1000);
		float unit = (float)sampelCount / (float)showPixels;//一个点多少个采样点
		if (unit < 1)
		{
			unit = 1;
			showPixels = sampelCount;
		}

		float yCenter = y + topMargin + yAxisLength / 2.0f;
		for (int i = 0; i <= showPixels; i++)
		{
			float curX = x + leftMargin + i * xAxisLength / (float)showPixels;

			int index = floor(i*unit);
			float aphla = i*unit - index;
			float value = source[index] * (1.0f - aphla) + source[index + 1] * aphla;
			float curY = yCenter - (value - (endVolt + startVolt) / 2.0f) / (endVolt - startVolt) * yAxisLength;
			if (preX >= 0)
			{
				g.drawLine(preX, preY, curX, curY);
			}
			preX = curX;
			preY = curY;
		}
	}
	return;
}

void HistoryView::buttonClicked(Button* b)
{
	if (b == xFangda)
	{
		if (zoomMode != 1 && (endIndex-startIndex+1) > minShowSamples)
		{
			zoomMode = 1;
			xFangda->setState(1);
			yFangda->setState(-1);
		}
		else
		{
			zoomMode = -1;
			xFangda->setState(-1);
			yFangda->setState(-1);

		}
	}
	else if (b == xSuoxiao)
	{
		int startIndex_ = startIndex;
		int endIndex_ = endIndex;
		int size = endIndex - startIndex + 1;

		startIndex_ -= size / 2;
		endIndex_ += size / 2;
		if (startIndex_ < 0)
		{
			startIndex_ = 0;
		}
		if (endIndex_ > totalSampleCount - 1)
		{
			endIndex_ = totalSampleCount - 1;
		}

		startIndex = startIndex_;
		endIndex = endIndex_;
		zoomMode = -1;
		xFangda->setState(-1);
		yFangda->setState(-1);
		repaint();
	}
	else if (b == yFangda)
	{
		if (zoomMode != 2)
		{
			zoomMode = 2;
			xFangda->setState(-1);
			yFangda->setState(1);
		}
		else
		{
			zoomMode = -1;
			xFangda->setState(-1);
			yFangda->setState(-1);

		}
	}
	else if (b == ySuoxiao)
	{
		float startVolt_ = startVolt;
		float endVolt_ = endVolt;
		float size = endVolt_ - startVolt_;
		startVolt_ = startVolt_ - size / 2.0f;
		endVolt_ = endVolt_ + size / 2.0f;

		float voltThredhold = ceil(abs(jmax(abs(minVolt), abs(maxVolt))));
		if (startVolt_ < 0 - voltThredhold)
		{
			startVolt_ = 0 - voltThredhold;
		}
		if (endVolt_ > voltThredhold)
		{
			endVolt_= voltThredhold;
		}

		startVolt = startVolt_;
		endVolt = endVolt_;
		zoomMode = -1;
		xFangda->setState(-1);
		yFangda->setState(-1);
		repaint();
	}
}

void HistoryView::mouseUp(const MouseEvent& event)
{
	if (startPoint != 0 && endPoint != 0)
	{
		if (zoomMode == 1)  //x axis zoom large
		{
			int size = endIndex - startIndex + 1;

			int size_ = size * abs(endPoint->x - startPoint->x)/ xAxisLength;
			if (size_ < minShowSamples)
			{
				size_ = minShowSamples;
				xFangda->setState(-1);
				zoomMode = -1;
			}
			int startIndex_ = startIndex + size * (jmin(endPoint->x,startPoint->x)-(x+leftMargin)) / xAxisLength;
			int endIndex_ = startIndex_ + size_ - 1;

			startIndex = startIndex_;
			endIndex = endIndex_;

			startPoint = 0;
			endPoint = 0;
			repaint();
		}
		else if (zoomMode == 2) // y axis zoom large
		{
			float size = endVolt - startVolt;
			float size_ = size * abs(endPoint->y - startPoint->y) / yAxisLength;

			float startVolt_ = startVolt + size * (y + topMargin + yAxisLength - jmax(endPoint->y, startPoint->y)) / yAxisLength;
			float endVolt_ = startVolt_ + size_;

			startVolt = startVolt_;
			endVolt = endVolt_;

			startPoint = 0;
			endPoint = 0;
			repaint();
		}
		
	}
}
void HistoryView::mouseDown(const MouseEvent& event)
{
	startPoint = new Point<int>(event.getPosition());
}
void HistoryView::mouseDrag(const MouseEvent& event)
{
	if (zoomMode == -1)
	{
		//左右拖拽 查看历史
		int offset = event.getPosition().x - startPoint->x;
		startPoint = new Point<int>(event.getPosition());

		int offsetSamples = offset * (endIndex - startIndex + 1) / xAxisLength;
		startIndex -= offsetSamples;
		endIndex -= offsetSamples;

		if (offset < 0) //向右移动
		{
			if (endIndex > totalSampleCount - 1)
			{
				int i = endIndex - (totalSampleCount - 1);
				endIndex = totalSampleCount - 1;
				startIndex -= i;
			}
		}
		else if (offset > 0) //向左移动
		{
			if (startIndex < 0)
			{
				int i = 0 - startIndex;
				startIndex = 0;
				endIndex += i;
			}
		}
		repaint();

	}
	else
	{
		//处理X轴，Y轴放大事件
		endPoint = new Point<int>(event.getPosition());
		repaint();
	}
}

/********************************/
/**************LoadView**********/
/********************************/
LoadView::LoadView()
{
}
LoadView::~LoadView()
{

}
void LoadView::resized()
{
}
void LoadView::start()
{
	startTimer(500);
}
void LoadView::stop()
{
	stopTimer();
}
void LoadView::timerCallback()
{
	count = count % 6 + 1;
	repaint();
}
void LoadView::paint(Graphics& g)
{
	g.setColour(Colours::green);
	g.fillAll();

	g.setColour(Colours::white);
	g.setFont(30);
	String str("Loading");
	for (int i = 0; i < count; i++)
	{
		str.append(String("."),1);
	}
	g.drawText(str, 0, 0, getWidth(), getHeight(), Justification::centred, false);
}

/********************************/
/**************ActionButton**********/
/********************************/
ActionButton::ActionButton(String name) : ImageButton()
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
		isButtonDown = true;
	}
	//g.setColour(isMouseOver || isButtonDown ? Colours::blue : Colours::black);
	//g.drawRect(-2, -2, getWidth()+4, getHeight()+4, 2);
	ImageButton::paintButton(g, isMouseOver, isButtonDown);
}
