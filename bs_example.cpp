/* Unit test for BS */

#include "bs.h"

#include <stdio.h>
#include <cv.h>
#include <highgui.h>

/* test gray-scale image */
static void test_color() {
        CvCapture *cap;
        IplImage *frame, *segmap;
        bs_t bg;
	
        if (bs_create(&bg)) {
                fprintf(stderr, "Error:bs module create failed!\n");
                return ;
        }
        cap = cvCaptureFromCAM(0);
        if (!cap) {
                fprintf(stderr, "ErrorL:Open camera failed!\n");
                return ;
        }

        frame = cvQueryFrame(cap);
        if (bs_initModel(&bg, (unsigned char*)frame->imageData, frame->width, frame->widthStep, frame->height, frame->nChannels)){
                fprintf(stderr, "Error:initialize bs module failed!\n");
                return ;
        }

        segmap = cvCreateImageHeader(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1);

        cvNamedWindow("frame",  1);
        cvNamedWindow("segmap", 1);
        while(frame = cvQueryFrame(cap)) {
		
                bs_update(&bg, (unsigned char*)frame->imageData, (unsigned char*)segmap->imageData);
                cvShowImage("frame", frame);
		
                segmap->imageData = (char*)bs_getSegMapPtr(bg);
                cvShowImage("segmap", segmap);
		
                if (27 == cvWaitKey(5) ) break;
        }

        cvReleaseCapture(&cap);
        cvReleaseImageHeader(&segmap);
        cvDestroyWindow("frame");
        cvDestroyWindow("segmap");

}

static void test_gray(){
        CvCapture *cap;
        IplImage *frame, *segmap, *gray;
        bs_t bg;
	
        if (bs_create(&bg)) {
                fprintf(stderr, "Error:bs module create failed!\n");
                return ;
        }
        cap = cvCaptureFromCAM(0);
        if (!cap) {
                fprintf(stderr, "ErrorL:Open camera failed!\n");
                return ;
        }

        frame = cvQueryFrame(cap);
        gray = cvCreateImage(cvGetSize(frame), 8, 1);
        segmap = cvCreateImageHeader(cvSize(frame->width, frame->height), 8, 1);
        cvCvtColor(frame, gray, CV_BGR2GRAY);

        if (bs_initModel(&bg, (unsigned char*)gray->imageData, gray->width, gray->widthStep, gray->height, gray->nChannels)){
                fprintf(stderr, "Error:initialize bs module failed!\n");
                return ;
        }


        cvNamedWindow("frame",  1);
        cvNamedWindow("segmap", 1);
        while(frame = cvQueryFrame(cap)) {
                cvCvtColor(frame, gray, CV_BGR2GRAY);
                bs_update(&bg, (unsigned char*)gray->imageData, (unsigned char*)segmap->imageData);
                cvShowImage("frame", frame);
		
                segmap->imageData = (char*)bs_getSegMapPtr(bg);
                cvShowImage("segmap", segmap);
		
                if ('q' == cvWaitKey(5) ) break;
        }

        cvReleaseCapture(&cap);
        cvReleaseImageHeader(&segmap);
        cvReleaseImage(&gray);
        cvDestroyWindow("frame");
        cvDestroyWindow("segmap");
}

int main(int ac, char *av[])
{
        test_color();
        return 0;
}
