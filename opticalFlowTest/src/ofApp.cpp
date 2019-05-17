#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofBackground(0);
    ofSetFrameRate(60);
    ofFill();

#ifdef USE_CAMERA
   // vid.setup(320,240);
#else
    vid.load("beer.mp4");
#endif

    vidW = vid.getWidth();
    vidH = vid.getHeight();
    
    // setup gui
    gui.setup();
    gui.add(flowPyrScale.set("Pyramid Scale", .5, 0, .99));
    gui.add(flowLevels.set("Pyramid Levels", 4, 1, 8));
    gui.add(flowIterations.set("Iterations", 2, 1, 8));
    gui.add(flowPolyN.set("Polynomial size", 7, 5, 10));
    gui.add(flowPolySigma.set("PolySigma", 1.5, 1.1, 2));
    gui.add(flowUseGaussian.set("Use Gaussian", true));
    gui.add(flowWinSize.set("Window size", 32, 4, 64));
    gui.add(loopVid.set("Loop video", true));
    gui.add(vidScale.set("Scale video", 0.5,0.1f, 3));
    gui.add(lineWidth.set("Draw line width", 4, 1, 10));
    gui.add(flowScale.set("Scale flow", 100,1, 100));
    gui.add(flowGrid.set("Flow grid", false));
    gui.add(flowGridResolution.set("Flow grid resolution", 15,2,20));
    gui.add(flowThreshold.set("Flow threshold", 50.0f,0.1f, 50.0f));
    gui.add(flowGridTimer.set("Flow grid timer", 50.0f,1.0f, 2000.0f));
    gui.add(averageColour.set(ofColor(0)));

    // start video
#ifndef USE_CAMERA
    vid.play();
    fbo.allocate(320,180,GL_RGB);
#endif

    reset();
}

//--------------------------------------------------------------
void ofApp::update(){
    updateCameraParams();

    vid.update();
    if (vid.isFrameNew()){
        
        setFlowParameters();
        
        // calculate flow
        scaleMovie(320,180);
        flow.calcOpticalFlow(pix);

        // save average flows per frame + per vid
        frameFlows.push_back(flow.getAverageFlow()*flowScale.get());
        
        vidFlowTotal += frameFlows.back();
        vidFlowAvg = vidFlowTotal / frameFlows.size();
        
        //per frame direction of flow
        flowDirection = flowDirection*0.95f +frameFlows.back().getNormalized()*0.05f;

        //per frame magnitude of flow
        flowMagnitude = frameFlows.back().length();

        float r = 0,g = 0,b = 0;
        float count = 0;
        for(unsigned int i = 0; i < pix.size();i+=3) {
            r += pix.getData()[i];
            g += pix.getData()[i+1];
            b += pix.getData()[i+2];
            count++;
        }
        averageColour.set(ofColor(r/count,g/count,b/count));

        // calculate dot position to show average movement
        dotPos += frameFlows.back();
        dotPath.addVertex(dotPos);

        if(flowGrid) {
            // Make grid of average flows
            for(int x = 0; x < flowGridResolution;x++) {
                for(int y = 0; y < flowGridResolution;y++) {
                    if(gridCounter[x][y] <= 0) {
                        grid[x][y] = flow.getTotalFlowInRegion(ofRectangle(x*320/flowGridResolution,y*180/flowGridResolution,320/flowGridResolution,180/flowGridResolution));
                        gridCounter[x][y] = flowGridTimer;
                    }
                    else {
                        gridCounter[x][y] -= (1.0f/ofGetFrameRate())*1000.0f;
                    }
                }
            }
        }

    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    // draw video and flow field
    ofSetColor(255);
    
    ofPushMatrix();
    ofTranslate(250,0); // make room for gui at left
    ofScale(vidScale, vidScale); // scale to vidScale gui choice
        vid.draw(0,0, vidW, vidH); // draw video at scale

        ofSetLineWidth(1); // draw thin flow field
        flow.draw(0,0, vidW, vidH); // draw flow field at scale

        drawFlowGrid();
        drawFlowPath();
    ofPopMatrix();
    
    // draw gui
    gui.draw();
    
    drawAverageFlowGui(120,550);
}

//--------------------------------------------------------------
void ofApp::scaleMovie(int scaleW,int scaleH)
{
    // take that texture and draw it into an FBO
    fbo.begin();
        vid.draw(0,0,scaleW,scaleH);
    fbo.end();

    fbo.readToPixels(pix);
}

//--------------------------------------------------------------
void ofApp::reset(){
    
    flow.resetFlow(); // clear ofxCv's flow cache
    
    // reset frameFlow tracking
    frameFlows.clear();
    vidFlowTotal.set(0,0);
    
    // moving dot
    dotPos = ofVec2f(vidW*0.5f, vidH*0.5f); // center of drawn video
    
    // polyline that tracks dot movement
    dotPath.clear();
    dotPath.addVertex(dotPos);
}

//--------------------------------------------------------------
void ofApp::drawAverageFlowGui(int x, int y)
{
    // draw average flow per frame and per video as lines
    if (frameFlows.size() > 0)
    { // only draw if there's been some flow

        ofPushMatrix();
        ofTranslate(x,y); // move below gui
            ofSetColor(250);
            ofDrawRectangle(-102,-102,204,204);
            ofSetColor(125);
            ofDrawRectangle(-100,-100,200,200);


            // draw avg magnitude + direction
            ofSetColor(255,0,0);
            ofDrawLine(0, 0, flowDirection.x*100, flowDirection.y*100); // scale up * 50 for visibility

            ofSetColor(0,255,0);
            ofDrawCircle(flowDirection.x*100, flowDirection.y*100,3);

            ofPopMatrix();
    }

    ofSetColor(255);
    ofDrawBitmapString("Flow dir: ( "+ofToString(flowDirection.x)+" "+ofToString(flowDirection.y)+")",10,y+125);
    ofDrawBitmapString("Flow magnitude: "+ofToString(flowMagnitude),10,y+140);

    // draw framerate at bottom of video
    ofDrawBitmapStringHighlight(ofToString((int) ofGetFrameRate()) + "fps", 250, vidH*vidScale);
}

//--------------------------------------------------------------
void ofApp::setFlowParameters()
{
    // Farneback flow parameters
    // more info at: http://docs.opencv.org/3.0-beta/modules/video/doc/motion_analysis_and_object_tracking.html#calcopticalflowfarneback

    flow.setPyramidScale(flowPyrScale);
        // 0.5 is classical image pyramid (scales image * 0.5 each layer)

    flow.setNumLevels(flowLevels); // number of levels of scaling pyramid
        // 1 means only original image is analyzed, no pyramid

    flow.setWindowSize(flowWinSize); // average window size
        // larger means robust to noise + better fast motion, but more blurred motion

    flow.setNumIterations(flowIterations); // # iterations of algorithm at each pyramid level

    flow.setPolyN(flowPolyN);
        // size of pixel neighborhood used for per-pixel polynomial expansion
        // larger means image will be approximated at smooth surfaces, more robust algorithm, more blurred motion

    flow.setPolySigma(flowPolySigma);
        // standard deviation of gaussian used to smooth derivates for polynomial expansion
        // for PolyN of 5, use PolySigma 1.1
        // for PolyN of 7, use PolySigma 1.5

    flow.setUseGaussian(flowUseGaussian);
        // use gaussian filter instead of box filter: slower but more accurate
        // normally use a larger window with this option
}

//--------------------------------------------------------------
void ofApp::drawFlowGrid()
{
    if(flowGrid) {
        // Make grid of average flows
        ofSetColor(255,255,255,100);
        for(int x = 0; x < flowGridResolution;x++) {
            for(int y = 0; y < flowGridResolution;y++) {
                if(grid[x][y].lengthSquared() > flowThreshold)
                {
                    ofDrawRectRounded(ofRectangle(x*vidW/flowGridResolution,y*vidH/flowGridResolution,vidW/flowGridResolution,vidH/flowGridResolution),0.5f);
                }
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::drawFlowPath()
{
    ofSetLineWidth(lineWidth); // draw other lines based on gui lineWidth
    // draw dot to track average flow
    ofSetColor(0,255,255);
    ofDrawCircle(dotPos, lineWidth*0.75f); // draw dot
    dotPath.draw(); // draw dot's path
}

//--------------------------------------------------------------
void ofApp::updateCameraParams()
{
#ifndef USE_CAMERA
    if (vid.getCurrentFrame() == 0) {
        reset();
        // will only do full reset if video is at beginning
        // doubles as an initialization
    }

    if (loopVid){
        vid.setLoopState(OF_LOOP_NORMAL);

        if (!vid.isPlaying()){
            vid.play(); // restart vid if it ended
        }

    } else {
        vid.setLoopState(OF_LOOP_NONE);

        if (vid.getIsMovieDone()){
            flow.resetFlow(); // reset flow cache in ofxCv to clear flow draw
        }
    }
#endif
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

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
