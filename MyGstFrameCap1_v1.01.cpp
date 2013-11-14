/**********************************************************************/
// Get a desired frame from video file using gstreamer and display it using OpenCV

/**********************************************************************/
// gstreamer includes and libraries
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#pragma comment( lib, "gstreamer-1.0.lib" )
#pragma comment( lib, "glib-2.0.lib" )
#pragma comment( lib, "gobject-2.0.lib" )
#pragma comment( lib, "gstapp-1.0.lib" )

/**********************************************************************/
// opencv includes and libraries
#include <opencv2/opencv.hpp>
#ifdef _DEBUG
#pragma comment( lib, "comctl32" )
#pragma comment( lib, "opencv_core246d" )
#pragma comment( lib, "opencv_highgui246d" )
#ifndef _DLL
#pragma comment( lib, "zlibd" )
#pragma comment( lib, "IlmImfd" )
#pragma comment( lib, "libjasperd" )
#pragma comment( lib, "libjpegd" )
#pragma comment( lib, "libpngd" )
#pragma comment( lib, "libtiffd" )
#endif
#else
#pragma comment( lib, "comctl32" )
#pragma comment( lib, "opencv_core246" )
#pragma comment( lib, "opencv_highgui246" )
#ifndef _DLL
#pragma comment( lib, "zlib" )
#pragma comment( lib, "IlmImf" )
#pragma comment( lib, "libjasper" )
#pragma comment( lib, "libjpeg" )
#pragma comment( lib, "libpng" )
#pragma comment( lib, "libtiff" )
#endif
#endif

/**********************************************************************/
void MyfCopyRGBbuf2CvMat( cv::Mat& oMatFrameInp, unsigned char *pDataInp )
{
	int nDi = 0;
	for( int nRi = 0; nRi < oMatFrameInp.rows; nRi++ )
	{
		for( int nCi = 0; nCi < oMatFrameInp.cols; nCi++ )
		{
			oMatFrameInp.data[oMatFrameInp.step[0] * nRi + oMatFrameInp.step[1] * nCi + 2] = pDataInp[nDi++]; // R
			oMatFrameInp.data[oMatFrameInp.step[0] * nRi + oMatFrameInp.step[1] * nCi + 1] = pDataInp[nDi++]; // G
			oMatFrameInp.data[oMatFrameInp.step[0] * nRi + oMatFrameInp.step[1] * nCi + 0] = pDataInp[nDi++]; // B
		}
	}
}

/**********************************************************************/
void MyfDispGstSampleInOpenCvWin( GstSample *pstGstSampleInp, gchar *szgWinTitle )
{
	if( !pstGstSampleInp ) { g_print( "*** Error: Could not make snapshot (may be due to eos)."); return; }

	GstCaps *pstGstCaps1;
	pstGstCaps1 = gst_sample_get_caps( pstGstSampleInp );
	if( !pstGstCaps1 ) { g_print( "*** Error: gst_sample_get_caps fails to get sample cpas." ); return; }

	GstStructure *pstGstSt1;
	pstGstSt1 = gst_caps_get_structure( pstGstCaps1, 0 );

	gint nFrameWidth, nFrameHeight;
	gboolean nGboolRes1;
	nGboolRes1 = gst_structure_get_int( pstGstSt1, "width", &nFrameWidth );
	nGboolRes1 |= gst_structure_get_int( pstGstSt1, "height", &nFrameHeight );
	if( !nGboolRes1 ) { g_print( "*** Error: gst_structure_get_int fails to get snapshot dimension."); return; }

	GstBuffer *pstGstBuffer1;
	pstGstBuffer1 = gst_sample_get_buffer( pstGstSampleInp );
	GstMapInfo stGstMapInfo1;
	gst_buffer_map( pstGstBuffer1, &stGstMapInfo1, GST_MAP_READ );

	cv::Mat oMatTstFrame;
	oMatTstFrame = cv::Mat::zeros( nFrameHeight, nFrameWidth, CV_8UC3 );
	MyfCopyRGBbuf2CvMat( oMatTstFrame, stGstMapInfo1.data );

	gst_buffer_unmap( pstGstBuffer1, &stGstMapInfo1 );

	cv::namedWindow( szgWinTitle, CV_WINDOW_AUTOSIZE );
	cv::imshow( szgWinTitle, oMatTstFrame );
}

//********************************************************************************
int	main( int argc, char *argv[] )
{
	gst_init( &argc, &argv );

	// Describe pipeline
	gchar *szgPipelineDescStr1;
	szgPipelineDescStr1 = g_strdup_printf( "uridecodebin uri=\"file:///D:/Test1.avi\" ! videoconvert ! videoscale ! appsink name=sink caps=\"video/x-raw,format=RGB\"" );

	// Create pipeline
	GstElement *pstGstElemPipeline1;
	GError *pstGErr1 = NULL;
	pstGstElemPipeline1 = gst_parse_launch( szgPipelineDescStr1, &pstGErr1 );
	if( pstGErr1 != NULL )
	{
		g_print( "could not construct pipeline: %s\n", pstGErr1->message );
		g_error_free( pstGErr1 ); return( -1 );
	}

	// Set to GST_STATE_PAUSED
	GstStateChangeReturn nStateChgRet;
	nStateChgRet = gst_element_set_state( pstGstElemPipeline1, GST_STATE_PAUSED );
	if( nStateChgRet == GST_STATE_CHANGE_FAILURE ) goto Cleanup;
	nStateChgRet = gst_element_get_state( pstGstElemPipeline1, NULL, NULL, 5 * GST_SECOND );
	if( nStateChgRet == GST_STATE_CHANGE_FAILURE ) goto Cleanup;

	// Set desired frame number to be displayed
	gint64 nDesiredMediaPosFrameNum = 120;

	// Init required variables
	GstElement *pstGstElemAppSink1;
	pstGstElemAppSink1 = gst_bin_get_by_name( GST_BIN( pstGstElemPipeline1 ), "sink" );
	gboolean nGboolRes1;
	char *szgWinTitle;

	// Seek by GST_FORMAT_DEFAULT
	nGboolRes1 = gst_element_seek_simple( pstGstElemPipeline1, GST_FORMAT_DEFAULT,	 (GstSeekFlags) ( GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_FLUSH ), nDesiredMediaPosFrameNum );
	if( !nGboolRes1 ) { g_print( "*** Error: gst_element_seek_simple by GST_FORMAT_DEFAULT failed!\n"); }

	// Read sample
	GstSample *pstGstSample1;
	pstGstSample1 = gst_app_sink_pull_preroll( (GstAppSink *) pstGstElemAppSink1 ); // or g_signal_emit_by_name( pstGstElemAppSink1, "pull-preroll", &pstGstSample1, NULL );

	// Display sample
	szgWinTitle = g_strdup_printf( "Frame #%lld", nDesiredMediaPosFrameNum );
	MyfDispGstSampleInOpenCvWin( pstGstSample1, szgWinTitle );


	// Set desired position by time to be displayed
	gint64 nDesiredMediaPosNanoSec = 1000 * GST_MSECOND;

	// Seek by GST_FORMAT_DEFAULT
	nGboolRes1 = gst_element_seek_simple( pstGstElemPipeline1, GST_FORMAT_TIME,	 (GstSeekFlags) ( GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_FLUSH ), nDesiredMediaPosNanoSec );
	if( !nGboolRes1 ) { g_print( "*** Error: gst_element_seek_simple by GST_FORMAT_TIME failed!\n"); }

	// Read sample
	GstSample *pstGstSample2;
	pstGstSample2 = gst_app_sink_pull_preroll( (GstAppSink *) pstGstElemAppSink1 );

	// Display sample
	szgWinTitle = g_strdup_printf( "Time %" GST_TIME_FORMAT, GST_TIME_ARGS( nDesiredMediaPosNanoSec ) );
	MyfDispGstSampleInOpenCvWin( pstGstSample2, szgWinTitle );

Cleanup:
	gst_element_set_state( pstGstElemPipeline1, GST_STATE_NULL );
	gst_object_unref( pstGstElemPipeline1 );

	g_print( "Press any key to finish...\n"); 
	cvWaitKey( 0 );
	return( 0 );
}
