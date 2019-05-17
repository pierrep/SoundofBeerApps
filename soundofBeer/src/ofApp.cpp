#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	camWidth = 1920;
	camHeight = 1080;

    ofBackground(0);
    ofSetFrameRate(30);
    ofFill();
	//ofSetLogLevel(OF_LOG_ERROR);

    setupOsc();
    setupMidi();
    setupVideo();
    setupGui();

    cvFbo.allocate(320,180,GL_RGB);
    colImg.allocate(320,180);
    flow.resetFlow();

    blobsManager.normalizePercentage = 0.5;
    blobsManager.giveLowestPossibleIDs = true;
    blobsManager.maxMergeDis = 5;
    blobsManager.maxUndetectedTime = 100;
    blobsManager.minDetectedTime = 100;
    blobsManager.debugDrawCandidates = false;

    /* Probabilities */
    numTracks = 8;
    avg_bpm = 110;

    trackProbability["ambience"] = 0.5f;
    trackProbability["guitar_oneshot"] = 0.2f;
    trackProbability["guitar_loops"] = 0.7f;
    trackProbability["mood_fx"] = 0.7f;
    trackProbability["textures"] = 0.1f;
    trackProbability["guitar_fx"] = 0.5f;
    trackProbability["guitar_tremolo"] = 0.5f;

    trackIndex["ambience"] = 1;
    trackIndex["guitar_oneshot"] = 2;
    trackIndex["guitar_loops"] = 3;
    trackIndex["mood_fx"] = 4;
    trackIndex["textures"] = 5;
    trackIndex["guitar_fx"] = 6;
    trackIndex["guitar_tremolo"] = 7;

    trackPlay["ambience"] = false;
    trackPlay["guitar_oneshot"] = false;
    trackPlay["guitar_loops"] = false;
    trackPlay["mood_fx"] = false;
    trackPlay["textures"] = false;
    trackPlay["guitar_fx"] = false;
    trackPlay["guitar_tremolo"] = false;

    ofAddListener(bpm.beatEvent, this, &ofApp::beatEvent);
    bpm.setBpm(avg_bpm);
    bpm.setBeatPerBar(4);
    bpm.start();

	setupSerial();
}

void ofApp::exit()
{
	ofxOscMessage m;
	m.clear();
	m.setAddress("/stop");
	m.addIntArg(1);
	sender.sendMessage(m, false);

	for (int i = 0; i < 127; i++) {
		ofxOscMessage m;
		m.setAddress("/vkb_midi/0/note/" + ofToString(i));
		m.addFloatArg(0); //velocity
		sender.sendMessage(m, false);
	}

	ofRemoveListener(bpm.beatEvent, this, &ofApp::beatEvent);
	bpm.stop();
	midiOut.closePort();
}

//--------------------------------------------------------------
void ofApp::beatEvent()
{
    beat++;
    if(beat > 4) {
        beat = 1;
        bar++;

        if(bar > 8) {
            bar = 1;

            for (std::map<string,float>::iterator itr=trackProbability.begin(); itr!=trackProbability.end(); ++itr) {
                if(ofRandomuf() < itr->second) trackPlay[itr->first] = true;
                else {
                    trackPlay[itr->first] = false;

                    /* stop track - may cut off sounds earlier */
                    ofxOscMessage m;
                    ofLogNotice() << itr->first << "\t\t: Stopping";
                    m.setAddress("/track/"+ofToString(trackIndex[itr->first])+"/clip/stop");
                    //sender.sendMessage(m, false);
                }
            }
        }
    }
   // ofLogNotice() << "Bar: " << bar << " Beat: " << beat << endl;
}

//--------------------------------------------------------------
void ofApp::processTrackPlayback()
{
    for (std::map<string,bool>::iterator itr=trackPlay.begin(); itr!=trackPlay.end(); ++itr) {
        if(itr->second) {
            itr->second = false;

            ofxOscMessage m;
            unsigned int clipnum = static_cast<unsigned int>(ofRandom(1,9));
            ofLogNotice() << itr->first << "\t\t: Clip num = " << clipnum;
            m.setAddress("/track/"+ofToString(trackIndex[itr->first])+"/clip/"+ofToString(clipnum)+ "/launch");
            sender.sendMessage(m, false);
        }
    }

//    if(do_ambience) {
//        do_ambience = false;

//        ofxOscMessage m;
//        unsigned int clipnum = ofRandom(1,9);
//        ofLogNotice() << "Clip num = " << clipnum;
//        m.setAddress("/track/1/clip/"+ofToString(clipnum)+ "/launch");
//        sender.sendMessage(m, false);
//    }

}

//--------------------------------------------------------------
void ofApp::update(){
    updateCameraParams();

#ifndef USE_AVCODEC
    vid.update();
    if (vid.isFrameNew())
#endif
    {
#ifdef USE_GSTREAMER_JACK
        texture.loadData(vid.getPixels().getData(),1920,1080, GL_RGB);
#endif

        setFlowParameters();

        // calculate flow
        scaleMovie(320,180);
        flow.calcOpticalFlow(pix);

        //average flow per frame
        ofVec2f frameFlow = flow.getAverageFlow()*flowScale.get();
        
        //per frame direction of flow
        oldFlowDirection = flowDirection;
        flowDirection = flowDirection*0.95f + frameFlow.getNormalized()*0.05f;

        flowRate = (flowDirection - oldFlowDirection).length() * 10.0f;
        if(flowRate > 1.0f) {
            //flowGrid = true;
        } else {
            //flowGrid = false;
        }

        //per frame magnitude of flow
        flowMagnitude = frameFlow.length();
		
        //average colour per frame
        float r = 0,g = 0,b = 0;
        float count = 0;
        for(unsigned int i = 0; i < pix.size();i+=4) {
            r += pix.getData()[i];
            g += pix.getData()[i+1];
            b += pix.getData()[i+2];
            count++;
        }
        averageColour.set(ofColor(r/count,g/count,b/count));
		

        // Make grid of average flows
        if(flowGrid) {
            for(int x = 0; x < flowGridResolution;x++) {
                for(int y = 0; y < flowGridResolution;y++) {
                    if(grid[x][y].timer <= 0) {
                        grid[x][y].value = flow.getTotalFlowInRegion(ofRectangle(x*320/flowGridResolution,y*180/flowGridResolution,320/flowGridResolution,180/flowGridResolution));
                        grid[x][y].timer = flowGridTimer;
                    }
                    else {
                        grid[x][y].timer -= (1.0f/ofGetFrameRate())*1000.0f;
                    }
                }
            }
        }

        // contour finding
        cvFbo.begin();
            ofClear(0,0,0,255);
            drawFlowAsCircles(ofRectangle(0,0,320,180));
        cvFbo.end();

        cvFbo.readToPixels(cvpix);
        colImg.setFromPixels(cvpix);
        greyImg = colImg;
        greyImg.blur();
        contourFinder.findContours(greyImg, 250, (320*180)/2, 20, false);
        blobsManager.update(contourFinder.blobs);

        sendOSCParameters();
    }

    bpm.setBpm(avg_bpm);

    processTrackPlayback();
    sendMidiParams();

	readRPISerial();
}

//--------------------------------------------------------------
void ofApp::draw(){

	if (bShowDebug) {
		drawVideo();
		gui.draw();
		drawAverageFlowGui(ofGetWidth() - 148, 655);
	}
	else {
		//ofSetWindowTitle(ofToString(ofGetFrameRate()));
		ofPushStyle();
		vid.draw(0, 0);
		if(ofGetFrameRate() < 29.9)
			ofSetColor(255, 0, 0);
		else
			ofSetColor(255, 255, 255);
		ofDrawBitmapString(ofToString(ofGetFrameRate()), 20, 20);
		ofPopStyle();
	}

}

//--------------------------------------------------------------
void ofApp::setupOsc()
{
    port = 8000;
    host = "localhost";
    sender.setup(host, port);
    bTogglePlay = true;

    ofxOscMessage m;
    m.clear();
    m.setAddress("/track/master/select");
    sender.sendMessage(m, false);

    m.clear();
    m.setAddress("/device/+");
    sender.sendMessage(m, false);
    sender.sendMessage(m, false);
    sender.sendMessage(m, false);

//    m.clear();
//    m.setAddress("/scene/1/launch");
//    sender.sendMessage(m, false);

    m.clear();
    m.setAddress("/refresh");
    sender.sendMessage(m, false);
}

//--------------------------------------------------------------
void ofApp::sendOSCParameters()
{
    ofxOscMessage m;
    m.setAddress("/tempo/raw");
    avg_bpm = (averageColour.get().r + averageColour.get().g + averageColour.get().b) / 3;
    m.addFloatArg(avg_bpm);
 //   sender.sendMessage(m, false);

//    m.clear();
//    m.setAddress("/device/param/1/value");
    int val = (flowDirection.x+1)*64;
//    m.addIntArg(val);
//    sender.sendMessage(m, false);

//    m.clear();
//    m.setAddress("/device/param/2/value");
    val = (flowDirection.y+1)*64;
//    m.addIntArg(val);
//    sender.sendMessage(m, false);

//    m.clear();
//    m.setAddress("/track/1/pan");
//    val = (flowDirection.y+1)*64;
//    m.addIntArg(val);
//    sender.sendMessage(m, false);

}

//--------------------------------------------------------------
void ofApp::sendMidiParams()
{
    if(sendMidi) {

        if (bInitKalman) {
             bInitKalman = false;
             flowDirSmoothed.setInitial(ofPoint(flowDirection.x,flowDirection.y));
             flowRateSmoothed.setInitial(ofPoint(flowRate,0));
        } else if (flowDirSmoothed.readyToTrack()) {
             flowDirSmoothed.update(ofPoint(flowDirection.x,flowDirection.y));
             flowRateSmoothed.update(ofPoint(flowRate,0));
        }

        // send flow direction
        {            
            float rate = ofClamp(flowRate*2,0,1);
            int x = (flowDirSmoothed.getPrediction().x+1)*64;
            x = ofClamp(x,0,127);
            int y = (flowDirSmoothed.getPrediction().y+1)*64;
            y = ofClamp(y,0,127);


            midiOut.sendControlChange(midiChannel, 17, x);
            midiOut.sendControlChange(midiChannel, 18, y);
            //midiOut.sendControlChange(midiChannel, 15, x);
            //midiOut.sendControlChange(midiChannel, 16, y);
        }
        {
            float rate = ofClamp(flowRate,0,1);
            int x = (flowDirection.x*rate+1)*64;
            x = ofClamp(x,0,127);
            int y = (flowDirection.y*rate+1)*64;
            y = ofClamp(y,0,127);
            midiOut.sendControlChange(midiChannel, 15, x);
            midiOut.sendControlChange(midiChannel, 16, y);
        }
        // distortion (flow rate)
        {
            int x = ofClamp(flowRateSmoothed.getPrediction().x,0,1) * 127;
            midiOut.sendControlChange(midiChannel, 14, x);
        }

        for (int i = 0; i < blobsManager.blobs.size(); i++){
            float x = (blobsManager.blobs[i].centroid.x / 320)*127;
            float y = (blobsManager.blobs[i].centroid.y / 180.0f);
            y = (1-y)*127;
            if(blobsManager.blobs[i].id < 8) {
                //midiOut.sendControlChange(midiChannel, blobsManager.blobs[i].id+3, x);
                midiOut.sendControlChange(midiChannel, blobsManager.blobs[i].id+4, y);
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::drawVideo()
{
    ofSetColor(255);
    ofPushMatrix();
    ofTranslate(275,5); // make room for gui at left
    ofScale(vidScale, vidScale); // scale to vidScale gui choice
#ifdef USE_GSTREAMER_JACK
        texture.draw(0,0, vidW, vidH); // draw video at scale
#else
#ifdef USE_AVCODEC
        vid.update();
#endif
        vid.draw(0,0, vidW, vidH); // draw video at scale
#endif

        ofSetLineWidth(1); // draw thin flow field
        if(drawAsCircles) {
            drawFlowAsCircles(ofRectangle(0,0,vidW,vidH));
        } else {
            flow.draw(0,0, vidW, vidH); // draw flow field at scale
        }

        if(flowGrid) {
            drawFlowGrid();
        } else {
           // cvFbo.draw(0,0,vidW,vidH);
            ofScale(6,6);
            ofPushStyle();
            for (int i = 0; i < contourFinder.nBlobs; i++){
                contourFinder.blobs[i].draw(0,0);
                font.drawString("blob: "+ofToString(i),contourFinder.blobs[i].centroid.x,contourFinder.blobs[i].centroid.y);
            }
            blobsManager.debugDraw(0, 0, 320, 180, 320, 180);
            ofPopStyle();
        }

    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::drawFlowAsCircles(ofRectangle rect)
{

    cv::Mat f = flow.getFlow();
    ofVec2f offset(rect.x,rect.y);
    ofVec2f scale(rect.width/f.cols, rect.height/f.rows);
    int stepSize = 4;
    for(int y = 0; y < f.rows; y += stepSize) {
        for(int x = 0; x < f.cols; x += stepSize) {
//            ofVec2f cur = ofVec2f(x, y) * scale + offset;
            ofDrawCircle(flow.getFlowPosition(x, y) * scale + offset,flow.getFlowOffset(x, y).length()*10);
        }
    }
}

//--------------------------------------------------------------
void ofApp::scaleMovie(int scaleW,int scaleH)
{
    // take that texture and draw it into an FBO
    fbo.begin();
#ifdef USE_GSTREAMER_JACK
        texture.draw(0,0,scaleW,scaleH);
#else
        vid.draw(0,0,scaleW,scaleH);
#endif
    fbo.end();

    fbo.readToPixels(pix);
}

//--------------------------------------------------------------
void ofApp::drawAverageFlowGui(int x, int y)
{
    // draw average flow per frame
    ofPushMatrix();
        ofTranslate(x,y); // move below gui
        ofSetColor(250);
        ofDrawRectangle(-102,-102,204,204);
        ofSetColor(125);
        ofDrawRectangle(-100,-100,200,200);

        ofSetColor(255,0,0);
        ofDrawLine(0, 0, flowDirection.x*100, flowDirection.y*100); // scale up * 50 for visibility

        ofSetColor(0,255,0);
        ofDrawCircle(flowDirection.x*100, flowDirection.y*100,3);

    ofPopMatrix();

    ofSetColor(255);
    ofDrawBitmapString("Flow dir: ( "+ofToString(flowDirection.x)+" "+ofToString(flowDirection.y)+")",x-100,y+125);
    ofDrawBitmapString("Flow magnitude: "+ofToString(flowMagnitude),x-100,y+140);
    ofDrawBitmapString("Flow rate: "+ofToString(flowRate),x-100,y+155);

    // draw framerate at bottom of video
    ofDrawBitmapStringHighlight(ofToString((int) ofGetFrameRate()) + "fps", 279, vidH*vidScale+20, ofColor::beige,ofColor::black);
    ofDrawBitmapStringHighlight("num blobs: "+ofToString(contourFinder.nBlobs), 340, vidH*vidScale+20, ofColor::beige,ofColor::black);
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
    // Make grid of average flows
    ofSetColor(255,255,255,100);
    for(int x = 0; x < flowGridResolution;x++) {
        for(int y = 0; y < flowGridResolution;y++) {
            if(grid[x][y].value.length() >= flowThreshold)
            {
                if(grid[x][y].trigger == false) {
                    grid[x][y].trigger = true;
                    //send midi on
                    ofxOscMessage m;
                    int val = 36 + y*(flowGridResolution) + x;
                    m.setAddress("/vkb_midi/0/note/"+ofToString(val));
                    m.addFloatArg(64); //velocity
                    sender.sendMessage(m, false);
                }
                ofDrawRectRounded(ofRectangle(x*vidW/flowGridResolution,y*vidH/flowGridResolution,vidW/flowGridResolution,vidH/flowGridResolution),0.5f);
            } else {

              if(grid[x][y].trigger == true) {
                  grid[x][y].trigger = false;
                  //send midi off note
                  ofxOscMessage m;
                  int val = 36 + y*(flowGridResolution) + x;
                  m.setAddress("/vkb_midi/0/note/"+ofToString(val));
                  m.addFloatArg(0); //velocity
                  sender.sendMessage(m, false);
              }

            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::updateCameraParams()
{
#ifndef USE_CAMERA
#ifndef USE_GSTREAMER_JACK
    if (vid.getCurrentFrame() == 0) {
        flow.resetFlow();
        // will only do full reset if video is at beginning
        // doubles as an initialization
    }
#endif

#ifndef USE_AVCODEC
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
#endif
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if(key == 'p') {
        bTogglePlay = !bTogglePlay;
        if(bTogglePlay)
        {
            ofxOscMessage m;
            m.clear();
            m.setAddress("/start");
            m.addIntArg(1);
            sender.sendMessage(m, false);
        }
        else {
            ofxOscMessage m;
            m.clear();
            m.setAddress("/stop");
            m.addIntArg(1);
            sender.sendMessage(m, false);
        }
    }

	if ((key == 'f') || (key == 'F')) {
		bShowDebug = !bShowDebug;
	}
	if (key == ' ') ofToggleFullscreen();

}

//--------------------------------------------------------------
void ofApp::controllerMoved1(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 3, intValue);
}

void ofApp::controllerMoved2(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 4, intValue);
}

void ofApp::controllerMoved3(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 5, intValue);
}

void ofApp::controllerMoved4(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 6, intValue);
}

void ofApp::controllerMoved5(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 7, intValue);
}

void ofApp::controllerMoved6(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 8, intValue);
}

void ofApp::controllerMoved7(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 9, intValue);
}

void ofApp::controllerMoved8(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 10, intValue);
}

void ofApp::controllerMoved9(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 11, intValue);
}

void ofApp::controllerMoved10(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 12, intValue);
}

void ofApp::controllerMoved11(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 13, intValue);
}

void ofApp::controllerMoved12(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 14, intValue);
}

void ofApp::controllerMoved13(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 15, intValue);
}

void ofApp::controllerMoved14(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 16, intValue);
}

void ofApp::controllerMoved15(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 17, intValue);
}

void ofApp::controllerMoved16(float & value)
{
    int intValue = value * 127;
    midiOut.sendControlChange(midiChannel, 18, intValue);
}

//--------------------------------------------------------------
void ofApp::setupGui()
{
    // setup gui
    gui.setup();
    flowPyrScale.set("Pyramid Scale", .5, 0, .99);
    flowLevels.set("Pyramid Levels", 2, 1, 8); // 4
    flowIterations.set("Iterations", 1, 1, 8); // 2
    flowPolyN.set("Polynomial size", 5, 5, 10); // 7
    flowPolySigma.set("PolySigma", 1.1, 1.1, 2); // 1.5
    flowUseGaussian.set("Use Gaussian", false); // true
    flowWinSize.set("Window size", 16, 4, 64); // 32

    gui.add(drawAsCircles.set("Draw as circles",false));
    gui.add(loopVid.set("Loop video", true));
    gui.add(vidScale.set("Scale video", 0.5,0.1f, 3));
    gui.add(lineWidth.set("Draw line width", 4, 1, 10));
    gui.add(flowScale.set("Scale flow", 100,1, 100));
    gui.add(flowGrid.set("Flow grid", true));
    gui.add(flowGridResolution.set("Flow grid resolution", 6,2,20));
    gui.add(flowThreshold.set("Flow threshold", 270.0f,0.1f, 500.0f));
    gui.add(flowGridTimer.set("Flow grid timer", 50.0f,1.0f, 2000.0f));
    gui.add(averageColour.set(ofColor(0)));
    gui.add(sendMidi.set("Send MIDI", true));

    controllers[0].addListener(this,&ofApp::controllerMoved1);
    controllers[1].addListener(this,&ofApp::controllerMoved2);
    controllers[2].addListener(this,&ofApp::controllerMoved3);
    controllers[3].addListener(this,&ofApp::controllerMoved4);
    controllers[4].addListener(this,&ofApp::controllerMoved5);
    controllers[5].addListener(this,&ofApp::controllerMoved6);
    controllers[6].addListener(this,&ofApp::controllerMoved7);
    controllers[7].addListener(this,&ofApp::controllerMoved8);
    controllers[8].addListener(this,&ofApp::controllerMoved9);
    controllers[9].addListener(this,&ofApp::controllerMoved10);
    controllers[10].addListener(this,&ofApp::controllerMoved11);
    controllers[11].addListener(this,&ofApp::controllerMoved12);
    controllers[12].addListener(this,&ofApp::controllerMoved13);
    controllers[13].addListener(this,&ofApp::controllerMoved14);
    controllers[14].addListener(this,&ofApp::controllerMoved15);
    controllers[15].addListener(this,&ofApp::controllerMoved16);

    gui.add(label.set("CONTROLLERS"));
    for(int i=0; i < 16;i++) {
        gui.add(controllers[i].set("Controller"+ofToString(i+3),0,0,1.0f));
    }

    font.load("verdana.ttf", 6, true, true);
	bShowDebug = true;
}

//--------------------------------------------------------------
void ofApp::setupVideo()
{
#ifdef USE_CAMERA
    vid.setup(camWidth,camHeight);
#else
    #ifdef USE_GSTREAMER_JACK
        //vid.setPipeline ("videotestsrc", OF_PIXELS_RGB, true, camWidth,camHeight);
        vid.setPipeline("filesrc location=./data/beer-long.mp4 ! qtdemux name=demux demux.audio_0 ! queue ! decodebin ! audioconvert ! audioresample ! jackaudiosink demux.video_0 ! queue ! decodebin ! videoconvert ! videoscale ", OF_PIXELS_RGB, true, camWidth,camHeight);
        vid.startPipeline();
    #else
        vid.load("beer.mp4");
    #endif
#endif
    vidW = vid.getWidth();
    vidH = vid.getHeight();

    // start video
#ifndef USE_CAMERA
#ifdef USE_GSTREAMER_JACK
    vid.play();
    texture.allocate(camWidth,camHeight, GL_RGB);
#else
    vid.setPaused(false);
    #ifdef USE_AVCODEC
        vid.setLoop(true);
    #endif
    vid.play();
#endif

#endif
	fbo.allocate(320, 180, GL_RGBA);

	bUsePBO = true;
	pixelBufferBack.allocate(320 * 180 * 4, GL_DYNAMIC_READ);
	pixelBufferFront.allocate(320 * 180 * 4, GL_DYNAMIC_READ);
}

//--------------------------------------------------------------
void ofApp::setupMidi()
{
    midiOut.listPorts();
    midiOut.openPort(1);
    midiChannel = 1;

    bInitKalman = true;
}

//--------------------------------------------------------------
void ofApp::setupSerial()
{
	serial.listDevices();
	vector <ofSerialDeviceInfo> deviceList = serial.getDeviceList();

	int baud = 115200;
	serial.setup(0, baud); //open the first device
						   //serial.setup("COM4", baud); // windows example
	serial.flush();
}

void ofApp::readRPISerial()
{
	unsigned char header[3] = { '0','0','0' };
	unsigned char buf[4];

	if (serial.available() > 0)
	{
		//ofLogNotice() << "serial available";
		if (serial.readBytes(header, 3) > 0)
		{
			//ofLogNotice() << "read header bytes";
			if ((header[0] == 'a') && (header[1] == 'b') && (header[2] == 'c'))
			{
				float f1, f2;

				//ofLogNotice() << "got header";
				//serial.readBytes((unsigned char*)&f1, sizeof(f1));
				//serial.readBytes((unsigned char*)&f2, sizeof(f2));
				if (serial.readBytes(buf, sizeof(float)) == sizeof(float))
				{
					memcpy(&f1, buf, sizeof(float));
					ofLogNotice() << "received float: " << f1;
				}
				if (serial.readBytes(buf, sizeof(float)) == sizeof(float))
				{
					memcpy(&f2, buf, sizeof(float));
					ofLogNotice() << "received float: " << f2;
				}

			}
		}

	}
}