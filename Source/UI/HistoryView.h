/* �����鿴��ʷ���ݵ� */
#ifndef _HISTORY_VIEW_H_181227
#define _HISTORY_VIEW_H_181227

#include "../../JuceLibraryCode/JuceHeader.h"
#include <stdio.h>
class LoadView;
class ActionButton;

class HistoryView : public Component,public FilenameComponentListener,public Thread,public Button::Listener
{
public:
	HistoryView();
	~HistoryView();

	void resized() override;
	void filenameComponentChanged(FilenameComponent*) override;
	void run() override;
	void paint(Graphics&) override;
	void buttonClicked(Button*) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;

	void initData();
private:
	ScopedPointer<FilenameComponent> filenameComponent;
	ScopedPointer<LoadView> loadView;
	ScopedPointer<ActionButton> xFangda;
	ScopedPointer<ActionButton> xSuoxiao;
	ScopedPointer<ActionButton> yFangda;
	ScopedPointer<ActionButton> ySuoxiao;

	AudioSampleBuffer* displayBuffer;
	FILE* file;
	long fileLength;
	float bitVolts;
	int sampleRate;
	std::vector<long> sampleIndex;  //ÿ��sample���ļ��е�λ��
	std::vector<int> sampleCount; //ÿ��sample�Ĵ�С

	//����
	int zoomMode = -1; //-1 ɶ�¶�û��   //1  X�ᰴť�Ŵ�   //2 Y�ᰴť�Ŵ�
	//X�ᣬY��Ŵ�
	ScopedPointer<Point<int>> startPoint = 0;
	ScopedPointer<Point<int>> endPoint = 0;

	float x = 0, y = 40, w, h; //������ͼ�������С 
	float leftMargin = 60, rightMargin = 20, topMargin = 20, bottomMargin = 40;
	float xAxisLength;
	float yAxisLength;

	//ȷ��Y��
	float minVolt;
	float maxVolt; 

	float startVolt;
	float endVolt;

	long totalSampleCount;
	int startIndex;  //X����ʾ�Ŀ�ʼ������
	int endIndex;

	int minShowSamples;

	bool isProcessing = false;
	
};

class LoadView :public Component,public Timer
{
public:
	LoadView();
	~LoadView();
	void resized() override;
	void start();
	void stop();
	void timerCallback() override;
	void paint(Graphics&) override;
private:
	int count = 0;
};

class ActionButton : public ImageButton
{
public:
	ActionButton(String name);
	void setState(int state_)
	{
		state = state_;
		repaint();
	}
	void paintButton(Graphics&, bool isMouseOver, bool isButtonDown) override;
private:
	int state = -1;
};
#endif