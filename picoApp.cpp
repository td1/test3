#include "picoApp.h"
#include "homography.h"

void picoApp::setup()
{
    ofSetLogLevel(OF_LOG_WARNING);
    ofSetLogLevel("ofThread", OF_LOG_WARNING);

    printf("SELF ADJUSTING PROJECTED VIDEO\n");

    startPlayVideo = false;
    string videoPath = ofToDataPath("../../../video/grid_640x480.mp4", true);

    omxPlayer.loadMovie(videoPath); 
    width = omxPlayer.getWidth();
    height = omxPlayer.getHeight();
    ofLog(OF_LOG_NOTICE, "omxPlayer movie loaded, widthxheight = %dx%d, WIDTHxHEIGHT default = %dx%d\n", width, height, WIDTH, HEIGHT);
    if (width != WIDTH || height != HEIGHT) {
    	ofLog(OF_LOG_NOTICE,"change to use width=%d, height=%d instead default values\n", width, height);
    }    
        
    ofBackground(0,0,0);
    consoleListener.setup(this);
    ofSetFrameRate(FRAME_RATE);
    
    captureConfig.width = CAPWIDTH;
    captureConfig.height = CAPHEIGHT;
    captureConfig.framerate = 30;
    captureConfig.isUsingTexture = true;
    captureConfig.enablePixels = true;
	
    captureVid.setup(captureConfig);
	if (!pixelOutput.isAllocated()) {
	    pixelOutput.allocate(width, height, GL_RGBA); 
	}
	
    captureImg.allocate(CAPWIDTH,CAPHEIGHT);
    grayCaptureImg.allocate(CAPWIDTH,CAPHEIGHT);
    grayCaptureImgSaved.allocate(CAPWIDTH,CAPHEIGHT);
    grayDiff.allocate(CAPWIDTH,CAPHEIGHT);

    ofo2.set(1,0,640,0,1,0,0,0,1);
	threshold = 90;
    ofHideCursor();
    sendBlobsEnable = false;
    bUpdateBackground = true;

    videoEnable = false;
    updatedMatrix = false;

    resyncMatrix[0] = 1; resyncMatrix[1] = 0; resyncMatrix[2] = 0; resyncMatrix[3] = 0;
    resyncMatrix[4] = 0; resyncMatrix[5] = 1; resyncMatrix[6] = 0; resyncMatrix[7] = 0;
    resyncMatrix[8] = 0; resyncMatrix[9] = 0; resyncMatrix[10]= 1; resyncMatrix[11]= 0;
    resyncMatrix[12]= 0; resyncMatrix[13]= 0; resyncMatrix[14]= 0; resyncMatrix[15]= 1;
}

void picoApp::update()
{
    omxPlayer.updatePixels();
    bool bNewFrame = false;
    ofBackground(0,0,0);
    bNewFrame = captureVid.isFrameNew();

    if (bNewFrame) {
    	captureImg.setFromPixels(captureVid.getPixels(), CAPWIDTH, CAPHEIGHT);
    	grayCaptureImg = captureImg;
    	if (bUpdateBackground == true) {
    		// at least update the first time
    		grayCaptureImgSaved = grayCaptureImg;
    		bUpdateBackground = false;
    		ofLog(OF_LOG_NOTICE, ">>>>> Update background the first time");
    	}
    	grayDiff.absDiff(grayCaptureImgSaved, grayCaptureImg);
    	grayDiff.threshold(80);
    	contourFinder.findContours(grayDiff, MIN_AREA, MAX_AREA, 20, false);
    	if (contourFinder.nBlobs) {
    		bUpdateBlobs = true;
    	}
    	grayCaptureImgSaved = grayCaptureImg;
    	nFrame ++;

    	// if (videoEnable == false)
    	sendBlobsEnable = (sendBlobsEnable == true ? false : true);
    }
}

void picoApp::draw(){
    int i,j,k;
    int var1, var2, nChannels;

#if 0 // TEST_RESYNC_HOMOGRAPHY
    // ofLog(OF_LOG_NOTICE, "test resync homography...");
    unsigned char *pixels = omxPlayer.getPixels();
    nChannels = 4; // omxPlayer.getPixelsRef().getNumChannels();

    src[0].set(0,0);
    src[1].set(640,0);
    src[2].set(640,480);
    src[3].set(0,480);

    if (setPositionByMouse) {
    	// ofLog(OF_LOG_NOTICE, "set new positions by mouse (%i,%i) (%i,%i) (%i,%i) (%i,%i)",dst[0],dst[1],dst[2],dst[3]);
    }
    else {
    	// ofLog(OF_LOG_NOTICE, "set positions by default");
    	dst[0].set(80,80);
    	dst[1].set(560,80);
    	dst[2].set(560,400);
    	dst[3].set(80,400);
    }
    ofSetHexColor(0x00FF00);
    ofDrawBitmapString("+", dst[0]);
    ofDrawBitmapString("+", dst[1]);
    ofDrawBitmapString("+", dst[2]);
    ofDrawBitmapString("+", dst[3]);
    ofSetHexColor(0xFFFFFF);
    getResyncHomography(src, dst, resyncMatrix);
    // for (i=0; i<16; i++)
    //    printf("%lf ", resyncMatrix[i]);
    // printf("\n");

    pixelOutput.loadData(pixels, width, height, GL_RGBA);
    glPushMatrix();
    glMultMatrixf(resyncMatrix);
    glTranslatef(0,0,0);
    pixelOutput.draw(0, 0, omxPlayer.getWidth(), omxPlayer.getHeight());
    glPopMatrix();
#endif

#if HOMOGRAPHY_TRANSFORM_ENABLE
    pixelOutput.loadData(pixels, width, height, GL_RGBA);
    glPushMatrix();
    glMultMatrixf(myMatrix);
    glTranslatef(0,0,0);    
    pixelOutput.draw(0, 0, omxPlayer.getWidth(), omxPlayer.getHeight());
    glPopMatrix();
#endif
    
#if TEST_RESYNC_CAPTURE
    // display detected blob positions
    int varx = 0;
    int vary = 0;

	int blobPosX[8];
    int blobPosY[8];

    for (i=0; i < contourFinder.nBlobs; i++) {
    	int blobX = contourFinder.blobs[i].centroid.x;
    	int blobY = contourFinder.blobs[i].centroid.y;

    	int blobA = contourFinder.blobs[i].area;
    	ofLog(OF_LOG_NOTICE, "blob[%d] = (%i,%i,%i)", i, blobX, blobY, blobA);

    	/* for debug only
    	int blobA = contourFinder.blobs[i].area;
    	ofLog(OF_LOG_NOTICE, "blob[%d] = (%i,%i,%i)", i, blobX, blobY, blobA);
    	ofSetHexColor(0xFF0000);
    	ofDrawBitmapString("+", contourFinder.blobs[i].centroid.x, contourFinder.blobs[i].centroid.y);
    	ofSetHexColor(0xFFFFFF); */
    	blobPosX[i] = blobX;
    	blobPosY[i] = blobY;
    }


    // videoEnable = false;
    if ( (contourFinder.nBlobs == 8 || contourFinder.nBlobs == 4) && updatedMatrix == false)  {

    	updateMatrix = true;
    	for (i=0; i < 8; i++) {
    		if (blobPosX[i] < 0 || blobPosY[i] < 0 || blobPosX[i] > 2000 || blobPosY[i] > 2000) {
    	    	printf("\n>>>>> blobPos are invalid...skip updating");
    	    	updateMatrix = false;
    	    	break;
    	    }
    	}

    	// printf("\nblobPos RAW = ");
    	// for (i=0; i < 8; i++) {
    	// 	printf("(%d,%d) ", blobPosX[i], blobPosY[i]);
    	// }
    	for (i=0; i<8; i++) {
            for (j=i+1; j<8; j++) {
                if (blobPosX[i]>blobPosX[j]) {
                    varx = blobPosX[i];
                    vary = blobPosY[i];
                    blobPosX[i] = blobPosX[j];
                    blobPosY[i] = blobPosY[j];
                    blobPosX[j] = varx;
                    blobPosY[j] = vary;
                }
            }
    	}

    	// printf("\nblobPos Sorted X = ");
    	// for (i=0; i < 8; i++) {
    	// 	printf("(%d,%d) ", blobPosX[i], blobPosY[i]);
    	// }
    	for (i=0; i<8; i+=2) {
    		if (blobPosY[i] > blobPosY[i+1]) {
    			varx = blobPosX[i];
    			vary = blobPosY[i];
    			blobPosX[i] = blobPosX[i+1];
    			blobPosY[i] = blobPosY[i+1];
    			blobPosX[i+1] = varx;
    			blobPosY[i+1] = vary;
    		}
    	}

    	/* getMatrixDistance to determine updating the homography matrix */
    	float distance = 0.0;
    	for (i=0; i<8; i++) {
    		blobPos[i].x = blobPosX[i];
    	    blobPos[i].y = blobPosY[i];
    	    distance += blobPos[i].squareDistance(blobPosSaved[i]);
    	    blobPosSaved[i] = blobPos[i];
    	}
    	printf("total distance: %5.2f", distance);

    	/*
    	distanceX = getMaxDistance(blobPosX, blobPosSavedX);
    	distanceY = getMaxDistance(blobPosX, blobPosSavedY);

    	printf("Max distanceX = %4.2f\n", distanceX);
    	printf("Max distanceY = %4.2f\n", distanceY);

    	if (distanceX > 100 || distanceY > 100) {
    		printf("X %4.2f\n", distanceY);
    		updateBlobs = false;
    	}
    	*/

    	/*printf("\nblobPos Sorted Y = ");
    	for (i=0; i < 8; i++) {
    	     printf("(%d,%d) ", blobPosX[i], blobPosY[i]);
    	}
    	printf("\n");*/

    	// TEST ONLY - UPDATE MATRIX ONLY ONCE
    	/*if (updateMatrix == true) {

    		updateMatrix = false;
    	}
    	else {
    		// videoEnable = false; // can be removed in the future?
    	}*/



    }
    else {
    	// printf("invalid number of blobs %d \n", totBlobs);
    	updateMatrix = false;
    }

    // TEST ONLY - WILL ENABLE VIDEO AGAIN AFTER TIMEOUT
    if (nFrame == 100) {
    	printf(">>>>> send out video after timeout for test at frame = %d\n", nFrame);
    	videoEnable = true;
        sendBlobsEnable = false;
    }


#endif

#if TEST_RESYNC_HOMOGRAPHY
    unsigned char *pixels = omxPlayer.getPixels();
    nChannels = 4; // omxPlayer.getPixelsRef().getNumChannels();



    switch (boardID) {
    	case ID_TD1:
    		// break;
    	case ID_TD2:
    		// ofo2.set(1,0,640,0,1,0,0,0,1);

    		if (updateMatrix == true) {
    			src[0].set(40,40);
    			src[1].set(40,440);
    			src[2].set(560,40);
    			src[3].set(560,440);
    			printf("src set1 = ");
    			for (i=0; i<4; i++)
    				printf("(%4.2f,%4.2f) ", src[i].x, src[i].y);
    			printf("\n");

    			dst[0].set(blobPosX[0],blobPosY[0]);
    			dst[1].set(blobPosX[1],blobPosY[1]);
    			dst[2].set(blobPosX[2],blobPosY[2]);
    			dst[3].set(blobPosX[3],blobPosY[3]);
    			printf("dst set1 = ");
    			for (i=0; i<4; i++)
    			    printf("(%4.2f,%4.2f) ", dst[i].x, dst[i].y);
    			printf("\n");

    			ofh1 = getResyncHomography3x3(src,dst);
    			ofh1inv = getResyncHomography3x3(dst,src);
//    			printf("h1 = ");
//    			for (i=0; i<9; i++)
//   			        printf("%4.2lf ", ofh1[i]);
//    			printf("\n");

    			src[0].set(120,40);
    			src[1].set(120,440);
    			src[2].set(640,40);
    			src[3].set(640,440);
    			printf("src set2 = ");
    			for (i=0; i<4; i++)
    				printf("(%4.2f,%4.2f) ", src[i].x, src[i].y);
    			printf("\n");

    			dst[0].set(blobPosX[4],blobPosY[4]);
    			dst[1].set(blobPosX[5],blobPosY[5]);
    			dst[2].set(blobPosX[6],blobPosY[6]);
    			dst[3].set(blobPosX[7],blobPosY[7]);
    			printf("dst set2 = ");
    			for (i=0; i<4; i++)
    			 	printf("(%4.2f,%4.2f) ", dst[i].x, dst[i].y);
    			printf("\n");

    			ofh2 = getResyncHomography3x3(src,dst);
    			ofh2inv = getResyncHomography3x3(dst,src);

//    			printf("h2inv = ");
//    			for (i=0; i<9; i++)
//   			       printf("%4.2lf ", ofh2inv[i]);
//    			printf("\n");

    			// Hc = ofh2inv*ofh1*ofo2;
    			// Hc = ofh2inv*ofh1;

    			if (boardID == ID_TD1) {
    				// Hc = ofh2inv*ofh1;
    				// resyncMatrix is unity
//    	   			resyncMatrix[0] = Hc[0]; resyncMatrix[1] = Hc[1]; resyncMatrix[2] = 0; resyncMatrix[3] = Hc[2];
//    	    		resyncMatrix[4] = Hc[3]; resyncMatrix[5] = Hc[4]; resyncMatrix[6] = 0; resyncMatrix[7] = Hc[5];
//    	    		resyncMatrix[8] = 0;     resyncMatrix[9] = 0;     resyncMatrix[10]= 0; resyncMatrix[11] = 0;
//    	    		resyncMatrix[12] = Hc[6];resyncMatrix[13] = Hc[7];resyncMatrix[14]= 0; resyncMatrix[15] = Hc[8];
    			}
    			else {
    				Hc = ofh2inv*ofh1;

      	   			resyncMatrix[0] = Hc[0]; resyncMatrix[1] = Hc[1]; resyncMatrix[2] = 0; resyncMatrix[3] = Hc[2];
     	    		resyncMatrix[4] = Hc[3]; resyncMatrix[5] = Hc[4]; resyncMatrix[6] = 0; resyncMatrix[7] = Hc[5];
      	    		resyncMatrix[8] = 0;     resyncMatrix[9] = 0;     resyncMatrix[10]= 0; resyncMatrix[11] = 0;
      	    		resyncMatrix[12] = Hc[6];resyncMatrix[13] = Hc[7];resyncMatrix[14]= 0; resyncMatrix[15] = Hc[8];
    			}

//    			printf("Hc = ");
//    			for (i=0; i<9; i++)
//    				printf("%4.2lf ", Hc[i]);
//    			printf("\n");

//    			resyncMatrix[0] = Hc[0]; resyncMatrix[1] = Hc[1]; resyncMatrix[2] = 0; resyncMatrix[3] = Hc[2];
//    			resyncMatrix[4] = Hc[3]; resyncMatrix[5] = Hc[4]; resyncMatrix[6] = 0; resyncMatrix[7] = Hc[5];
//    			resyncMatrix[8] = 0;     resyncMatrix[9] = 0;     resyncMatrix[10]= 0; resyncMatrix[11] = 0;
//    			resyncMatrix[12] = Hc[6];resyncMatrix[13] = Hc[7];resyncMatrix[14]= 0; resyncMatrix[15] = Hc[8];

    			printf(">>>>>>>>>>>>> Updated resyncMatrix = ");
    			updateMatrix = false;
    			// updatedMatrix = true;

    			for (i=0; i<16; i++)
    			    printf("%4.2lf ", resyncMatrix[i]);
    			printf("\n");


    		}
    		break;
    	case ID_TD3:
    	   	break;
    	case ID_TD4:
    	   	break;
    	default:;
    }

    // output
    // check to update reSyncMatrix
    if (videoEnable) {
    	updatedMatrix = true;

    	pixelOutput.loadData(pixels, width, height, GL_RGBA);
    	glPushMatrix();
    	glMultMatrixf(resyncMatrix);
    	glTranslatef(0,0,0);
    	pixelOutput.draw(0, 0, omxPlayer.getWidth(), omxPlayer.getHeight());
    	glPopMatrix();
    }
    else {
    	// printf("resyncMatrix is not ready, mute video\n");
    }
#endif

#if OMX_CAMERA
    // DRAW
    int varx = 0;
    int vary = 0;
	int blobPosX[8];
    int blobPosY[8];
    int blobPosA[8];
    int nBlobs = contourFinder.nBlobs;

    if (bUpdateBlobs) {
    	bUpdateBlobs = false;

    	// if ( (nBlobs == 8 || nBlobs == 4) && updatedMatrix == false)  {
    	if (nBlobs == 4)  { // need all 4 blobs detected to be valid

    		updateMatrix = true;

    		for (i=0; i < nBlobs; i++) {
    		    blobPosX[i] = contourFinder.blobs[i].centroid.x;
    		    blobPosY[i]  = contourFinder.blobs[i].centroid.y;
    		    blobPosA[i]  = contourFinder.blobs[i].area;
    		    // ofLog(OF_LOG_NOTICE, "blob[%d] = (%i,%i,%i)", i, blobX, blobY, blobA);
    		}

    		for (i=0; i < nBlobs; i++) {
    			if (blobPosX[i] < 0 || blobPosY[i] < 0 || blobPosX[i] > 2000 || blobPosY[i] > 2000) {
    				updateMatrix = false;
    				printf("\n>>>>> blobPos are invalid...updateMatrix = %d\n");
    				break;
    			}
    		}

    		if (updateMatrix == true) {
    			printf("\nframe[%d]:", nFrame);
    			for (i=0; i < nBlobs; i++) {
    				printf("(%d %d %d)",blobPosX[i],blobPosY[i],blobPosA[i]);
    			}
    			printf("\n");
    		}

    		for (i=0; i<nBlobs; i++) {
    			for (j=i+1; j<nBlobs; j++) {
    				if (blobPosX[i]>blobPosX[j]) {
    					varx = blobPosX[i];
    					vary = blobPosY[i];
    					blobPosX[i] = blobPosX[j];
    					blobPosY[i] = blobPosY[j];
    					blobPosX[j] = varx;
    					blobPosY[j] = vary;
    				}
    			}
    		}

    		for (i=0; i<nBlobs; i+=2) {
    			if (blobPosY[i] > blobPosY[i+1]) {
    				varx = blobPosX[i];
    				vary = blobPosY[i];
    				blobPosX[i] = blobPosX[i+1];
    				blobPosY[i] = blobPosY[i+1];
    				blobPosX[i+1] = varx;
    				blobPosY[i+1] = vary;
    			}
    		}

    		if (updateMatrix == true) {
        		printf("\nFRAME[%d]:", nFrame);
        		for (i=0; i < nBlobs; i++) {
        			printf("(%d %d)",blobPosX[i],blobPosY[i]);
        		}
        		printf("\n");
        	}

    		/* getMatrixDistance to determine updating the homography matrix */
    		float distance = 0.0;
    		for (i=0; i<nBlobs; i++) {
    			blobPos[i].x = blobPosX[i];
    			blobPos[i].y = blobPosY[i];
    			distance += blobPos[i].squareDistance(blobPosSaved[i]);
    		}

    		// if (distance < 1000 && updateMatrix == true) {
    		if (updateMatrix == true) { // SKIP CHECKING DISTANCE
    			printf("distance: %5.2f\n", distance);
//    			printf("\nFRAME[%d]:", nFrame);
    			for (i=0; i<nBlobs; i++) {
    				blobPosSaved[i] = blobPos[i];
//    				printf("(%d %d %d)",blobPosSaved[i].x,blobPosSaved[i].y);
    			}
//    			printf("\n");
    		}
    		else {
    			updateMatrix = false;
       		}
    	}
    	else {
    		updateMatrix = false;
    	}
    }

    if (nFrame == 50) {
    	printf(">>>>> send out video after timeout for test at frame = %d\n", nFrame);
    	videoEnable = true;
        sendBlobsEnable = false;
    }

    unsigned char *pixels = omxPlayer.getPixels();
    nChannels = 4;

	if (updateMatrix == true) {
		src[0].set(120,40);
		src[1].set(120,440);
		src[2].set(560,40);
		src[3].set(560,440);
		dst[0].set(blobPosSaved[0].x,blobPosSaved[0].y);
		dst[1].set(blobPosSaved[1].x,blobPosSaved[1].y);
		dst[2].set(blobPosSaved[2].x,blobPosSaved[2].y);
		dst[3].set(blobPosSaved[3].x,blobPosSaved[3].y);
		ofh1 = getResyncHomography3x3(src,dst);
		ofh1inv = getResyncHomography3x3(dst,src);

		Hc = ofh1inv;

		resyncMatrix[0] = Hc[0]; resyncMatrix[1] = Hc[1]; resyncMatrix[2] = 0; resyncMatrix[3] = Hc[2];
		resyncMatrix[4] = Hc[3]; resyncMatrix[5] = Hc[4]; resyncMatrix[6] = 0; resyncMatrix[7] = Hc[5];
		resyncMatrix[8] = 0;     resyncMatrix[9] = 0;     resyncMatrix[10]= 0; resyncMatrix[11] = 0;
		resyncMatrix[12] = Hc[6];resyncMatrix[13] = Hc[7];resyncMatrix[14]= 0; resyncMatrix[15] = Hc[8];

		printf(">>>>>>>>>>>>> Updated resyncMatrix = ");
		updateMatrix = false;

		for (i=0; i<16; i++)
			printf("%4.2lf ", resyncMatrix[i]);
		printf("\n");
	}

    //unsigned char *capPixels = captureVid.getPixels(); // NOTE: getPixels at draw first, should be at update
    //nChannels = 4;
    // pixelOutput.loadData(capPixels, 640, 480, GL_RGBA);

    /*glPushMatrix();
    glMultMatrixf(resyncMatrix);
    glTranslatef(0,0,0);
	pixelOutput.draw(0, 0, 640, 480);
	glPopMatrix();
	*/


#if DISPLAY_CAPTURE_IMG_FOR_DEBUG
    captureImg.draw(0,0,320,240);
    grayCaptureImg.draw(320,0,320,240);
    grayDiff.draw(0,240,320,240);
    contourFinder.draw(320,240,320,240);
#endif
    // HUNG2
    // Display captureImg for debug
    // captureImg.draw(0,0,640,480);
    // contourFinder.draw(0,0,640,480);

#if 1 // WORKING DISPLAY omxplayer
    pixelOutput.loadData(pixels, width, height, GL_RGBA);
    updatedMatrix = false;
    glPushMatrix();
    glMultMatrixf(resyncMatrix);
    glTranslatef(0,0,0);
    pixelOutput.draw(0, 0, 640, 480); 
    glPopMatrix();
#endif

#endif // OMX_CAMERA

#if NO_HOMOGRAPHY_TRANFORM
    unsigned char *pixels = omxPlayer.getPixels();
    nChannels = 4; // omxPlayer.getPixelsRef().getNumChannels();

    pixelOutput.loadData(pixels, width, height, GL_RGBA);
    pixelOutput.draw(0, 0, omxPlayer.getWidth(), omxPlayer.getHeight());
#endif

    /* FILL BLOBS */
    int HOFFSET = 80;
    int VOFFSET = 20;
    int BLOBRADIUS = 10;

    ofFill();
    ofSetHexColor(0x000000);
    ofRect(HOFFSET,0,2*VOFFSET,2*VOFFSET);
    ofRect(WIDTH-HOFFSET-2*VOFFSET,0,2*VOFFSET,2*VOFFSET);
    ofRect(HOFFSET,HEIGHT-2*VOFFSET,2*VOFFSET,2*VOFFSET);
    ofRect(WIDTH-HOFFSET-2*VOFFSET,HEIGHT-2*VOFFSET,2*VOFFSET,2*VOFFSET);

    ofSetHexColor(0xFFFFFF);
    if (sendBlobsEnable == true) {
	ofCircle(HOFFSET+VOFFSET,VOFFSET,BLOBRADIUS);  
	ofCircle(WIDTH-HOFFSET-VOFFSET,VOFFSET,BLOBRADIUS);  
	ofCircle(HOFFSET+VOFFSET,HEIGHT-VOFFSET,BLOBRADIUS);  
	ofCircle(WIDTH-HOFFSET-VOFFSET,HEIGHT-VOFFSET,BLOBRADIUS);  
    }
    drawFrame ++;

#if 0
    stringstream info;
    info <<"\n" << "output frame rate: " << ofGetFrameRate() << "drawFrame: " << drawFrame << "\n";
    info << "Player: " << omxPlayer.getWidth() << "x" << omxPlayer.getHeight() << " @ "<< omxPlayer.getFPS() << "fps"<< "\n";
    info << "Camera: " << captureVid.getWidth() << "x" << captureVid.getHeight() << " @ "<< captureVid.getFrameRate() << "fps"<< "\n";
    ofDrawBitmapStringHighlight(info.str(), 60, 60, ofColor(ofColor::black, 90), ofColor::green);
#endif
    
    /////////////////////////////////////////////
    // Display for testing only
    /////////////////////////////////////////////
#if 0
    // if (nFrame > 100) {
    // grayBackground.drawROI(80,80,640-160,480-160);
    // grayDiff.drawROI(80,80,640-160,480-160);
    grayCaptureImg.drawROI(80,80,640-160,480-160);
    	// contourFinder.draw(80,80);
    // }
    // else {
        // ofFill();
        // ofSetHexColor(0x000000);
        // ofRect(80,80,640-160,480-160);
    // }
#endif
}

void picoApp::keyPressed  (int key)
{
	switch (key) {
		case 'p':
			printf("toggle pause, current pause = %d\n",omxPlayer.isPaused());
			omxPlayer.togglePause();
			// pauseFlag = ~pauseFlag;
			// if (pauseFlag)
				// omxPlayer.setPaused(!omxPlayer.isPaused());
				// OMXClock::OMXPause(true);
			// else
				// omxPlayer.togglePaused();
				// OMXClock::OMXResume(true);
		break;
	}





#if	TEST_RESYNC_CAPTURE
	switch (key) {
		case ' ':
    		bUpdateBackground = true;
    		break;
    	case '+':
    		threshold ++;
    		if (threshold > 255) threshold = 255;
    		break;
    	case '-':
    		threshold --;
    		if (threshold < 0) threshold = 0;
    		break;
	}
#endif
}

void picoApp::onCharacterReceived(SSHKeyListenerEventData& e)
{
    keyPressed((int)e.character);
}

void picoApp::readMatrix2(char* filename)
{
    FILE *matp;
    int i,j;

    matp = fopen(filename, "r");
    if (matp == NULL)
    {
        fputs("Matrix file read error.\n", stderr);
        return;
    }
	
    printf("Matrix 3x3: ");
    for (i=0; i<3; i++) {
	for (j=0; j<3; j++) {
            if (!fscanf(matp, "%f", &matrix[i][j])) {
                fputs("Matrix file read error: 3x3 matrix parameters\n", stderr);
                return;
            }
            else {
                printf("%lf ", matrix[i][j]);
            }
        }
    }            
    printf("\n");
    
    printf("Mapping Matrix 3x3 to Matrix 4x4...\n");
    myMatrix[0] = matrix[0][0]; myMatrix[1] = matrix[1][0]; myMatrix[2] = 0; myMatrix[3] = matrix[2][0];
    myMatrix[4] = matrix[0][1]; myMatrix[5] = matrix[1][1]; myMatrix[6] = 0; myMatrix[7] = matrix[2][1];
    myMatrix[8] = 0;            myMatrix[9] = 0;            myMatrix[10]= 0; myMatrix[11]= 0;
    myMatrix[12]= matrix[0][2]; myMatrix[13]= matrix[1][2]; myMatrix[14]= 0; myMatrix[15]= matrix[2][2];
    
    for (i=0; i<16; i++)
        printf("%lf ", myMatrix[i]);

#ifdef ENABLE_BLENDING
    printf("\nhtlx,htly,hblx,hbly,hbrx,hbry,htrx,htry : \n");
    if (!fscanf(matp, "%lf %lf %lf %lf %lf %lf %lf %lf", &htlx, &htly, &hblx, &hbly, &htrx, &htry, &hbrx, &hbry)) {
        fputs("Matrix file read error: 8 horizontal parameters\n", stderr);
        return;
    }
    printf("%lf %lf %lf %lf %lf %lf %lf %lf \n", htlx, htly, hblx, hbly, htrx, htry, hbrx, hbry);
        
    if (!fscanf(matp, "%lf %lf %lf %lf %lf %lf %lf %lf", &vtlx, &vtly, &vblx, &vbly, &vtrx, &vtry, &vbrx, &vbry)) {
        fputs("Matrix file read error: 8 vertical parameters\n", stderr);
        return;
    }

    /* no needed to prevent singularity for topSlope and bottomSlope */
    topSlope = (vtly-vtry)/(vtlx-vtrx);
    // topSlope = 0.0;
    bottomSlope = (vbly-vbry)/(vblx-vbrx);
    // bottomSlope = 0.0; 
    
    // prevent singularity
    if (htlx == hblx) {
        leftSlope = 100000; 
        leftSlopeInv = 0;
    }
    else {
        leftSlope = (htly-hbly)/ (htlx-hblx);
        leftSlopeInv = (htlx-hblx)/(htly-hbly);
    }
    
    if (htrx == hbrx) {
        rightSlope = 100000;
        rightSlopeInv = 0;
    }
    else {
        rightSlope = (htry-hbry)/(htrx-hbrx);
        rightSlopeInv = (htrx-hbrx)/(htry-hbry);
    }
    
    printf("Top Slope: %lf\n", topSlope);
    printf("Bottom Slope: %lf\n", bottomSlope);
    printf("Left Slope: %lf\n", leftSlope);
    printf("Right Slope: %lf\n", rightSlope);
    printf("Left Slope Inversed: %lf\n", leftSlopeInv);
    printf("Right Slope Inversed: %lf\n", rightSlopeInv);
#endif
    
    fclose(matp);
}

//--------------------------------------------------------------
void picoApp::readMatrix(char* filename)
{
    FILE *matp;
    int i,j;

    matp = fopen(filename, "r");
    if (matp == NULL)
    {
        fputs("Matrix file read error.\n", stderr);
        return;
    }
	
    printf("Matrix 3x3: ");
    for (i=0; i<3; i++) {
	for (j=0; j<3; j++) {
            if (!fscanf(matp, "%f", &matrix[i][j])) {
                fputs("Matrix file read error: 3x3 matrix parameters\n", stderr);
                return;
            }
            else {
                printf("%lf ", matrix[i][j]);
            }
        }
    }            
    printf("\n");
    
    printf("Mapping Matrix 3x3 to Matrix 4x4...\n");
    myMatrix[0] = matrix[0][0]; myMatrix[1] = matrix[1][0]; myMatrix[2] = 0; myMatrix[3] = matrix[2][0];
    myMatrix[4] = matrix[0][1]; myMatrix[5] = matrix[1][1]; myMatrix[6] = 0; myMatrix[7] = matrix[2][1];
    myMatrix[8] = 0;            myMatrix[9] = 0;            myMatrix[10]= 0; myMatrix[11]= 0;
    myMatrix[12]= matrix[0][2]; myMatrix[13]= matrix[1][2]; myMatrix[14]= 0; myMatrix[15]= matrix[2][2];
    
    for (i=0; i<16; i++)
        printf("%lf ", myMatrix[i]);

    printf("\nhtlx,htly,hblx,hbly,hbrx,hbry,htrx,htry : \n");
    if (!fscanf(matp, "%lf %lf %lf %lf %lf %lf %lf %lf", &htlx, &htly, &hblx, &hbly, &htrx, &htry, &hbrx, &hbry)) {
        fputs("Matrix file read error: 8 horizontal parameters\n", stderr);
        return;
    }
    printf("%lf %lf %lf %lf %lf %lf %lf %lf \n", htlx, htly, hblx, hbly, htrx, htry, hbrx, hbry);
        
    if (!fscanf(matp, "%lf %lf %lf %lf %lf %lf %lf %lf", &vtlx, &vtly, &vtrx, &vtry, &vblx, &vbly, &vbrx, &vbry)) {
        fputs("Matrix file read error: 8 vertical parameters\n", stderr);
        return;
    }

    // topSlope = (vtly-vtry)/(vtlx-vtrx);
    topSlope = 0.0;
    // bottomSlope = (vbly-vbry)/(vblx-vbrx);
    bottomSlope = 0.0; 
    
    // prevent singularity
    if (htlx == hblx) {
        leftSlope = 100000; 
        leftSlopeInv = 0;
    }
    else {
        leftSlope = (htly-hbly)/ (htlx-hblx);
        leftSlopeInv = (htlx-hblx)/(htly-hbly);
    }
    
    if (htrx == hbrx) {
        rightSlope = 100000;
        rightSlopeInv = 0;
    }
    else {
        rightSlope = (htry-hbry)/(htrx-hbrx);
        rightSlopeInv = (htrx-hbrx)/(htry-hbry);
    }
    
    printf("Top Slope: %lf\n", topSlope);
    printf("Bottom Slope: %lf\n", bottomSlope);
    printf("Left Slope: %lf\n", leftSlope);
    printf("Right Slope: %lf\n", rightSlope);
    printf("Left Slope Inversed: %lf\n", leftSlopeInv);
    printf("Right Slope Inversed: %lf\n", rightSlopeInv);

    fclose(matp);
}

int picoApp::getRightX(int y)
{
    // xOverlapRight[y] = (y-htry)/(rightSlope)+htrx;
    xOverlapRight[y] = (y-htry)*(rightSlopeInv)+htrx;
    // printf("xR[%d]=%f \n", y, xOverlapRight[y]);
    if(xOverlapRight[y] > WIDTH-1) 
        xOverlapRight[y] = WIDTH-1; 
    return xOverlapRight[y];
} 

int picoApp::getLeftX(int y)
{
    // xOverlapLeft[y] = (y-htly)/(leftSlope)+htlx;
    xOverlapLeft[y] = (y-htly)*(leftSlopeInv)+htlx;
    
    // printf("xL[%d]=%f \n", y, xOverlapLeft[y]);
    if(xOverlapLeft[y] < 0)
        xOverlapLeft[y] = 0;
    return xOverlapLeft[y];
}

int picoApp::getTopY(int x)
{
    yOverlapTop[x] = topSlope*(x-vtrx)+vtry;
    // printf("yT[%d]=%f \n", x, yOverlapTop[x]);
    if(yOverlapTop[x] < 0)
        yOverlapTop[x] = 0;
    return yOverlapTop[x];
}

int picoApp::getBottomY(int x)
{
    yOverlapBottom[x] = bottomSlope*(x-vbrx)+vbry;
    // printf("yB[%d]=%f \n", x, yOverlapBottom[x]);
    if(yOverlapBottom[x] > HEIGHT-1)
        yOverlapBottom[x] = HEIGHT-1;
    return yOverlapBottom[x];
}

double picoApp::getXFade(int x, int y)
{
    double x1,x2,result;
    double result2,gamma;

    if (fadeRight)	// Projector 1, 3
    {
        x2 = getRightX(y)-x;
        x1 = x - getLeftX(y);
    }
    else	// Projector 2, 4
    {
        x2 = x-getLeftX(y);
        x1 = getRightX(y) - x;
    }
    // remove adjustment result = ((x2+0.5*x1)/(x2+x1));
    result = x2/(x2+x1);

    #if 1 // turn on option to do gamma correction
    /* add gamma correction */
    gamma = 5.5;
    result2 = pow(result,1/gamma);
    // printf("getXFade: %lf\n", result2);
    return (result2 >= 0 && result2 <= 1)? result2:1;
    #else
    return result;
    #endif
}

double picoApp::getYFade(int x, int y)
{
    double y1,y2,result;
    double gamma, result2;

    if(fadeDown == true)	// Projector 1, 2
    {
            y2 = getBottomY(x)-y;
            y1 = y - getTopY(x);
    }
    else	// Projector 3, 4
    {
            y2 = y - getTopY(x);
            y1 = getBottomY(x) - y;
    }
    result = ((y2)/(y2+y1));

#if 1
    /* add gamma correction */
    gamma = 3.2;
    result2 = pow(result,1/gamma);

    // printf("*** getYFade[%d,%d]=%lf %lf\n", x, y, result, result2);	
    if(result > 0 && result <= 1)
    {
        /*cout << "Pixel: " << x << "," << y << endl;
        cout << "Top bound: " << getTopY(x) << endl;
        cout << "Bottom bound: " << getBottomY(x) << endl;
        cout << "Y1: " << y1 << endl;
        cout << "Y2: " << y2 << endl;
        cout << "Factor: " << result << endl;*/
    }
    return (result >= 0 && result <= 1) ? result2 : 1;
#else
    return result;
#endif
}

/* allocate a double matrix with subscript range m[nrl..nrh][ncl..nch] */
double **dmatrix(int nrl, int nrh, int ncl, int nch)
{
	int i,nrow=nrh-nrl+1,ncol=nch-ncl+1;
	double **m;
	/* allocate pointers to rows */
	m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));
	m += NR_END;
	m -= nrl;
	/* allocate rows and set pointers to them */
	m[nrl]=(double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
	m[nrl] += NR_END;
	m[nrl] -= ncl;
	for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;
	/* return pointer to array of pointers to rows */
	return m;
}

double *dvector(int nl, int nh)
/* allocate a double vector with subscript range v[nl..nh] */
{
	double *v;
	v=(double *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(double)));
	return v-nl+NR_END;
}

void free_dvector(double *v, int nl, int nh)
/* free a double vector allocated with dvector() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

double pythag(double a, double b)
/* compute (a2 + b2)^1/2 without destructive underflow or overflow */
{
	double absa,absb;
	absa=fabs(a);
	absb=fabs(b);
	if (absa > absb) return absa*sqrt(1.0+(absb/absa)*(absb/absa));
	else return (absb == 0.0 ? 0.0 : absb*sqrt(1.0+(absa/absb)*(absa/absb)));
}

/******************************************************************************/
void svdcmp(double **a, int m, int n, double w[], double **v)
/*******************************************************************************
Given a matrix a[1..m][1..n], this routine computes its singular value
decomposition, A = U.W.VT.  The matrix U replaces a on output.  The diagonal
matrix of singular values W is output as a vector w[1..n].  The matrix V (not
the transpose VT) is output as v[1..n][1..n].
*******************************************************************************/
{
	int flag,i,its,j,jj,k,l,nm;
	double anorm,c,f,g,h,s,scale,x,y,z,*rv1;

	rv1=dvector(1,n);
	g=scale=anorm=0.0; /* Householder reduction to bidiagonal form */
	for (i=1;i<=n;i++) {
		l=i+1;
		rv1[i]=scale*g;
		g=s=scale=0.0;
		if (i <= m) {
			for (k=i;k<=m;k++) scale += fabs(a[k][i]);
			if (scale) {
				for (k=i;k<=m;k++) {
					a[k][i] /= scale;
					s += a[k][i]*a[k][i];
				}
				f=a[i][i];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][i]=f-g;
				for (j=l;j<=n;j++) {
					for (s=0.0,k=i;k<=m;k++) s += a[k][i]*a[k][j];
					f=s/h;
					for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
				}
				for (k=i;k<=m;k++) a[k][i] *= scale;
			}
		}
		w[i]=scale *g;
		g=s=scale=0.0;
		if (i <= m && i != n) {
			for (k=l;k<=n;k++) scale += fabs(a[i][k]);
			if (scale) {
				for (k=l;k<=n;k++) {
					a[i][k] /= scale;
					s += a[i][k]*a[i][k];
				}
				f=a[i][l];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][l]=f-g;
				for (k=l;k<=n;k++) rv1[k]=a[i][k]/h;
				for (j=l;j<=m;j++) {
					for (s=0.0,k=l;k<=n;k++) s += a[j][k]*a[i][k];
					for (k=l;k<=n;k++) a[j][k] += s*rv1[k];
				}
				for (k=l;k<=n;k++) a[i][k] *= scale;
			}
		}
		anorm = DMAX(anorm,(fabs(w[i])+fabs(rv1[i])));
	}
	for (i=n;i>=1;i--) { /* Accumulation of right-hand transformations. */
		if (i < n) {
			if (g) {
				for (j=l;j<=n;j++) /* Double division to avoid possible underflow. */
					v[j][i]=(a[i][j]/a[i][l])/g;
				for (j=l;j<=n;j++) {
					for (s=0.0,k=l;k<=n;k++) s += a[i][k]*v[k][j];
					for (k=l;k<=n;k++) v[k][j] += s*v[k][i];
				}
			}
			for (j=l;j<=n;j++) v[i][j]=v[j][i]=0.0;
		}
		v[i][i]=1.0;
		g=rv1[i];
		l=i;
	}
	for (i=IMIN(m,n);i>=1;i--) { /* Accumulation of left-hand transformations. */
		l=i+1;
		g=w[i];
		for (j=l;j<=n;j++) a[i][j]=0.0;
		if (g) {
			g=1.0/g;
			for (j=l;j<=n;j++) {
				for (s=0.0,k=l;k<=m;k++) s += a[k][i]*a[k][j];
				f=(s/a[i][i])*g;
				for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
			}
			for (j=i;j<=m;j++) a[j][i] *= g;
		} else for (j=i;j<=m;j++) a[j][i]=0.0;
		++a[i][i];
	}
	for (k=n;k>=1;k--) { /* Diagonalization of the bidiagonal form. */
		for (its=1;its<=30;its++) {
			flag=1;
			for (l=k;l>=1;l--) { /* Test for splitting. */
				nm=l-1; /* Note that rv1[1] is always zero. */
				if ((double)(fabs(rv1[l])+anorm) == anorm) {
					flag=0;
					break;
				}
				if ((double)(fabs(w[nm])+anorm) == anorm) break;
			}
			if (flag) {
				c=0.0; /* Cancellation of rv1[l], if l > 1. */
				s=1.0;
				for (i=l;i<=k;i++) {
					f=s*rv1[i];
					rv1[i]=c*rv1[i];
					if ((double)(fabs(f)+anorm) == anorm) break;
					g=w[i];
					h=pythag(f,g);
					w[i]=h;
					h=1.0/h;
					c=g*h;
					s = -f*h;
					for (j=1;j<=m;j++) {
						y=a[j][nm];
						z=a[j][i];
						a[j][nm]=y*c+z*s;
						a[j][i]=z*c-y*s;
					}
				}
			}
			z=w[k];
			if (l == k) { /* Convergence. */
				if (z < 0.0) { /* Singular value is made nonnegative. */
					w[k] = -z;
					for (j=1;j<=n;j++) v[j][k] = -v[j][k];
				}
				break;
			}
			if (its == 30) printf("no convergence in 30 svdcmp iterations");
			x=w[l]; /* Shift from bottom 2-by-2 minor. */
			nm=k-1;
			y=w[nm];
			g=rv1[nm];
			h=rv1[k];
			f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
			g=pythag(f,1.0);
			f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
			c=s=1.0; /* Next QR transformation: */
			for (j=l;j<=nm;j++) {
				i=j+1;
				g=rv1[i];
				y=w[i];
				h=s*g;
				g=c*g;
				z=pythag(f,h);
				rv1[j]=z;
				c=f/z;
				s=h/z;
				f=x*c+g*s;
				g = g*c-x*s;
				h=y*s;
				y *= c;
				for (jj=1;jj<=n;jj++) {
					x=v[jj][j];
					z=v[jj][i];
					v[jj][j]=x*c+z*s;
					v[jj][i]=z*c-x*s;
				}
				z=pythag(f,h);
				w[j]=z; /* Rotation can be arbitrary if z = 0. */
				if (z) {
					z=1.0/z;
					c=f*z;
					s=h*z;
				}
				f=c*g+s*y;
				x=c*y-s*g;
				for (jj=1;jj<=m;jj++) {
					y=a[jj][j];
					z=a[jj][i];
					a[jj][j]=y*c+z*s;
					a[jj][i]=z*c-y*s;
				}
			}
			rv1[l]=0.0;
			rv1[k]=f;
			w[k]=x;
		}
	}
	free_dvector(rv1,1,n);
}

typedef struct s_thdata {
    char myID;
    char shotAnalyzed;
    char time2wait;
} thread_data;



/* screenCapture for getHomography function */
void *screenShotGetHomography(void* ptrData)
{
    FILE *fp;
    char fileToOpen[50], qrstr[40], *numstr, systemCmd[100];
    int i, j, fileSize, sshotNum = 0, tempNum, projectorID, projectorFN, numQR;
    int qrcorner[NUMBER_OF_QRCODE][10];
    
    bool doneCaptureQR = false;
    
    thread_data *tdata; // pointer to thread_data
    tdata = (thread_data *)ptrData; // get passing pointer  

    printf("screenShotGetHomography QRs start...\n");   

#if BYPASS_CAPTURE_HOMOGRAPHY
        sprintf(systemCmd, "./scan_image sample8qr.png > QRcorners");
        system(systemCmd);

        fp = fopen("QRcorners", "r");
        i = j = 0;
        numQR = 0;
        while(!feof(fp))
        {
            if (fgets(qrstr, 50, fp) != NULL)
            {
                // printf("%s\n", qrstr);
                numstr = strtok(qrstr, ":");
                // printf("%s\n", numstr);
                numstr = strtok(NULL, ",");
                while (numstr != NULL) {
                    tempNum = atoi(numstr);
                    // printf("%s\n", numstr);
                    // printf("j = %d, tempNum= %d\n", j, tempNum);

                    if (j == 0) {
                        projectorID = tempNum / 100;
                        projectorFN = tempNum % 100;
                        qrcorner[i][0] = projectorID;
                        qrcorner[i][1] = projectorFN % 2;
                        j = 2;
                        numQR ++;
                    }
                    else {
                        qrcorner[i][j] = tempNum;
                        j = j + 1;
                    }
                    numstr = strtok(NULL, ",");
                } 
                i = i + 1; j = 0; // next QR
            }
        }
        
        printf("#QRs: %d \n", numQR);
        
        printf("qrcorner= ");
            for (i=0; i<NUMBER_OF_QRCODE; i++)
                for (j=0; j<10; j++)
                    printf("%d ", qrcorner[i][j]);

#else
    // not detect all displayed QRs yet (one per each pico set)
    while (doneCaptureQR != true)
    {
        /* start to capture and analyze one frame */
        numQR = 0;
        for (i=0; i<NUMBER_OF_QRCODE; i++)
            for (j=0; j<10; j++)
                qrcorner[i][j] = 0;

        system("echo 'pause' >/tmp/test.fifo");
        system("echo 'screenshot 0' >/tmp/test.fifo");
        sshotNum++;

        // save screenshot to SDCard
        sprintf(fileToOpen, "shot%04d.png", sshotNum);
        while (1)
        {
            fp = fopen(fileToOpen, "r");
            if (fp)
            {
                fseek(fp, 0, SEEK_END);
                fileSize = ftell(fp);
                if (fileSize > 0) break;
                else fclose(fp);
            }
        }
        fclose(fp);
        // system("./scan_image shot%04d.png > QRcorners", sshotNum);
        // sprintf(systemCmd, "zbarimg shot%04d.png > QRresult", sshotNum);

        // sprintf(systemCmd, "./scan_image sample4qr.png > QRcorners");
        // system(systemCmd);
    
        sprintf(systemCmd, "./scan_image shot%04d.png > QRcorners", sshotNum);
        system(systemCmd);

        /*
        fp = fopen("QRcorners", "r");
        while(!feof(fp))
        {
            if (fgets(qrstr, 20, fp) != NULL)
            {
                numstr = strtok(qrstr, ":");
                numstr = strtok(NULL, ":");
                tempNum = atoi(numstr);
                projectorID = tempNum / 100;
                projectorFN = tempNum % 100;
                // store the bigger frame number for each projector ID
                if (qrcode[projectorID] < projectorFN) 
                    qrcode[projectorID] = projectorFN;
            }
        }
        */
        
        fp = fopen("QRcorners", "r");
        i = j = 0;
        numQR = 0;
        while(!feof(fp))
        {
            if (fgets(qrstr, 50, fp) != NULL)
            {
                // printf("%s\n", qrstr);
                numstr = strtok(qrstr, ":");
                // printf("%s\n", numstr);
                numstr = strtok(NULL, ",");
                while (numstr != NULL) {
                    tempNum = atoi(numstr);
                    // printf("%s\n", numstr);
                    // printf("j = %d, tempNum= %d\n", j, tempNum);

                    if (j == 0) {
                        projectorID = tempNum / 100;
                        projectorFN = tempNum % 100;
                        qrcorner[i][0] = projectorID;
                        qrcorner[i][1] = projectorFN % 2;
                        j = 2;
                        numQR ++;
                    }
                    else {
                        qrcorner[i][j] = tempNum;
                        j = j + 1;
                    }
                    numstr = strtok(NULL, ",");
                } 
                i = i + 1; j = 0; // next QR
            }
        }
        
        printf("#QRs: %d \n", numQR);
        
        /* Need to get both QR codes for each projector, total 8 QR codes */
        if (numQR >= NUMBER_OF_QRCODE) { 
            doneCaptureQR = true;
            
            /* save the last shot to sample8qr.png */
            // sprintf(systemCmd, "cp shot%04d.png sample8qr_td%01d.png", sshotNum, tdata->myID);
            sprintf(systemCmd, "cp shot%04d.png sample8qr.png", sshotNum);
            system(systemCmd);
            
            #ifdef DEBUG_HOMOGRAPHY
                printf("qrcorner= ");
                for (i=0; i<NUMBER_OF_QRCODE; i++)
                    for (j=0; j<10; j++)
                        printf("%d ", qrcorner[i][j]);
            #endif

        }
    }
#endif
    
    /* START CALCULATE HOMOGRAPHY */
    printf("\n***** Got QR codes, start calculate the homography\n");

    double **a1, **a2, **a1inv, **a2inv;
    double **a3, **a4, **a3inv, **a4inv;
    double *w;
    double **u,**v;
    double *h;

    double t;
    double t1[NROW+1],t2[NROW+1];
    int k;
    
    /* local, need to put into thread_data */
    /*
    double h1[NCOL+1];      
    double h1inv[NCOL+1];
    double h2[NCOL+1];
    double h2inv[NCOL+1];
    double h1h2inv[NCOL+1];      
    double a[3][3];    
    double b[3][3];    
    double c[3][3]; 
    double offset1[3][3]; 
     */
    
    double **h1,**h1inv;
    double **h2,**h2inv,**tform2,**otform2,**offset2;
    double **h3,**h3inv,**offset3;
    double **h4,**h4inv,**offset4;

    double *htl, *hbl, *hbr, *htr, *vtl, *vtr, *vbl, *vbr;
    double *offset5;
    double **tform1;

    h1     = dmatrix(1,3,1,3); // start row,row size,start col,col size 
    h1inv  = dmatrix(1,3,1,3);
    h2     = dmatrix(1,3,1,3);
    h2inv  = dmatrix(1,3,1,3);
    tform2  = dmatrix(1,3,1,3);
    otform2 = dmatrix(1,3,1,3);
    offset2 = dmatrix(1,3,1,3);
    h3     = dmatrix(1,3,1,3);
    h3inv  = dmatrix(1,3,1,3);
    offset3 = dmatrix(1,3,1,3);
    h4     = dmatrix(1,3,1,3);
    h4inv  = dmatrix(1,3,1,3);
    offset4 = dmatrix(1,3,1,3);
    
    htl    = dvector(1,3); 
    hbl    = dvector(1,3);
    hbr    = dvector(1,3);
    htr    = dvector(1,3);

    vtl    = dvector(1,3); 
    vtr    = dvector(1,3);
    vbl    = dvector(1,3);
    vbr    = dvector(1,3);
    
    offset5 = dvector(1,3);
    tform1  = dmatrix(1,3,1,3);

    // inner corners LEFT MID & RIGHT MID
    const double x1[] = {0,87,87,253,253,387,387,553,553};
    const double y1[] = {0,157,323,323,157,157,323,323,157};
 
    const double x2[] = {0,87,87,253,253,387,387,553,553};
    const double y2[] = {0,157,323,323,157,157,323,323,157};
 
    const double x3[] = {0,87,87,253,253,387,387,553,553};
    const double y3[] = {0,157,323,323,157,157,323,323,157};
 
    const double x4[] = {0,87,87,253,253,387,387,553,553};
    const double y4[] = {0,157,323,323,157,157,323,323,157};
 
    // detected corners

// #define REFERENCE_CORNERS_TEST    
#ifdef REFERENCE_CORNERS_TEST
    const double X1[] = {0,128,128,211,211,428,428,512,512};
    const double Y1[] = {0,198,282,282,198,198,282,282,198};
    const double X2[] = {0,128,128,211,211,428,428,512,512};
    const double Y2[] = {0,198,282,282,198,198,282,282,198};
#else 
    double X1[] = {0,0,0,0,0,0,0,0,0};
    double Y1[] = {0,0,0,0,0,0,0,0,0};
    double X2[] = {0,0,0,0,0,0,0,0,0};
    double Y2[] = {0,0,0,0,0,0,0,0,0};
    double X3[] = {0,0,0,0,0,0,0,0,0};
    double Y3[] = {0,0,0,0,0,0,0,0,0};
    double X4[] = {0,0,0,0,0,0,0,0,0};
    double Y4[] = {0,0,0,0,0,0,0,0,0};
    
    /* Get detected corners */
    for (i=0; i<NUMBER_OF_QRCODE; i++) {
        if (qrcorner[i][0] == 1) {
            X1[0] = 1; /* detected valid QR code */
            if (qrcorner[i][1] == 0) {
                // printf("\n*** set1 LEFT corner \n");
                for (j=1; j<=4; j++) {
                    X1[j] = qrcorner[i][2*j];
                    // printf("%lf ", X1[j]);
                    Y1[j] = qrcorner[i][2*j+1];
                    // printf("%lf ", Y1[j]);
                }
            }
            else {
                // printf("\n*** set1 RIGHT corner \n");
                for (j=1; j<=4; j++) {
                    X1[j+4] = qrcorner[i][2*j];
                    // printf("%lf ", X1[j+4]);
                    Y1[j+4] = qrcorner[i][2*j+1];
                    // printf("%lf ", Y1[j+4]);
                }
            }
        }
        else if (qrcorner[i][0] == 2) {
            X2[0] = 1; /* detected valid QR code */
            if (qrcorner[i][1] == 0) {
                // printf("\n*** set2 LEFT corner \n");
                for (j=1; j<=4; j++) {
                    X2[j] = qrcorner[i][2*j];
                    // printf("%lf ", X2[j]);
                    Y2[j] = qrcorner[i][2*j+1];
                    // printf("%lf ", Y2[j]);
                }
            }
            else {
                // printf("\n*** set2 RIGHT corner \n");
                for (j=1; j<=4; j++) {
                    X2[j+4] = qrcorner[i][2*j];
                    // printf("%lf ", X2[j+4]);
                    Y2[j+4] = qrcorner[i][2*j+1];
                    // printf("%lf ", Y2[j+4]);
                }
            }
        }
        else if (qrcorner[i][0] == 3) {
            X3[0] = 1; /* detected valid QR code */
            if (qrcorner[i][1] == 0) {
                // printf("\n*** set3 LEFT corner \n");
                for (j=1; j<=4; j++) {
                    X3[j] = qrcorner[i][2*j];
                    // printf("%lf ", X3[j]);
                    Y3[j] = qrcorner[i][2*j+1];
                    // printf("%lf ", Y3[j]);
                }
            }
            else {
                // printf("\n*** set3 RIGHT corner \n");
                for (j=1; j<=4; j++) {
                    X3[j+4] = qrcorner[i][2*j];
                    // printf("%lf ", X3[j+4]);
                    Y3[j+4] = qrcorner[i][2*j+1];
                    // printf("%lf ", Y3[j+4]);
                }
            }
        }
        else if (qrcorner[i][0] == 4) {
            X4[0] = 1; /* detected valid QR code */
            if (qrcorner[i][1] == 0) {
                // printf("\n*** set4 LEFT corner \n");
                for (j=1; j<=4; j++) {
                    X4[j] = qrcorner[i][2*j];
                    // printf("%lf ", X4[j]);
                    Y4[j] = qrcorner[i][2*j+1];
                    // printf("%lf ", Y4[j]);
                }
            }
            else {
                // printf("\n*** set4 RIGHT corner \n");
                for (j=1; j<=4; j++) {
                    X4[j+4] = qrcorner[i][2*j];
                    // printf("%lf ", X4[j+4]);
                    Y4[j+4] = qrcorner[i][2*j+1];
                    // printf("%lf ", Y4[j+4]);
                }
            }
        }
    }
#endif
    
    /* a matrix */
    a1 = dmatrix(1,NROW,1,NCOL); // mxn 
    a2 = dmatrix(1,NROW,1,NCOL); // mxn 
    a3 = dmatrix(1,NROW,1,NCOL); // mxn 
    a4 = dmatrix(1,NROW,1,NCOL); // mxn 
    a1inv = dmatrix(1,NROW,1,NCOL); // mxn 
    a2inv = dmatrix(1,NROW,1,NCOL); // mxn 
    a3inv = dmatrix(1,NROW,1,NCOL); // mxn 
    a4inv = dmatrix(1,NROW,1,NCOL); // mxn 

    u = dmatrix(1,NROW,1,NCOL); // mxn
    w = dvector(1,NCOL);     // 1xn
    v = dmatrix(1,NCOL,1,NCOL); // nxn
    h = dvector(1,NCOL);     // 1xn
    
        // calculate a1 for h1
        for (i=1; i<= NUMBER_OF_POINTS; i++) {
            a1[2*i-1][1] = -x1[i];
            a1[2*i-1][2] = -y1[i];
            a1[2*i-1][3] = -1.0;
            a1[2*i-1][4] = 0.0;
            a1[2*i-1][5] = 0.0;
            a1[2*i-1][6] = 0.0;
            a1[2*i-1][7] = x1[i]*X1[i];
            a1[2*i-1][8] = y1[i]*X1[i];
            a1[2*i-1][9] = X1[i];
            
            a1[2*i][1] = 0;
            a1[2*i][2] = 0;
            a1[2*i][3] = 0;
            a1[2*i][4] = -x1[i];
            a1[2*i][5] = -y1[i];
            a1[2*i][6] = -1.0;
            a1[2*i][7] = x1[i]*Y1[i];
            a1[2*i][8] = y1[i]*Y1[i];
            a1[2*i][9] = Y1[i];
        }
 
#if 1
        // calculate a1inv for h1inv
        for (i=1; i<= NUMBER_OF_POINTS; i++) {
            a1inv[2*i-1][1] = -X1[i];
            a1inv[2*i-1][2] = -Y1[i];
            a1inv[2*i-1][3] = -1.0;
            a1inv[2*i-1][4] = 0.0;
            a1inv[2*i-1][5] = 0.0;
            a1inv[2*i-1][6] = 0.0;
            a1inv[2*i-1][7] = X1[i]*x1[i];
            a1inv[2*i-1][8] = Y1[i]*x1[i];
            a1inv[2*i-1][9] = x1[i];
            
            a1inv[2*i][1] = 0;
            a1inv[2*i][2] = 0;
            a1inv[2*i][3] = 0;
            a1inv[2*i][4] = -X1[i];
            a1inv[2*i][5] = -Y1[i];
            a1inv[2*i][6] = -1.0;
            a1inv[2*i][7] = X1[i]*y1[i];
            a1inv[2*i][8] = Y1[i]*y1[i];
            a1inv[2*i][9] = y1[i];
        }
        
        // calculate a2 for h2
        for (i=1; i<= NUMBER_OF_POINTS; i++) {
            a2[2*i-1][1] = -x2[i];
            a2[2*i-1][2] = -y2[i];
            a2[2*i-1][3] = -1.0;
            a2[2*i-1][4] = 0.0;
            a2[2*i-1][5] = 0.0;
            a2[2*i-1][6] = 0.0;
            a2[2*i-1][7] = x2[i]*X2[i];
            a2[2*i-1][8] = y2[i]*X2[i];
            a2[2*i-1][9] = X2[i];
            
            a2[2*i][1] = 0;
            a2[2*i][2] = 0;
            a2[2*i][3] = 0;
            a2[2*i][4] = -x2[i];
            a2[2*i][5] = -y2[i];
            a2[2*i][6] = -1.0;
            a2[2*i][7] = x2[i]*Y2[i];
            a2[2*i][8] = y2[i]*Y2[i];
            a2[2*i][9] = Y2[i];
        }
#endif
        // calculate a2inv for h2inv
        for (i=1; i<= NUMBER_OF_POINTS; i++) {
            a2inv[2*i-1][1] = -X2[i];
            a2inv[2*i-1][2] = -Y2[i];
            a2inv[2*i-1][3] = -1.0;
            a2inv[2*i-1][4] = 0.0;
            a2inv[2*i-1][5] = 0.0;
            a2inv[2*i-1][6] = 0.0;
            a2inv[2*i-1][7] = X2[i]*x2[i];
            a2inv[2*i-1][8] = Y2[i]*x2[i];
            a2inv[2*i-1][9] = x2[i];
            
            a2inv[2*i][1] = 0;
            a2inv[2*i][2] = 0;
            a2inv[2*i][3] = 0;
            a2inv[2*i][4] = -X2[i];
            a2inv[2*i][5] = -Y2[i];
            a2inv[2*i][6] = -1.0;
            a2inv[2*i][7] = X2[i]*y2[i];
            a2inv[2*i][8] = Y2[i]*y2[i];
            a2inv[2*i][9] = y2[i];
        }

#if 1     
        // calculate a3 for h3
        for (i=1; i<= NUMBER_OF_POINTS; i++) {
            a3[2*i-1][1] = -x3[i];
            a3[2*i-1][2] = -y3[i];
            a3[2*i-1][3] = -1.0;
            a3[2*i-1][4] = 0.0;
            a3[2*i-1][5] = 0.0;
            a3[2*i-1][6] = 0.0;
            a3[2*i-1][7] = x3[i]*X3[i];
            a3[2*i-1][8] = y3[i]*X3[i];
            a3[2*i-1][9] = X3[i];
            
            a3[2*i][1] = 0;
            a3[2*i][2] = 0;
            a3[2*i][3] = 0;
            a3[2*i][4] = -x3[i];
            a3[2*i][5] = -y3[i];
            a3[2*i][6] = -1.0;
            a3[2*i][7] = x3[i]*Y3[i];
            a3[2*i][8] = y3[i]*Y3[i];
            a3[2*i][9] = Y3[i];
        }
#endif    
        // calculate a3inv for h3inv
        for (i=1; i<= NUMBER_OF_POINTS; i++) {
            a3inv[2*i-1][1] = -X3[i];
            a3inv[2*i-1][2] = -Y3[i];
            a3inv[2*i-1][3] = -1.0;
            a3inv[2*i-1][4] = 0.0;
            a3inv[2*i-1][5] = 0.0;
            a3inv[2*i-1][6] = 0.0;
            a3inv[2*i-1][7] = X3[i]*x3[i];
            a3inv[2*i-1][8] = Y3[i]*x3[i];
            a3inv[2*i-1][9] = x3[i];
            
            a3inv[2*i][1] = 0;
            a3inv[2*i][2] = 0;
            a3inv[2*i][3] = 0;
            a3inv[2*i][4] = -X3[i];
            a3inv[2*i][5] = -Y3[i];
            a3inv[2*i][6] = -1.0;
            a3inv[2*i][7] = X3[i]*y3[i];
            a3inv[2*i][8] = Y3[i]*y3[i];
            a3inv[2*i][9] = y3[i];
        }
    
#if 1
        // calculate a4 for h4
        for (i=1; i<= NUMBER_OF_POINTS; i++) {
            a4[2*i-1][1] = -x4[i];
            a4[2*i-1][2] = -y4[i];
            a4[2*i-1][3] = -1.0;
            a4[2*i-1][4] = 0.0;
            a4[2*i-1][5] = 0.0;
            a4[2*i-1][6] = 0.0;
            a4[2*i-1][7] = x4[i]*X4[i];
            a4[2*i-1][8] = y4[i]*X4[i];
            a4[2*i-1][9] = X4[i];
            
            a4[2*i][1] = 0;
            a4[2*i][2] = 0;
            a4[2*i][3] = 0;
            a4[2*i][4] = -x4[i];
            a4[2*i][5] = -y4[i];
            a4[2*i][6] = -1.0;
            a4[2*i][7] = x4[i]*Y4[i];
            a4[2*i][8] = y4[i]*Y4[i];
            a4[2*i][9] = Y4[i];
        }
#endif
    
        // calculate a4inv for h4inv
        for (i=1; i<= NUMBER_OF_POINTS; i++) {
            a4inv[2*i-1][1] = -X4[i];
            a4inv[2*i-1][2] = -Y4[i];
            a4inv[2*i-1][3] = -1.0;
            a4inv[2*i-1][4] = 0.0;
            a4inv[2*i-1][5] = 0.0;
            a4inv[2*i-1][6] = 0.0;
            a4inv[2*i-1][7] = X4[i]*x4[i];
            a4inv[2*i-1][8] = Y4[i]*x4[i];
            a4inv[2*i-1][9] = x4[i];
            
            a4inv[2*i][1] = 0;
            a4inv[2*i][2] = 0;
            a4inv[2*i][3] = 0;
            a4inv[2*i][4] = -X4[i];
            a4inv[2*i][5] = -Y4[i];
            a4inv[2*i][6] = -1.0;
            a4inv[2*i][7] = X4[i]*y4[i];
            a4inv[2*i][8] = Y4[i]*y4[i];
            a4inv[2*i][9] = y4[i];
        }
    
    ////////////////////////////////////////////////////////////////   
    /* SET 1 h1 and h1inv */
    // printf("\nA1 = \n");
    for (i=1; i<=NROW; i++) {
        for (j=1; j<=NCOL; j++) {
            u[i][j] = a1[i][j];
            // printf("%5.0lf ", u[i][j]);    
        }
            // printf("\n");
    }
	svdcmp(u,NROW,NCOL,w,v);
    /* Sort the singular values in descending order */
	for (i=1; i<NCOL; i++) {
        for (j=i+1; j<=NCOL; j++) {
            if (w[i]<w[j]) {
                t = w[i];
                w[i] = w[j];
                w[j] = t;
                for (k=1; k<=NROW; k++) t1[k] = u[k][i];
                for (k=1; k<=NCOL; k++) t2[k] = v[k][i];
                for (k=1; k<=NROW; k++) u[k][i] = u[k][j];
                for (k=1; k<=NCOL; k++) v[k][i] = v[k][j];
                for (k=1; k<=NROW; k++) u[k][j] = t1[k];
                for (k=1; k<=NCOL; k++) v[k][j] = t2[k];
            }
        }
	}
        
    for (i=1; i<=NCOL; i++) {
        h[i] = v[i][9];
        // printf("        h[%d]    = %lf\n", i, h[i]); 
    }
    for (i=1; i<=NCOL; i++) {
        h[i] = h[i]/h[9];
        // printf("        H[%d]    = %lf\n", i, h[i]);
    }

    printf("h1 = ");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            h1[i][j] = h[j+3*i-3];
            printf("%lf ", h1[i][j]);
        }
	}
    printf("\n");
        
    // printf("\n A1inv = \n");
    for (i=1; i<=NROW; i++) {
        for (j=1; j<=NCOL; j++) {
            u[i][j] = a1inv[i][j];
            // printf("%5.0lf ", u[i][j]);    
        }
        // printf("\n");
        // printf("%5.0lf %5.0lf %5.0lf %5.0lf %5.0lf %5.0lf %5.0lf %5.0lf %5.0lf\n",u[i][1],u[i][2],u[i][3],u[i][4],u[i][5],u[i][6],u[i][7],u[i][8],u[i][9]);
    }
	svdcmp(u,NROW,NCOL,w,v);
        
	/* Sort the singular values in descending order */
	for (i=1; i<NCOL; i++) {
        for (j=i+1; j<=NCOL; j++) {
            if (w[i]<w[j]) {
                t = w[i];
                w[i] = w[j];
                w[j] = t;
                for (k=1; k<=NROW; k++) t1[k] = u[k][i];
                for (k=1; k<=NCOL; k++) t2[k] = v[k][i];
                for (k=1; k<=NROW; k++) u[k][i] = u[k][j];
                for (k=1; k<=NCOL; k++) v[k][i] = v[k][j];
                for (k=1; k<=NROW; k++) u[k][j] = t1[k];
                for (k=1; k<=NCOL; k++) v[k][j] = t2[k];
            }
        }
	}
        
    for (i=1; i<=NCOL; i++) {
        h[i] = v[i][9];
        // printf("        h[%d]    = %lf\n", i, h[i]);
	}
    for (i=1; i<=NCOL; i++) {
        h[i] = h[i]/h[9];
        // printf("        H[%d]    = %lf\n", i, h[i]);
	}
    printf("h1inv = ");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            h1inv[i][j] = h[j+3*i-3];
            printf("%lf ", h1inv[i][j]);
        }
	}
    printf("\n");
        
////////////////////////////////////////////////////////
    // printf("\n A2 = \n");
    for (i=1; i<=NROW; i++) {
        for (j=1; j<=NCOL; j++) {
            u[i][j] = a2[i][j];
            // printf("%5.0lf ", u[i][j]);    
        }
        // printf("\n");
	}
	svdcmp(u,NROW,NCOL,w,v);
	/* Sort the singular values in descending order */
	for (i=1; i<NCOL; i++) {
        for (j=i+1; j<=NCOL; j++) {
            if (w[i]<w[j]) {
                t = w[i];
                w[i] = w[j];
                w[j] = t;
                for (k=1; k<=NROW; k++) t1[k] = u[k][i];
                for (k=1; k<=NCOL; k++) t2[k] = v[k][i];
                for (k=1; k<=NROW; k++) u[k][i] = u[k][j];
                for (k=1; k<=NCOL; k++) v[k][i] = v[k][j];
                for (k=1; k<=NROW; k++) u[k][j] = t1[k];
                for (k=1; k<=NCOL; k++) v[k][j] = t2[k];
            }
        }
	}
    for (i=1; i<=NCOL; i++) {
        h[i] = v[i][9];
        // printf("        h[%d]    = %lf\n", i, h[i]);
    }
    for (i=1; i<=NCOL; i++) {
        h[i] = h[i]/h[9];
        // printf("        H[%d]    = %lf\n", i, h[i]);
    }

    printf("h2 = ");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            h2[i][j] = h[j+3*i-3];
            printf("%lf ", h2[i][j]);
        }
    }
    printf("\n");
        
    /* A2inv */
    // printf("\n A2inv = \n");
    for (i=1; i<=NROW; i++) {
        for (j=1; j<=NCOL; j++) {
            u[i][j] = a2inv[i][j];
            // printf("%5.0lf ", u[i][j]);    
        }
        // printf("\n");
	}
	svdcmp(u,NROW,NCOL,w,v);
	/* Sort the singular values in descending order */
	for (i=1; i<NCOL; i++) {
        for (j=i+1; j<=NCOL; j++) {
		    if (w[i]<w[j]) {
                t = w[i];
                w[i] = w[j];
                w[j] = t;
                for (k=1; k<=NROW; k++) t1[k] = u[k][i];
                for (k=1; k<=NCOL; k++) t2[k] = v[k][i];
                for (k=1; k<=NROW; k++) u[k][i] = u[k][j];
                for (k=1; k<=NCOL; k++) v[k][i] = v[k][j];
                for (k=1; k<=NROW; k++) u[k][j] = t1[k];
                for (k=1; k<=NCOL; k++) v[k][j] = t2[k];
            }
       }
	}
    for (i=1; i<=NCOL; i++) {
        h[i] = v[i][9];
        // printf("        h[%d]    = %lf\n", i, h[i]);
	}
    for (i=1; i<=NCOL; i++) {
        h[i] = h[i]/h[9];
        // printf("        H[%d]    = %lf\n", i, h[i]);
	}
    printf("h2inv = ");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            h2inv[i][j] = h[j+3*i-3];
            printf("%lf ", h2inv[i][j]);
        }
	}
    printf("\n");

    /* SET 3 */
    // printf("\nA3 = \n");
    for (i=1; i<=NROW; i++) {
        for (j=1; j<=NCOL; j++) {
            u[i][j] = a3[i][j];
            // printf("%5.0lf ", u[i][j]);    
        }
        // printf("\n");
    }
	svdcmp(u,NROW,NCOL,w,v);
    /* Sort the singular values in descending order */
	for (i=1; i<NCOL; i++) {
        for (j=i+1; j<=NCOL; j++) {
        	if (w[i]<w[j]) {
                t = w[i];
                w[i] = w[j];
                w[j] = t;
                for (k=1; k<=NROW; k++) t1[k] = u[k][i];
                for (k=1; k<=NCOL; k++) t2[k] = v[k][i];
                for (k=1; k<=NROW; k++) u[k][i] = u[k][j];
                for (k=1; k<=NCOL; k++) v[k][i] = v[k][j];
                for (k=1; k<=NROW; k++) u[k][j] = t1[k];
                for (k=1; k<=NCOL; k++) v[k][j] = t2[k];
            }
        }
	}
        
    for (i=1; i<=NCOL; i++) {
        h[i] = v[i][9];
	}
    for (i=1; i<=NCOL; i++) {
        h[i] = h[i]/h[9];
	}

    printf("h3 = ");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            h3[i][j] = h[j+3*i-3];
            printf("%lf ", h3[i][j]);
        }
	}
    printf("\n");    

    /* A3inv */
    // printf("\n A3inv = \n");
    for (i=1; i<=NROW; i++) {
        for (j=1; j<=NCOL; j++) {
            u[i][j] = a3inv[i][j];
            // printf("%5.0lf ", u[i][j]);    
        }
        // printf("\n");
	}
	svdcmp(u,NROW,NCOL,w,v);
	/* Sort the singular values in descending order */
	for (i=1; i<NCOL; i++) {
        for (j=i+1; j<=NCOL; j++) {
            if (w[i]<w[j]) {
                t = w[i];
                w[i] = w[j];
                w[j] = t;
                for (k=1; k<=NROW; k++) t1[k] = u[k][i];
                for (k=1; k<=NCOL; k++) t2[k] = v[k][i];
                for (k=1; k<=NROW; k++) u[k][i] = u[k][j];
                for (k=1; k<=NCOL; k++) v[k][i] = v[k][j];
                for (k=1; k<=NROW; k++) u[k][j] = t1[k];
                for (k=1; k<=NCOL; k++) v[k][j] = t2[k];
            }
        }
	}
    for (i=1; i<=NCOL; i++) {
        h[i] = v[i][9];
        // printf("        h[%d]    = %lf\n", i, h[i]);
	}
    for (i=1; i<=NCOL; i++) {
        h[i] = h[i]/h[9];
        // printf("        H[%d]    = %lf\n", i, h[i]);
	}
    printf("h3inv = ");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            h3inv[i][j] = h[j+3*i-3];
            printf("%lf ", h3inv[i][j]);
        }
	}
    printf("\n");
    
    /* SET 4 */
    // printf("\nA4 = \n");
    for (i=1; i<=NROW; i++) {
        for (j=1; j<=NCOL; j++) {
            u[i][j] = a4[i][j];
            // printf("%5.0lf ", u[i][j]);    
        }
        // printf("\n");
    }
	svdcmp(u,NROW,NCOL,w,v);
    /* Sort the singular values in descending order */
	for (i=1; i<NCOL; i++) {
        for (j=i+1; j<=NCOL; j++) {
        	if (w[i]<w[j]) {
                t = w[i];
                w[i] = w[j];
                w[j] = t;
                for (k=1; k<=NROW; k++) t1[k] = u[k][i];
                for (k=1; k<=NCOL; k++) t2[k] = v[k][i];
                for (k=1; k<=NROW; k++) u[k][i] = u[k][j];
                for (k=1; k<=NCOL; k++) v[k][i] = v[k][j];
                for (k=1; k<=NROW; k++) u[k][j] = t1[k];
                for (k=1; k<=NCOL; k++) v[k][j] = t2[k];
            }
        }
	}
        
    for (i=1; i<=NCOL; i++) {
        h[i] = v[i][9];
	}
    for (i=1; i<=NCOL; i++) {
        h[i] = h[i]/h[9];
	}

    printf("h4 = ");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            h4[i][j] = h[j+3*i-3];
            printf("%lf ", h4[i][j]);
        }
	}
    printf("\n");        

    /* A4inv */
    // printf("\n A4inv = \n");
    for (i=1; i<=NROW; i++) {
        for (j=1; j<=NCOL; j++) {
            u[i][j] = a4inv[i][j];
            // printf("%5.0lf ", u[i][j]);    
        }
        // printf("\n");
	}
	svdcmp(u,NROW,NCOL,w,v);
	/* Sort the singular values in descending order */
	for (i=1; i<NCOL; i++) {
        for (j=i+1; j<=NCOL; j++) {
        	if (w[i]<w[j]) {
                t = w[i];
                w[i] = w[j];
                w[j] = t;
                for (k=1; k<=NROW; k++) t1[k] = u[k][i];
                for (k=1; k<=NCOL; k++) t2[k] = v[k][i];
                for (k=1; k<=NROW; k++) u[k][i] = u[k][j];
                for (k=1; k<=NCOL; k++) v[k][i] = v[k][j];
                for (k=1; k<=NROW; k++) u[k][j] = t1[k];
                for (k=1; k<=NCOL; k++) v[k][j] = t2[k];
            }
        }
	}
    for (i=1; i<=NCOL; i++) {
        h[i] = v[i][9];
        // printf("        h[%d]    = %lf\n", i, h[i]);
	}
    for (i=1; i<=NCOL; i++) {
        h[i] = h[i]/h[9];
        // printf("        H[%d]    = %lf\n", i, h[i]);
	}
    printf("h4inv = ");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            h4inv[i][j] = h[j+3*i-3];
            printf("%lf ", h4inv[i][j]);
        }
	}
    printf("\n");

    /******************************************************************************/
    /* Write to text files                                                        */    
    /******************************************************************************/
FILE *matp;
/* Write to h2inv.txt, NOT USED comment out */
#if 0
    matp = fopen("h2inv.txt", "w");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            fprintf(matp, "%f ", h2inv[i][j]);
        }
    }
    fclose(matp);
#endif 
    
/* OFFSET */
    /* inversed offset */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            offset2[i][j] = 0;
            offset3[i][j] = 0;
            offset4[i][j] = 0;
        }
    }
    offset2[1][1] = offset2[2][2] = offset2[3][3] = 1;
    offset3[1][1] = offset3[2][2] = offset3[3][3] = 1;
    offset4[1][1] = offset4[2][2] = offset4[3][3] = 1;
    // offset2[1][3] = -560; // TEST -640 current setting = 560

#ifdef OFFSET_16_9
    offset2[2][3] = 0; // test offset
    offset2[1][3] = 640; 
    offset3[2][3] = 360; // 480 test offset 
    offset4[1][3] = 640; 
    offset4[2][3] = 360; // 480 test offset 
#else
    offset2[1][3] = HOVL; // reduced by 20 to fit with overlapped image 640; 
    offset3[2][3] = VOVL; // reduced by 20 to fit with overlapped image 480; 
    offset4[1][3] = HOVL; 
    offset4[2][3] = VOVL; 
#endif
    
#if 0
    /* H1invH2*/    
    /* tform2 = h1invh2 */     
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            tform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                tform2[i][j] = tform2[i][j] + h1inv[i][k]*h2[k][j];
            }
        }
    }
#endif
    
/* Write to h1invh2.txt NOT USED, comment out */
#if 0    
    matp = fopen("h1invh2.txt", "w");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            fprintf(matp, "%5.5g ", tform2[i][j]);
        }
    }
    fclose(matp);
#endif
    
#if 0
    /* Write to o2h1invh2.txt */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            otform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                otform2[i][j] = otform2[i][j] + offset2[i][k]*tform2[k][j];
            }
        }
    }
    matp = fopen("o2h1invh2.txt", "w");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            fprintf(matp, "%5.5g ", otform2[i][j]);
        }
    }
    fclose(matp);
#endif

#if 0    
/* H1invH3*/    
    /* tform2 = h1invh3 */     
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            tform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                tform2[i][j] = tform2[i][j] + h1inv[i][k]*h3[k][j];
            }
        }
    }
    /* Write to o3h1invh3.txt */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            otform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                otform2[i][j] = otform2[i][j] + offset3[i][k]*tform2[k][j];
            }
        }
    }
    matp = fopen("o3h1invh3.txt", "w");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            fprintf(matp, "%5.5g ", otform2[i][j]);
        }
    }
    fclose(matp);
#endif

#if 0    
/* H1invH4*/    
    /* tform2 = h1invh4 */     
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            tform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                tform2[i][j] = tform2[i][j] + h1inv[i][k]*h4[k][j];
            }
        }
    }
    /* Write to o4h1invh4.txt */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            otform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                otform2[i][j] = otform2[i][j] + offset4[i][k]*tform2[k][j];
            }
        }
    }
    matp = fopen("o4h1invh4.txt", "w");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            fprintf(matp, "%5.5g ", otform2[i][j]);
        }
    }
    fclose(matp);
#endif
    
/* o2h2invh1.txt NOT USED, comment out */
#if 0
    /* Write to tform2, h2invh1.txt */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            tform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                tform2[i][j] = tform2[i][j] + h2inv[i][k]*h1[k][j];
            }
        }
    }
    matp = fopen("h2invh1.txt", "w");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            fprintf(matp, "%5.5g ", tform2[i][j]);
        }
    }
    fclose(matp);
    
    /* inversed offset2 */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            offset2[i][j] = 0;
        }
    }
    offset2[1][1] = offset2[2][2] = offset2[3][3] = 1;
    offset2[1][3] = 580; // TEST 640,  current setting = 560
    
    /* Write to o2h2invh1.txt */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            otform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                otform2[i][j] = otform2[i][j] + offset2[i][k]*tform2[k][j];
            }
        }
    }
    matp = fopen("o2h2invh1.txt", "w");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            fprintf(matp, "%5.5g ", otform2[i][j]);
        }
    }
    fclose(matp);
#endif    

/* HOMOGRAPHY CALCULATION */  
/* myblend1.txt : unity */    
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            otform2[i][j] = 0;
        }
    }
    otform2[1][1] = otform2[2][2] = otform2[3][3] = 1;
#if OFFSET_16_9
    otform2[2][3] = 0; // test offset 120; 
#endif
    matp = fopen("myblend1.txt", "w");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            fprintf(matp, "%5.5g ", otform2[i][j]);
        }
    }
    fclose(matp);
    
/* myblend2.txt combined h2invh1o2.txt */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            tform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                tform2[i][j] = tform2[i][j] + h2inv[i][k]*h1[k][j];
            }
        }
    }
    
    /* Write to h2invh1o2.txt */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            otform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                otform2[i][j] = otform2[i][j] + tform2[i][k]*offset2[k][j];
            }
        }
    }
    // matp = fopen("h2invh1o2.txt", "w");
    matp = fopen("myblend2.txt", "w");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            fprintf(matp, "%5.5g ", otform2[i][j]);
        }
    }
    fclose(matp);
    
/* myblend3.txt combined h3invh1o3.txt */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            tform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                tform2[i][j] = tform2[i][j] + h3inv[i][k]*h1[k][j];
            }
        }
    }
    
    /* Write to h3invh1o3.txt */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            otform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                otform2[i][j] = otform2[i][j] + tform2[i][k]*offset3[k][j];
            }
        }
    }
    // matp = fopen("h3invh1o3.txt", "w");
    matp = fopen("myblend3.txt", "w");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            fprintf(matp, "%5.5g ", otform2[i][j]);
        }
    }
    fclose(matp);
    
/* myblend4.txt combined h4invh1o4.txt */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            tform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                tform2[i][j] = tform2[i][j] + h4inv[i][k]*h1[k][j];
            }
        }
    }
    
    /* Write to h4invh1o4.txt */
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            otform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                otform2[i][j] = otform2[i][j] + tform2[i][k]*offset4[k][j];
            }
        }
    }
    // matp = fopen("h4invh1o4.txt", "w");
    matp = fopen("myblend4.txt", "w");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            fprintf(matp, "%5.5g ", otform2[i][j]);
        }
    }
    fclose(matp);
    
/* BLENDING PARAMETERS CALCULATION */

    /* offset values for blending */
    for (i=1; i<=3; i++) {
        offset5[i] = 1;
    }
    
    /* set 1, (htform) tform1 = h1inv*h2, (vtform) tform2 = h1inv*h3 */
    printf("\nset1 tform1 = h1inv*h2, tform2 = h1inv*h3: \n");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            tform1[i][j] = 0;
            tform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                tform1[i][j] = tform1[i][j] + h1inv[i][k]*h2[k][j];
                tform2[i][j] = tform2[i][j] + h1inv[i][k]*h3[k][j];
            }            
            printf("%lf/%lf ", tform1[i][j],tform2[i][j]);
        }
    }
    
    /* htl */
    printf("\nset1 htl = ");
    offset5[2] = 1; offset5[1] = 1;
    for (i=1; i<3; i++) {
        htl[i] = 0; 
        for (k=1; k<=3; k++) {
            htl[i] = htl[i] + tform1[i][k]*offset5[k];
        }
        printf("%lf ", htl[i]);
    }
    /* vtl */
    printf("\nset1 vtl = ");
    for (i=1; i<3; i++) {
        vtl[i] = 0; 
        for (k=1; k<=3; k++) {
            vtl[i] = vtl[i] + tform2[i][k]*offset5[k];
        }
        printf("%lf ", vtl[i]);
    }
    /* hbl */
    printf("\nset1 hbl = ");
    offset5[2] = 480; /* keep 640x480 */
    offset5[1] = 1;
    for (i=1; i<3; i++) {
        hbl[i] = 0; 
        for (k=1; k<=3; k++) {
            hbl[i] = hbl[i] + tform1[i][k]*offset5[k];
        }
        printf("%lf ", hbl[i]);
    }
    /* vbl */
    printf("\nset1 vbl = ");
    for (i=1; i<3; i++) {
        vbl[i] = 0; 
        for (k=1; k<=3; k++) {
            vbl[i] = vbl[i] + tform2[i][k]*offset5[k];
        }
        printf("%lf ", vbl[i]);
    }
    /* htr */
    printf("\nset1 htr = ");
    offset5[2] = 1; 
    offset5[1] = 640; /* keep 640x480 */
    for (i=1; i<3; i++) {
        htr[i] = 0; 
        for (k=1; k<=3; k++) {
            htr[i] = htr[i] + tform1[i][k]*offset5[k];
        }
        printf("%lf ", htr[i]);
    }
    /* vtr */
    printf("\nset1 vtr = ");
    for (i=1; i<3; i++) {
        vtr[i] = 0; 
        for (k=1; k<=3; k++) {
            vtr[i] = vtr[i] + tform2[i][k]*offset5[k];
        }
        printf("%lf ", vtr[i]);
    }
    /* hbr */
    printf("\nset1 hbr = ");
    offset5[2] = 480; /* keep 640x480 */
    offset5[1] = 640; /* keep 640x480 */
    for (i=1; i<3; i++) {
        hbr[i] = 0; 
        for (k=1; k<=3; k++) {
            hbr[i] = hbr[i] + tform1[i][k]*offset5[k];
        }
        printf("%lf ", hbr[i]);
    }
    /* vbr */
    printf("\nset1 vbr = ");
    for (i=1; i<3; i++) {
        vbr[i] = 0; 
        for (k=1; k<=3; k++) {
            vbr[i] = vbr[i] + tform2[i][k]*offset5[k];
        }
        printf("%lf ", vbr[i]);
    }
    
    matp = fopen("myblend1.txt", "a");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", htl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", hbl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", htr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", hbr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vtl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vbl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vtr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vbr[i]);
    }
    /* end Blending Set 1 */

    
    /* set 2, (htform) tform1 = h2inv*h1, (vtform) tform2 = h2inv*h4 */
    // printf("\nset2 tform1 = h2inv*h1, tform2 = h2inv*h4: \n");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            tform1[i][j] = 0;
            tform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                tform1[i][j] = tform1[i][j] + h2inv[i][k]*h1[k][j];
                tform2[i][j] = tform2[i][j] + h2inv[i][k]*h4[k][j];
            }
        }
    }
    /* htl */
    offset5[2] = 1; offset5[1] = 1;
    for (i=1; i<3; i++) {
        htl[i] = 0; 
        for (k=1; k<=3; k++) {
            htl[i] = htl[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vtl */
    for (i=1; i<3; i++) {
        vtl[i] = 0; 
        for (k=1; k<=3; k++) {
            vtl[i] = vtl[i] + tform2[i][k]*offset5[k];
        }
    }
    /* hbl */
    offset5[2] = 480; /* keep 640x480 */
    offset5[1] = 1;
    for (i=1; i<3; i++) {
        hbl[i] = 0; 
        for (k=1; k<=3; k++) {
            hbl[i] = hbl[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vbl */
    for (i=1; i<3; i++) {
        vbl[i] = 0; 
        for (k=1; k<=3; k++) {
            vbl[i] = vbl[i] + tform2[i][k]*offset5[k];
        }
    }
    /* htr */
    offset5[2] = 1; 
    offset5[1] = 640; /* keep 640x480 */
    for (i=1; i<3; i++) {
        htr[i] = 0; 
        for (k=1; k<=3; k++) {
            htr[i] = htr[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vtr */
    for (i=1; i<3; i++) {
        vtr[i] = 0; 
        for (k=1; k<=3; k++) {
            vtr[i] = vtr[i] + tform2[i][k]*offset5[k];
        }
    }
    /* hbr */
    offset5[2] = 480; /* keep 640x480 */
    offset5[1] = 640; /* keep 640x480 */
    for (i=1; i<3; i++) {
        hbr[i] = 0; 
        for (k=1; k<=3; k++) {
            hbr[i] = hbr[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vbr */
    for (i=1; i<3; i++) {
        vbr[i] = 0; 
        for (k=1; k<=3; k++) {
            vbr[i] = vbr[i] + tform2[i][k]*offset5[k];
        }
    }
    
    matp = fopen("myblend2.txt", "a");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", htl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", hbl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", htr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", hbr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vtl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vbl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vtr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vbr[i]);
    }
    /* end Blending Set 2 */

    /* set 3, (htform) tform1 = h3inv*h4, (vtform) tform2 = h3inv*h1 */
    // printf("\nset3 tform1 = h3inv*h4, tform2 = h3inv*h1: \n");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            tform1[i][j] = 0;
            tform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                tform1[i][j] = tform1[i][j] + h3inv[i][k]*h4[k][j];
                tform2[i][j] = tform2[i][j] + h3inv[i][k]*h1[k][j];
            }
        }
    }
    /* htl */
    offset5[2] = 1; offset5[1] = 1;
    for (i=1; i<3; i++) {
        htl[i] = 0; 
        for (k=1; k<=3; k++) {
            htl[i] = htl[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vtl */
    for (i=1; i<3; i++) {
        vtl[i] = 0; 
        for (k=1; k<=3; k++) {
            vtl[i] = vtl[i] + tform2[i][k]*offset5[k];
        }
    }
    /* hbl */
    offset5[2] = 480; /* keep 640x480 */
    offset5[1] = 1;
    for (i=1; i<3; i++) {
        hbl[i] = 0; 
        for (k=1; k<=3; k++) {
            hbl[i] = hbl[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vbl */
    for (i=1; i<3; i++) {
        vbl[i] = 0; 
        for (k=1; k<=3; k++) {
            vbl[i] = vbl[i] + tform2[i][k]*offset5[k];
        }
    }
    /* htr */
    offset5[2] = 1; 
    offset5[1] = 640; /* keep 640x480 */
    for (i=1; i<3; i++) {
        htr[i] = 0; 
        for (k=1; k<=3; k++) {
            htr[i] = htr[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vtr */
    for (i=1; i<3; i++) {
        vtr[i] = 0; 
        for (k=1; k<=3; k++) {
            vtr[i] = vtr[i] + tform2[i][k]*offset5[k];
        }
    }
    /* hbr */
    offset5[2] = 480; /* keep 640x480 */
    offset5[1] = 640; /* keep 640x480 */
    for (i=1; i<3; i++) {
        hbr[i] = 0; 
        for (k=1; k<=3; k++) {
            hbr[i] = hbr[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vbr */
    for (i=1; i<3; i++) {
        vbr[i] = 0; 
        for (k=1; k<=3; k++) {
            vbr[i] = vbr[i] + tform2[i][k]*offset5[k];
        }
    }
    
    matp = fopen("myblend3.txt", "a");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", htl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", hbl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", htr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", hbr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vtl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vbl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vtr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vbr[i]);
    }
    fclose(matp);
    /* end Blending Set 3 */
   
    /* set 4, (htform) tform1 = h4inv*h3, (vtform) tform2 = h4inv*h2 */
    // printf("\nset4 tform4 = h4inv*h3, tform2 = h4inv*h2: \n");
    for (i=1; i<=3; i++) {
        for (j=1; j<=3; j++) {
            tform1[i][j] = 0;
            tform2[i][j] = 0;
            for (k=1;k<=3;k++) {
                tform1[i][j] = tform1[i][j] + h4inv[i][k]*h3[k][j];
                tform2[i][j] = tform2[i][j] + h4inv[i][k]*h2[k][j];
            }
        }
    }
    /* htl */
    offset5[2] = 1; offset5[1] = 1;
    for (i=1; i<3; i++) {
        htl[i] = 0; 
        for (k=1; k<=3; k++) {
            htl[i] = htl[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vtl */
    for (i=1; i<3; i++) {
        vtl[i] = 0; 
        for (k=1; k<=3; k++) {
            vtl[i] = vtl[i] + tform2[i][k]*offset5[k];
        }
    }
    /* hbl */
    offset5[2] = 480; /* keep 640x480 */
    offset5[1] = 1;
    for (i=1; i<3; i++) {
        hbl[i] = 0; 
        for (k=1; k<=3; k++) {
            hbl[i] = hbl[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vbl */
    for (i=1; i<3; i++) {
        vbl[i] = 0; 
        for (k=1; k<=3; k++) {
            vbl[i] = vbl[i] + tform2[i][k]*offset5[k];
        }
    }
    /* htr */
    offset5[2] = 1; 
    offset5[1] = 640; /* keep 640x480 */
    for (i=1; i<3; i++) {
        htr[i] = 0; 
        for (k=1; k<=3; k++) {
            htr[i] = htr[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vtr */
    for (i=1; i<3; i++) {
        vtr[i] = 0; 
        for (k=1; k<=3; k++) {
            vtr[i] = vtr[i] + tform2[i][k]*offset5[k];
        }
    }
    /* hbr */
    offset5[2] = 480; /* keep 640x480 */
    offset5[1] = 640; /* keep 640x480 */
    for (i=1; i<3; i++) {
        hbr[i] = 0; 
        for (k=1; k<=3; k++) {
            hbr[i] = hbr[i] + tform1[i][k]*offset5[k];
        }
    }
    /* vbr */
    for (i=1; i<3; i++) {
        vbr[i] = 0; 
        for (k=1; k<=3; k++) {
            vbr[i] = vbr[i] + tform2[i][k]*offset5[k];
        }
    }
    
    matp = fopen("myblend4.txt", "a");
    if (matp == NULL) {
        exit -1;
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", htl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", hbl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", htr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", hbr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vtl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vbl[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vtr[i]);
    }
    for (i=1; i<3; i++) {
        fprintf(matp, "%5.5g ", vbr[i]);
    }
    fclose(matp);
    /* end Blending Set 4 */   
    
    /* END OF HOMOGRAPHY CALCULATION HERE */
#if BYPASS_CAPTURE_HOMOGRAPHY
    tdata->time2wait = 0; 
#else
    tdata->time2wait = WAIT_AFTER_DONE_GET_HOMOGRAPHY; // WAIT FOR ANOTHER CATCH UP
#endif
    tdata->shotAnalyzed = 1;
    return tdata;
}

/* screenShot for syncVideo function */
void *screenShotSyncVideo(void* ptrData)
{
    FILE *fp;
    char fileToOpen[50], qrstr[20], *numstr, systemCmd[100];
    int i, numProjector = 0, fileSize, tempWait, sshotNum = 0, tempNum, projectorID, projectorFN;
    int qrcode[MAX_PICO_SET];
    int min_counter = 999;

    thread_data *tdata; // pointer to thread_data
    tdata = (thread_data *)ptrData; // get passing pointer  

    printf(">>>>> screenShotSyncVideo start\n");   

    // not detect all displayed QRs yet (one per each pico set)
    while (numProjector != MAX_PICO_SET)
    {
        numProjector = 0;
        for (i=0; i<MAX_PICO_SET; i++) qrcode[i] = 0;

        system("echo 'pause' >/tmp/test.fifo");
        system("echo 'screenshot 0' >/tmp/test.fifo");
        sshotNum++;

        // save screenshot to SDCard
        sprintf(fileToOpen, "shot%04d.png", sshotNum);
        while (1)
        {
            fp = fopen(fileToOpen, "r");
            if (fp)
            {
                fseek(fp, 0, SEEK_END);
                fileSize = ftell(fp);
                if (fileSize > 0) break;
                else fclose(fp);
            }
        }
        fclose(fp);

        // sprintf(systemCmd, "/home/root/zbar-0.10/zbarimg/zbarimg shot%04d.png > QRresult", sshotNum);
        sprintf(systemCmd, "zbarimg shot%04d.png > QRresult", sshotNum);
        system(systemCmd);
    
        fp = fopen("QRresult", "r");
        while(!feof(fp))
        {
            if (fgets(qrstr, 20, fp) != NULL)
            {
                numstr = strtok(qrstr, ":");
                numstr = strtok(NULL, ":");
                tempNum = atoi(numstr);
                projectorID = tempNum / 100;
                projectorFN = tempNum % 100;
                // store the bigger frame number for each projector ID
                printf("detected pID=%d frame=%d\n", projectorID, projectorFN);
                if (qrcode[projectorID-1] < projectorFN) 
                    qrcode[projectorID-1] = projectorFN;
                
                 // reject homography QR
                if (qrcode[projectorID-1] == 98 || qrcode[projectorID-1] == 99)
                    qrcode[projectorID-1] = 0;
            }
        }

        // count number of projector based on at least one good QR detected for each pico set
        for (i=0; i<MAX_PICO_SET; i++)
            if (qrcode[i] > 0) numProjector++;
        
        printf("syncVideo #QR = %d ", numProjector);
        for (i=0; i<MAX_PICO_SET; i++)
            // if (qrcode[i] > 0) 
            printf("%d ", qrcode[i]);
        printf("\n");
    }

    // calculate lagging time for my projector
    // find min frame number among projectors
    for (i=0; i<MAX_PICO_SET; i++) {
        if (qrcode[i] > 0 && qrcode[i] < min_counter) min_counter = qrcode[i];
    }

    if (min_counter == 999)   
        printf("ERROR: cannot get good qrcode, min_counter=%d\n", min_counter);   
    else
        printf("min frame number: %d\n", min_counter);   

    // calculate time2wait, if min_counter is my frame, my time2wait = 0
    if (tdata->myID == 1) tempWait = qrcode[0] - min_counter;
    else if (tdata->myID == 2) tempWait = qrcode[1] - min_counter;
    else if (tdata->myID == 3) tempWait = qrcode[2] - min_counter;
    else if (tdata->myID == 4) tempWait = qrcode[3] - min_counter;
  
    if (tempWait < 0) {
        tdata->time2wait = 0;
    }
    else {
      tdata->time2wait = tempWait;
    }
    printf(">>>>>>>> my ID:%d, time2wait: %d\n", tdata->myID, tempWait);   
    tdata->shotAnalyzed = 1;
    return tdata;
}

#define WAIT_FOR_ALL_PICO_SENDING_QR    5 // 30

int picoApp::loadQR(int qrnum)
{
    FILE *fp;
    char *fbp = 0;   
    char fileToOpen[30];
    int shift;
    
    sprintf(fileToOpen, "../../video/qrblob/QR%03d.rgb", qrnum);
    fp = fopen(fileToOpen, "r");
    fread(&qr_frame[0], 1, 230*230*3, fp);
    printf(">>>> loadQR QR%03d to qr_frame buffer\n", qrnum);

/*    
    if (qrnum % 2 == 0) shift = -150;
    else shift = 150;

    for (y=125; y<355; y++) {
        for (x=205+shift; x<435+shift; x++) {
            red = *pixel_ptr++;
            green = *pixel_ptr++;
            blue = *pixel_ptr++;

            location =  (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                        (y+vinfo.yoffset) * finfo.line_length;
            *((unsigned short int*)(fbp + location)) = ((red>>3)<<11)|((green>>2)<<5)|(blue>>3);
        }   
    }
  */  
    
    
    
    return 1;
}

int picoApp::getHomography(int BoardID)
{

    int fbfd = 0;
    int loopNum = 0;
    int synch = 0;
    int numBars = -1;

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    long int screensize = 0;
    char *fbp = 0;
    int x, y;
    int shift;
    long int location = 0;
    char fileToOpen[30];
    struct timeval tv;
    double prevBarTime;
    FILE *fp;
    const char *fifoname = "/tmp/test.fifo";

    unsigned char red, green, blue, tookShot = 0;
    unsigned char *video_frame; 
    unsigned char *pixel_ptr;
    video_frame = (unsigned char*)malloc(640*480*3);

    /* change rate from 0.2 to 0.3 */
    barRate = 0.3;
    framePeriod = 0.3;

    pthread_t thread2;
    thread_data thdata2; // instantiate passing thread data

    thdata2.myID = boardID;
    thdata2.shotAnalyzed = 0;
    thdata2.time2wait = 0;

    system("rm -f shot*");
    system("echo -e '\\033[?17;0;0c' > /dev/tty1");	// Disable cursor
    system("echo -e \\\\x1b[9\\;0] > /dev/tty1");		// Disable screensaver
    
    if (access (fifoname, F_OK))
        system("mkfifo /tmp/test.fifo");

    system("mplayer -nolirc -slave -quiet -input file=/tmp/test.fifo -vo null -vf screenshot -tv driver=v4l2 tv:// &");

    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (!fbfd) {
        ofLog(OF_LOG_ERROR, "Error: cannot open framebuffer device.\n");
        return 0;
    }

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        ofLog(OF_LOG_ERROR, "Error reading fixed information.\n");
        return 0;
    }

    // Get variable screen information */
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        ofLog(OF_LOG_ERROR, "Error reading variable information.\n");
        return 0;
    }

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    ofLog(OF_LOG_NOTICE, "screensize = %ld bytes\n", screensize);
    ofLog(OF_LOG_NOTICE, "vinfo.xres = %d\n", vinfo.xres);
    ofLog(OF_LOG_NOTICE, "vinfo.yres = %d\n", vinfo.yres);
    ofLog(OF_LOG_NOTICE, "vinfo.bits_per_pixel = %d\n", vinfo.bits_per_pixel);

    /* Map the device to memory */
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0); 
    if ((int)fbp == -1) {
    	ofLog(OF_LOG_ERROR, "Error: failed to map framebuffer device to memory.\n");
        return 0;
    }
    // ofLog(OF_LOG_NOTICE, "The framebuffer device was mapped to memory successfully.\n");

    gettimeofday(&tv, NULL);
    prevBarTime = (double)tv.tv_sec + (0.000001 * tv.tv_usec);

    while (1)
    {
        // STOP HERE UNTIL barRate
        gettimeofday(&tv, NULL);
        if ((double)tv.tv_sec + (0.000001 * tv.tv_usec) - prevBarTime < barRate)	continue; 

        numBars++;
        loopNum++;
        // ofLog(OF_LOG_NOTICE, "BAR%dLOOP%d ", numBars, loopNum);

        if ((loopNum == 1) || (numBars == MAX_FRAMES))
        {
            for (y=0; y<479; y++)
            for (x=0; x<640; x++)
            {
                location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                           (y+vinfo.yoffset) * finfo.line_length;
                *((unsigned short int*)(fbp + location)) = 0;
            }
            numBars = 1;
        }

        if (synch == 0 || thdata2.time2wait > 0)
        {
            if (thdata2.time2wait) 
            {
            	ofLog(OF_LOG_NOTICE, "getHomography done...wait %d\n", thdata2.time2wait);
                thdata2.time2wait--;
            }

            sprintf(fileToOpen, "../../video/qrblob/QR%03d.rgb", (numBars%2) + 98 + thdata2.myID * 100);
            // ofLog(OF_LOG_NOTICE, "sending QR#%d\n", (numBars%2) + 98 + thdata2.myID * 100);
            
            fp = fopen(fileToOpen, "r");
            fread(video_frame, 1, 640*480*3, fp);
            pixel_ptr = video_frame;

            // shifted positions for left and right QRs              
            if (numBars % 2 == 0) shift = -150;
            else shift = 150;

            // send a QR by updating pixels in the frame buffer
            for (y=125; y<355; y++) {
                for (x=205+shift; x<435+shift; x++) {
                    red = *pixel_ptr++;
                    green = *pixel_ptr++;
                    blue = *pixel_ptr++;
                   
                    // remove brightness reduction 
                    // red /= 3;
                    // green /= 3;
                    // blue /= 3;
                    
                    location =  (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                                (y+vinfo.yoffset) * finfo.line_length;
                    *((unsigned short int*)(fbp + location)) = ((red>>3)<<11)|((green>>2)<<5)|(blue>>3);
                }   
            }
        }
        else { // synch=1, finish get matrix                
        	ofLog(OF_LOG_NOTICE, "*** done screenShotGetHomography...\n");
            break;
        }
        
        // create thread1 to take screenshot of QRs, adjust WAIT_FOR_ALL_PICO_SENDING_QR time to have all pico sets sending their QRs 
        if (loopNum > WAIT_FOR_ALL_PICO_SENDING_QR && synch == 0)
        {
            // Take screenshots to analyze and sync
            if (tookShot == 0)
            {
            	ofLog(OF_LOG_NOTICE, "*** screenShotGetHomography thread...\n");
                pthread_create(&thread2, NULL, &screenShotGetHomography, &thdata2);
                tookShot = 1;
            }

            if (thdata2.shotAnalyzed) {
            	ofLog(OF_LOG_NOTICE, "*** done screenShotGetHomography, wait for others %d frames\n", thdata2.time2wait);
                synch = 1;
                // break; TEST // should we break here after finished
            }
        }
        
        prevBarTime = (double)tv.tv_sec + (0.000001 * tv.tv_usec);
    }

    system("echo 'stop' > /tmp/test.fifo");
    pthread_join(thread2, NULL);
    munmap(fbp, screensize);
    close(fbfd);
    return 1;
}

int picoApp::syncVideo(int BoardID)
{

    int fbfd = 0;
    int loopNum = 0;
    int sync = 0;
    int upperleft_x;
    int numBars = -1;
    int time2wait = 0;

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    long int screensize = 0;
    char *fbp = 0, convertCmd[200];
    int x, y, imageCounter, fileSize, shift;
    long int location = 0;
    char fileToOpen[30];
    struct timeval tv;
    double prevBarTime;
    FILE *fp;
    const char *fifoname = "/tmp/test.fifo";

    unsigned char red, green, blue, tookShot = 0;
    unsigned char *video_frame = (unsigned char*)malloc(640*480*3);
    unsigned char *pixel_ptr;
    
    /* change rate from 0.2 to 0.3 */
    barRate = 0.2;
    framePeriod = 0.2;

    pthread_t thread1;
    thread_data thdata1; // instantiate passing thread data

    thdata1.myID = boardID;
    thdata1.shotAnalyzed = 0;
    thdata1.time2wait = 0;

    system("rm -f shot*");
    system("echo -e '\\033[?17;0;0c' > /dev/tty1");	// Disable cursor
    system("echo -e \\\\x1b[9\\;0] > /dev/tty1");		// Disable screensaver
    
    if (access (fifoname, F_OK))
        system("mkfifo /tmp/test.fifo");

    system("mplayer -nolirc -slave -quiet -input file=/tmp/test.fifo -vo null -vf screenshot -tv driver=v4l2 tv:// &");
    
    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (!fbfd) {
    	ofLog(OF_LOG_ERROR, "Error: cannot open framebuffer device.\n");
        return 0;
    }
    // ofLog(OF_LOG_NOTICE, "The framebuffer device was opened successfully.\n");

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
    	ofLog(OF_LOG_ERROR, "Error reading fixed information.\n");
        return 0;
    }

    // Get variable screen information */
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
    	ofLog(OF_LOG_ERROR, "Error reading variable information.\n");
        return 0;
    }

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    ofLog(OF_LOG_NOTICE, "screensize = %ld\n", screensize);
    ofLog(OF_LOG_NOTICE, "vinfo.xres = %d\n", vinfo.xres);
    ofLog(OF_LOG_NOTICE, "vinfo.yres = %d\n", vinfo.yres);
    ofLog(OF_LOG_NOTICE, "vinfo.bits_per_pixel = %d\n", vinfo.bits_per_pixel);

    /* Map the device to memory */
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0); 

    if ((int)fbp == -1) {
    	ofLog(OF_LOG_ERROR, "Error: failed to map frame buffer device to memory.\n");
        return 0;
    }
    // ofLog(OF_LOG_NOTICE, "The framebuffer device was mapped to memory successfully.\n");

    gettimeofday(&tv, NULL);
    prevBarTime = (double)tv.tv_sec + (0.000001 * tv.tv_usec);

    while (1)
    {
        gettimeofday(&tv, NULL);
        if ((double)tv.tv_sec + (0.000001 * tv.tv_usec) - prevBarTime < barRate)	continue; 
        
        /* STEP6: done syncVideo, playing video now */
        if (startPlayVideo == true) {
        	ofLog(OF_LOG_NOTICE, ">>>>>>>>>>>> start play video = %d\n", startPlayVideo);
            break;
        } 

        /* STEP3: done detected QRs, start wait time to sync */
        if (sync == 0 && thdata1.shotAnalyzed)
        {
            sync = 1; 
            time2wait = thdata1.time2wait;
            ofLog(OF_LOG_NOTICE, "time2wait = %d\n", time2wait);
        }

        numBars++;
        loopNum++;
        // ofLog(OF_LOG_NOTICE, "loop%d ", loopNum);
        
        // clear screen
        if ((loopNum == 1) || ((numBars == MAX_FRAMES) && (!sync)))
        {
        	// ofLog(OF_LOG_NOTICE, "clear screen at beginning\n");
            for (y=0; y<479; y++)
            for (x=0; x<640; x++)
            {
                location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                           (y+vinfo.yoffset) * finfo.line_length;
                *((unsigned short int*)(fbp + location)) = 0;
            }
            numBars = 1;
        }

        upperleft_x = UPPERLEFT_X + (BAR_WIDTH + BAR_DISTANCE) * (numBars - 1);
        
        /* STEP1: keep sending out QRs,
                  even after sync, until reach MAX_FRAMES */
        if (sync == 0 || (loopNum < MAX_FRAMES)) {
            /* STEP4: skip if time2wait > 0, else continue until loopNumber reaching MAX_FRAMES */
            if (sync == 1) {
                if (time2wait)
                {
                    time2wait --;
                    ofLog(OF_LOG_NOTICE, "time2wait = %d\n", time2wait);

                    if (loopNum)
                        loopNum --;
                    else
                    	ofLog(OF_LOG_ERROR, "wrong state, loopNumber = %d\n", loopNum);
                    
                    /* freeze to update the frame number */
                    prevBarTime = (double)tv.tv_sec + (0.000001 * tv.tv_usec);
                    continue;
                }
            }
            
            sprintf(fileToOpen, "../../video/qrblob/QR%03d.rgb", numBars + thdata1.myID * 100);
            fp = fopen(fileToOpen, "r");
            if (fp == NULL) {
            	ofLog(OF_LOG_ERROR, ">>>>> QR file read error, numBars %d, thdata1.myID %d \n", numBars, thdata1.myID);
                return 0;
            }
            fp = fopen(fileToOpen, "r");
            fread(video_frame, 1, 640*480*3, fp);
            pixel_ptr = video_frame;

            /* clear screen */
            for (y=0; y<479; y++) {
                for (x=0; x<640; x++) {
                    location =  (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                                (y+vinfo.yoffset) * finfo.line_length;
                                *((unsigned short int*)(fbp + location)) = 0;
                }
            }

            // shifted positions for left and right QRs              
            if (numBars % 2 == 0) shift = -150;
            //else if (numBars % 3 == 1) shift = 0;
            else shift = 150;
            //shift = 0;

            // send a QR by updating pixels in the frame buffer
            for (y=125; y<355; y++) {
                for (x=205+shift; x<435+shift; x++) {
                    red = *pixel_ptr++;
                    green = *pixel_ptr++;
                    blue = *pixel_ptr++;

                    // remove brightness reduction 
                    // red /= 3;
                    // green /= 3;
                    // blue /= 3;

                    location =  (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                                (y+vinfo.yoffset) * finfo.line_length;
                    *((unsigned short int*)(fbp + location)) = ((red>>3)<<11)|((green>>2)<<5)|(blue>>3);
                }   
            }
            
        }
        /* STEP5: REACH MAX FRAMES AND sync = 1 => OK to start playing video */ 
        else { // wait=0, sync=1, start playing now                
        	ofLog(OF_LOG_NOTICE, "start playing video now, loopNum=%d, sync=%d\n", loopNum, sync);
            startPlayVideo = true;
        }

        /* STEP2: start after a while and wait for shotAnalyzed then skip with sync = 1
           create thread1 to take screenshot of QRs, adjust WAIT_FOR_ALL_PICO_SENDING_QR time to have all pico sets sending their QRs 
         */ 
        if (loopNum > WAIT_FOR_ALL_PICO_SENDING_QR && thdata1.time2wait == 0 && sync == 0) {
            // Take screenshots to analyze and sync
            if (tookShot == 0) {
                pthread_create(&thread1, NULL, &screenShotSyncVideo, &thdata1);
                tookShot = 1;
            }
        }
        
        prevBarTime = (double)tv.tv_sec + (0.000001 * tv.tv_usec);
    } // end while loop

    system("echo 'stop' > /tmp/test.fifo");
    pthread_join(thread1, NULL);
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}

void picoApp::calFading(void)
{
    int i,j,k,x,y;
    double w;
    double xfade = 1.0;
    double yfade = 1.0;

    /* note: since the video has not been loaded yet, the width=height=0 so we 
       try to use the default resolution 640x480 for this calculation */
    printf("calculation fading at setup time with default resolution %dx%d \n", WIDTH, HEIGHT);

    for (i=0; i<HEIGHT; i++) {             
        for (j=0; j<WIDTH; j++) {    
            w = matrix[2][0] * j + matrix[2][1] * i + matrix[2][2];
            x = (int)((matrix[0][0] * j + matrix[0][1] * i + matrix[0][2])/w);
            y = (int)((matrix[1][0] * j + matrix[1][1] * i + matrix[1][2])/w);
            // printf(" p[%d %d]=%d %d d% ",i,j,x,y,w);

            if (x >= 0 && x < WIDTH && y > 0 && y < HEIGHT) {
                if (j >= getLeftX(i) && j <= getRightX(i)) {
                    xfade = getXFade(j,i);
                    if (xfade < 1) 
                        xfadeMat[i][j] = xfade*256;
                    else
                        xfadeMat[i][j] = 255;
                    // printf("xfadeMat[%d %d]=%d ", i,j,xfadeMat[i][j]);
                }
                
                if (i >= getTopY(j) && i <= getBottomY(j)) {
                    yfade = getYFade(j,i);
                    if (yfade < 1) 
                        yfadeMat[i][j] = yfade*256;
                    else
                        yfadeMat[i][j] = 255;
                    // printf("yfadeMat[%d %d]=%d ", i, j, yfadeMat[i][j]);
                }
            }
        }
    }
}

//--------------------------------------------------------------
void picoApp::mousePressed(int x, int y, int button){
	ofLog(OF_LOG_NOTICE, "mousePressed = (%i,%i - button %i), nClick = %i", x, y, button, nClick);
	switch (button) {
		case 0: /* left button */
			if (nClick >= 3) {
				dst[3].set(x,y);
				nClick = 0;
			}
			else {
				dst[nClick].set(x,y);
				nClick ++;
			}
			setPositionByMouse = true;
			break;
		case 2: /* right button */
			setPositionByMouse = false;
			nClick = 0;
			break;
		default:;
	}
}
