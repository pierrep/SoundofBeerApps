#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	ofBackground(40, 100, 40);

	// open an outgoing connection to HOST:PORT
	sender.setup(HOST, PORT);
    
    masterVolume.addListener(this, &ofApp::masterVolumeChanged);

    gui.setup("Panel");
    gui.add(masterVolume.set( "MasterVolume", 64, 0, 127 ));
    gui.add(track1Volume.set( "Track1Volume", 64, 0, 127 ));
    gui.add(track1Mute.set( "Track1Mute", false));

}

//--------------------------------------------------------------
void ofApp::update(){

    ofxOscMessage m;
    m.setAddress("/track/1/clip/9/launch");
    //m.addFloatArg(track1Volume.get());
    sender.sendMessage(m, false);

    ofxOscMessage m2;
    m2.setAddress("/track/1/mute");
    if(track1Mute.get()) {
        m2.addIntArg(1);
    } else {
        m2.addIntArg(0);
    }
    sender.sendMessage(m2, false);
}

//--------------------------------------------------------------
void ofApp::draw(){

    gui.draw();

}

//--------------------------------------------------------------
void ofApp::masterVolumeChanged(float & masterVol)
{
    ofxOscMessage m;
    m.setAddress("/master/volume");
    m.addFloatArg(masterVol);
    sender.sendMessage(m, false);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if(key == 'a' || key == 'A'){
		ofxOscMessage m;
		m.setAddress("/test");
		m.addIntArg(1);
		m.addFloatArg(3.5f);
		m.addStringArg("hello");
		m.addFloatArg(ofGetElapsedTimef());
		sender.sendMessage(m, false);
	}
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){
	ofxOscMessage m;
	m.setAddress("/mouse/position");
	m.addIntArg(x);
	m.addIntArg(y);
	sender.sendMessage(m, false);
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	ofxOscMessage m;
	m.setAddress("/mouse/button");
	m.addIntArg(button);
	m.addStringArg("down");
	sender.sendMessage(m, false);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	ofxOscMessage m;
	m.setAddress("/mouse/button");
	m.addIntArg(button);
	m.addStringArg("up");
	sender.sendMessage(m, false);

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}

