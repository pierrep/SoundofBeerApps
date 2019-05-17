#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofBackground(255,255,255);
	ofSetVerticalSync(true);

    movie.load("Segment_0001.mp4");
    movie.setLoopState(OF_LOOP_NORMAL);
    movie.play();

    fbo.allocate(320,240,GL_RGB);
}

//--------------------------------------------------------------
void ofApp::update(){
    ofSetWindowTitle(ofToString(ofGetFrameRate()));
    movie.update();
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofSetHexColor(0xFFFFFF);

    if(movie.isFrameNew()) {
        scaleMovie(320,240);
    }
    //movie.draw(0,0);
    frame.draw(0,0);

}

//--------------------------------------------------------------
void ofApp::scaleMovie(int scaleW,int scaleH)
{
    // take that texture and draw it into an FBO
    fbo.begin();
        movie.draw(0,0,scaleW,scaleH);
    fbo.end();

    //ofPixels pix;
    fbo.readToPixels(pix);
    frame.setFromPixels(pix);
}


//--------------------------------------------------------------
void ofApp::keyPressed  (int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}


//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

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
