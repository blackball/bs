/**
 * Unit test for BGooth, aka. ViBe cpp version.
 * Image I/O is based on OpenCV.
 * 
 * @blackball
 */

#include "bgooth.h"
#include <cv.h>
#include <highgui.h>
#include <stdio.h>

int test_gray()
{
        CvCapture *cap;
        IplImage *frame, *gray, *segImg;
        unsigned char *segMap;
        BGooth *bg;

        cap = cvCaptureFromCAM(0);
        if (!cap) return -1;

        frame = cvQueryFrame(cap);

        gray = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1);
        cvCvtColor(frame, gray, CV_BGR2GRAY);

        bg = new BGooth(); // use default parameters

        if (bg->ModelInit((const unsigned char*)(gray->imageData), 
                          gray->width, gray->widthStep, gray->height, gray->nChannels)){
                fprintf(stderr, "Error: Could not Initialize correctly!\n");
                return -1;
        }
        // one channel w*h segmentation map. foreground 0xFF, else 0x00
        segImg = cvCreateImage(cvSize(gray->width, gray->height), IPL_DEPTH_8U, 1);
        segMap = (unsigned char*)(segImg->imageData);

        cvNamedWindow("orig", 1);
        cvNamedWindow("segmap", 1);
        while ( frame = cvQueryFrame(cap) )
        {
                cvCvtColor(frame, gray, CV_BGR2GRAY);
                bg->ModelUpdate((const unsigned char*)(gray->imageData), segMap);
                // here you could use seg_map, or show it
                cvShowImage("orig", frame);
                cvShowImage("segmap", segImg);
                if ('q' == cvWaitKey(5)) break;
        }

        cvReleaseImage(&segImg);
        cvReleaseImage(&gray);
        cvReleaseCapture( &cap );
        cvDestroyWindow("orig");
        cvDestroyWindow("segmap");
        delete bg;

        return 0;
}


int test_color()
{
        CvCapture *cap;
        IplImage *frame, *segImg;
        unsigned char *segMap;
        BGooth *bg;

        cap = cvCaptureFromCAM(0);
        if (!cap) return -1;

  
        frame = cvQueryFrame(cap);

        bg = new BGooth();  // use default parameters
        if (bg->ModelInit((const unsigned char*)(frame->imageData), 
                          frame->width, frame->widthStep, frame->height, frame->nChannels)) {
                fprintf(stderr, "Error: Could not Initialize correctly!\n");
                return -1;
        }

        // one channel w*h segmentation map. foreground 0xFF, else 0x00
        segImg = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1);
        if (!segImg) {
                fprintf(stderr, "Error: creage segmentation image failed!\n");
                exit(0);
        }

        segMap = (unsigned char*)(segImg->imageData);

        cvNamedWindow("orig", 1);
        cvNamedWindow("segmap", 1);
        while ( frame = cvQueryFrame(cap) )
        {
                bg->ModelUpdate((const unsigned char*)(frame->imageData), segMap);
                // here you could use seg_map, or show it
                cvShowImage("orig", frame);
                cvShowImage("segmap", segImg);
                if ('q' == cvWaitKey(5)) break;
        }

        cvReleaseImage(&segImg);
        cvReleaseCapture( &cap );
        cvDestroyWindow("orig");
        cvDestroyWindow("segmap");
        delete bg;

        return 0;
}

#define TEST
#ifdef TEST

int main(int ac, char *av[])
{
        // test_color();
        test_gray();
        return 0;
}

#endif




