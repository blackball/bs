/**
 * Unit test for BGooth, aka. ViBe cpp version.
 * Unit test is based on OpenCV.
 * 
 * @blackball
 */

#include "vibecpp.h"
#include <cv.h>
#include <highgui.h>
#include <stdio.h>

int test_gray()
{
  CvCapture *cap = cvCaptureFromAVI("D:/finger/finger_video2/Video2.wmv");
  if (!cap) return -1;

  BGooth bg; // use default parameters
  IplImage *frame = cvQueryFrame(cap); 
  IplImage *gray = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1);
  cvCvtColor(frame, gray, CV_BGR2GRAY);
  if (bg.ModelInit((const unsigned char*)(gray->imageData), 
                   gray->width, gray->height, gray->nChannels))
  {
    fprintf(stderr, "Error: Could not Initialize correctly!\n");
    return -1;
  }
  // one channel w*h segmentation map. foreground 0xFF, else 0x00
  IplImage *segImg = cvCreateImage(cvSize(gray->width, gray->height), IPL_DEPTH_8U, 1);
  unsigned char *seg_map = (unsigned char*)(segImg->imageData);

  cvNamedWindow("orig", 1);
  cvNamedWindow("segmap", 1);
  while ( frame = cvQueryFrame(cap) )
  {
    cvCvtColor(frame, gray, CV_BGR2GRAY);
    bg.ModelUpdate((const unsigned char*)(gray->imageData), seg_map);
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

  return 0;
}


int test_color()
{
  CvCapture *cap = cvCaptureFromAVI("D:/finger/finger_video2/Video2.wmv");
  if (!cap) return -1;

  BGooth bg; // use default parameters
  IplImage *frame = cvQueryFrame(cap); 
  if (bg.ModelInit((const unsigned char*)(frame->imageData), 
                   frame->width, frame->height, frame->nChannels))
  {
    fprintf(stderr, "Error: Could not Initialize correctly!\n");
    return -1;
  }
  // one channel w*h segmentation map. foreground 0xFF, else 0x00
  IplImage *segImg = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1);
  unsigned char *seg_map = (unsigned char*)(segImg->imageData);

  cvNamedWindow("orig", 1);
  cvNamedWindow("segmap", 1);
  while ( frame = cvQueryFrame(cap) )
  {
    bg.ModelUpdate((const unsigned char*)(frame->imageData), seg_map);
    // here you could use seg_map, or show it
    cvShowImage("orig", frame);
    cvShowImage("segmap", segImg);
    if ('q' == cvWaitKey(5)) break;
  }

  cvReleaseImage(&segImg);
  cvReleaseCapture( &cap );
  cvDestroyWindow("orig");
  cvDestroyWindow("segmap");

  return 0;
}

#ifdef TEST

int main(int ac, char *av[])
{
  //test_color();
  test_gray();
  return 0;
}

#endif




