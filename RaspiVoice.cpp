// raspivoice
// Based on:
// http://www.seeingwithsound.com/hificode_OpenCV.cpp
// C program for soundscape generation. (C) P.B.L. Meijer 1996
// hificode.c modified for camera input using OpenCV. (C) 2013
// Last update: December 29, 2014; released under the Creative
// Commons Attribution 4.0 International License (CC BY 4.0),
// see http://www.seeingwithsound.com/im2sound.htm for details
// License: https://creativecommons.org/licenses/by/4.0/

#include <iostream>
#include "RaspiVoice.h"
#include "ImageToSoundscape.h"
#include "test_image.h"
#include "printtime.h"

RaspiVoice::RaspiVoice(RaspiVoiceOptions opt) :
	rows(opt.rows),
	columns(opt.columns),
	image_source(opt.image_source),
	preview(opt.preview),
	use_bw_test_image(opt.use_bw_test_image),
	verbose(opt.verbose),
	opt(opt)
{
	if ((image_source == 0) && (opt.input_filename == "")) //Test image, fixed size
	{
		rows = 64;
		columns = 64;
	}
	init();

	i2ssConverter = new ImageToSoundscapeConverter(rows, columns, opt.freq_lowest, opt.freq_highest, opt.sample_freq_Hz, opt.total_time_s, opt.use_exponential, opt.use_stereo, opt.use_delay, opt.use_fade, opt.use_diffraction, opt.use_bspline, opt.speed_of_sound_m_s, opt.acoustical_size_of_head_m);
}

RaspiVoice::~RaspiVoice()
{
	if (i2ssConverter)
	{
		delete(i2ssConverter);
	}

	if (image_source == 1)
	{
		raspiCam.release();
	}
}


void RaspiVoice::init()
{
	if (verbose)
	{
		printtime("");
		printtime("Init...");
	}

	image = new std::vector<float>(rows*columns);

	if (image_source == 0) //Test image
	{
		if (opt.input_filename == "")
		{
			initTestImage();
		}
		else
		{
			initFileImage();
		}
		
	}
	else if (image_source == 1) //RaspiCAM
	{
		initRaspiCam();
	}
	else if (image_source >= 2)
	{
		initUsbCam();
	}

	if (preview)
	{
		cv::namedWindow("RaspiVoice Preview", CV_WINDOW_NORMAL);
	}
}

void RaspiVoice::initTestImage()
{
	if (verbose)
	{
		if (use_bw_test_image)
		{
			std::cout << "Using B/W test image" << std::endl;
		}
		else
		{
			std::cout << "Using grayscale test image" << std::endl;
		}
	}

	// Set hard-coded image
	for (int j = 0; j < columns; j++)
	{
		for (int i = 0; i < rows; i++)
		{
			if (use_bw_test_image)
			{
				if (P_bw[rows - i - 1][j] != '#')
				{
					(*image)[IDX2D(i, j)] = 1.0; // white;
				}
				else
				{
					(*image)[IDX2D(i, j)] = 0.0; // black;
				}
			}
			else if (P_grayscale[i][j] > 'a')
			{
				(*image)[IDX2D(rows - i - 1, j)] = pow(10.0, (P_grayscale[i][j] - 'a' - 15) / 10.0);   // 2dB steps
			}
			else
			{
				(*image)[IDX2D(rows - i - 1, j)] = 0.0;
			}
		}
	}
}

void RaspiVoice::initFileImage()
{
	if (verbose)
	{
		std::cout << "Checking input file " << opt.input_filename << std::endl;

		//Test read:
		cv::Mat mat = cv::imread(opt.input_filename.c_str(), CV_LOAD_IMAGE_GRAYSCALE);

		if (mat.data == NULL)
		{
			throw(std::runtime_error("Cannot read input file as image."));
		}

		std::cout << "ok" << std::endl;
	}
}


void RaspiVoice::initRaspiCam()
{
	if (verbose)
	{
		std::cout << "Opening RaspiCam..." << std::endl;
	}

	raspiCam.set(CV_CAP_PROP_FORMAT, CV_8UC1);
	raspiCam.set(CV_CAP_PROP_FRAME_WIDTH, 320);
	raspiCam.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
	if (!raspiCam.open())
	{
		throw(std::runtime_error("Error opening RaspiCam."));
	}
	else
	{
		if (verbose)
		{
			std::cout << "Ok" << std::endl;
		}
	}
}

void RaspiVoice::initUsbCam()
{
	int cam_id = image_source - 2;  // OpenCV camera index 0+

	if (verbose)
	{
		std::cout << "Opening USB camera..." << std::endl;
	}

	cap.open(cam_id);
	if (!cap.isOpened())
	{
		throw(std::runtime_error("Could not open camera."));
	}
	// Setting standard capture size, may fail; resize later
	cv::Mat rawImage;
	cap.read(rawImage);  // Dummy read needed with some devices
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 176);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 144);

	if (verbose)
	{
		std::cout << "Ok" << std::endl;
	}
}

void RaspiVoice::readImage()
{
	cv::Mat rawImage;
	cv::Mat processedImage;

	if (verbose)
	{
		printtime("ReadImage start");
	}
	if ((image_source == 0) && (opt.input_filename != ""))
	{
		rawImage = cv::imread(opt.input_filename.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
		processedImage = rawImage;
	}
	else if (image_source == 1) //RaspiCAM
	{
		raspiCam.grab();
		raspiCam.retrieve(rawImage);
		if (opt.read_frames > 1)
		{
			for (int r = 1; r < opt.read_frames; r++)
			{
				raspiCam.grab();
				raspiCam.retrieve(rawImage);
			}
		}
		//cv::imwrite("/var/tmp/raspicam_frame.jpg", processedImage);
		processedImage = rawImage;
	}
	else if (image_source >= 2) //OpenCv camera
	{

		cap.read(rawImage);
		if (opt.read_frames > 1)
		{
			for (int r = 1; r < opt.read_frames; r++)
			{
				cap.read(rawImage);
			}
		}

		if (rawImage.empty())
		{
			throw(std::runtime_error("Error reading frame from camera."));
		}

		cv::cvtColor(rawImage, processedImage, CV_BGR2GRAY);
	}

	if (verbose)
	{
		printtime("ProcessImage start");
	}

	if ((image_source > 0) || (opt.input_filename != ""))
	{
		if (processedImage.rows != rows || processedImage.cols != columns)
		{
			cv::resize(processedImage, processedImage, cv::Size(columns, rows));
		}

		if (opt.edge_detection_opacity > 0.0)
		{
			cv::Mat blurImage;
			cv::Mat edgesImage;
			int ratio = 3;
			int kernel_size = 3;
			int lowThreshold = opt.edge_detection_threshold;
			if (lowThreshold <= 0)
			{
				lowThreshold = 127;
			}
			cv::blur(processedImage, blurImage, cv::Size(3, 3));
			cv::Canny(blurImage, edgesImage, lowThreshold, lowThreshold*ratio, kernel_size);

			double alpha = opt.edge_detection_opacity;
			if (alpha > 1.0)
			{
				alpha = 1.0;
			}
			double beta = (1.0 - alpha);
			cv::addWeighted(edgesImage, alpha, processedImage, beta, 0.0, processedImage);
		}

		if (opt.threshold > 0)
		{
			if (opt.threshold < 255)
			{
				cv::threshold(processedImage, processedImage, opt.threshold, 255, cv::THRESH_BINARY);
			}
			else
			{
				//Auto threshold:
				cv::threshold(processedImage, processedImage, 127, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
			}
			
		}

		if (opt.negative_image)
		{
			cv::Mat sub_mat = cv::Mat::ones(processedImage.size(), processedImage.type()) * 255;
			cv::subtract(sub_mat, processedImage, processedImage);
		}

		if ((opt.flip >= 1) && (opt.flip <= 3))
		{
			int flipCode;
			if (opt.flip == 1) //h
			{
				flipCode = 1;
			}
			else if (opt.flip == 2) //v
			{
				flipCode = 0;
			}
			else if (opt.flip == 3) //h+v
			{
				flipCode = -1;
			}

			cv::flip(processedImage, processedImage, flipCode);
		}

		if (preview)
		{
			//Screen views
			//imwrite("raspivoice_capture_raw.jpg", rawImage);
			//imwrite("raspivoice_capture_scaled_gray.jpg", processedImage);
			cv::imshow("RaspiVoice Preview", processedImage);

			cv::waitKey(200);
		}

		/* Set live camera image */
		for (int j = 0; j < columns; j++)
		{
			for (int i = 0; i < rows; i++)
			{
				int mVal = processedImage.at<uchar>(rows - 1 - i, j) / 16;
				if (mVal == 0)
				{
					(*image)[IDX2D(i, j)] = 0;
				}
				else
				{
					(*image)[IDX2D(i, j)] = pow(10.0, (mVal - 15) / 10.0);   // 2dB steps
				}
			}
		}
	}

}

void RaspiVoice::PlayFrame()
{
	readImage();

	if (verbose)
	{
		printtime("Process start");
	}
	i2ssConverter->Process(*image);

	AudioData &audioData = i2ssConverter->GetAudioData();
	audioData.DeviceName = opt.audio_device;
	audioData.Verbose = verbose;

	if (verbose)
	{
		printtime("Playing audio");
	}
	audioData.Play();
	//audioData.SaveToWavFile("raspivoice_output.wav");
	//audioData.PlayWav(FNAME);
}


