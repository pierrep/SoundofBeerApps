#pragma once

#include "ofMain.h"

#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxOsc.h"
#include "ofxGui.h"
#include "ofxBpm.h"
//#include "ofxAvVideoPlayer.h"
#include "ofxMidi.h"
#include "ofxKalmanFilter.h"
#include "ofxBlobsManager.h"

#define USE_CAMERA //using directshow video player hangs after 5mins at the moment
//#define USE_GSTREAMER_JACK
//#define USE_AVCODEC

using namespace cv;
using namespace ofxCv;

class Grid {
public:
    float timer;
    bool trigger;
    ofVec2f value;
};


class ofApp : public ofBaseApp {

public:
    void setup();
    void update();
    void draw();
    void exit();  
    void reset();
    void keyPressed(int key);
    void scaleMovie(int scaleW,int scaleH);
    void drawAverageFlowGui(int x, int y);
    void setFlowParameters();
    void drawFlowGrid();
    void updateCameraParams();
    void drawFlowAsCircles(ofRectangle rect);
    void sendOSCParameters();
    void beatEvent();
    void processTrackPlayback();
    void setupMidi();
    void setupGui();
    void setupOsc();
	void setupSerial();
	void readRPISerial();
    void setupVideo();
    void drawVideo();
    void sendMidiParams();

#ifdef USE_CAMERA
    ofVideoGrabber vid;
#else
    #ifdef USE_GSTREAMER_JACK
        ofGstVideoUtils vid;
        ofTexture texture;
    #else
        #ifdef USE_AVCODEC
            ofxAvVideoPlayer vid;
        #else
            ofVideoPlayer vid;
        #endif
    #endif
#endif

    float vidW, vidH;
	float camWidth, camHeight;

    ofFbo               fbo;
	/* PBO optimisation */
	bool bUsePBO;
	ofBufferObject pixelBufferBack, pixelBufferFront;

    ofFbo               cvFbo;
    ofPixels            pix;
    ofPixels            cvpix;
    ofxCvGrayscaleImage greyImg;
    ofxCvColorImage     colImg;
    ofxCvContourFinder 	contourFinder;
    ofxBlobsManager     blobsManager;
    FlowFarneback flow; // ofxCv's dense optical flow analyzer (for whole image)
    
    vector<ofVec2f> frameFlows; // store average flow frame-by-frame
//    ofVec2f grid[20][20];
//    float   gridCounter[20][20];
//    bool    gridTrigger[20][20];
    Grid    grid[20][20];
    ofVec2f flowDirection; //per frame flow direction
    float   flowMagnitude; //per frame flow magnitude
    ofVec2f oldFlowDirection;
    float   flowRate;

    ofxOscSender sender;
    int port;
    string host;
    bool bTogglePlay;
	bool bShowDebug;
    
    ofxPanel gui;
    ofParameter<float> flowPyrScale, flowPolySigma, vidScale, lineWidth, flowScale, flowThreshold,flowGridTimer;
    ofParameter<int> flowLevels, flowIterations, flowPolyN, flowWinSize, flowGridResolution;
    ofParameter<bool> flowUseGaussian, loopVid, flowGrid, drawAsCircles, sendMidi;
    ofParameter<ofColor> averageColour;
    ofParameter<string> label;
    ofParameter<float> controllers[16];

    // Probabilities
    int numTracks;
    map<string,float> trackProbability;
    map<string,bool> trackPlay;
    map<string,int> trackIndex;

    ofxKalmanFilter flowDirSmoothed;
    ofxKalmanFilter flowRateSmoothed;

    bool bInitKalman;

//    float ambience;
//    float bass;
//    float guitar;
//    float elec_guitar;
//    float drums;
//    float fx;
//    float guitar_fx;

//    bool do_ambience;
//    bool do_bass;
//    bool do_guitar;
//    bool do_elec_guitar;
//    bool do_drums;
//    bool do_fx;
//    bool do_guitar_fx;

    int beat = 0;
    int bar = 1;
    int avg_bpm;

    ofxBpm bpm;

    ofxMidiOut midiOut;
    int midiChannel;

    void controllerMoved1(float & value);
    void controllerMoved2(float & value);
    void controllerMoved3(float & value);
    void controllerMoved4(float & value);
    void controllerMoved5(float & value);
    void controllerMoved6(float & value);
    void controllerMoved7(float & value);
    void controllerMoved8(float & value);
    void controllerMoved9(float & value);
    void controllerMoved10(float & value);
    void controllerMoved11(float & value);
    void controllerMoved12(float & value);
    void controllerMoved13(float & value);
    void controllerMoved14(float & value);
    void controllerMoved15(float & value);
    void controllerMoved16(float & value);

    ofTrueTypeFont font;
	ofSerial	serial;

};
