#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include "ofxGui.h"

//#define HOST "192.168.0.19"
#define HOST "127.0.0.1"
#define PORT 8000

//--------------------------------------------------------
class ofApp : public ofBaseApp {

	public:

		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

        void masterVolumeChanged(float & masterVol);

		ofTrueTypeFont font;
		ofxOscSender sender;
        ofBuffer imgAsBuffer;
        ofImage img; 

        /* gui */
        ofxPanel gui;
        ofParameter<float> masterVolume;
        ofParameter<float> track1Volume;
        ofParameter<bool>   track1Mute;
};

