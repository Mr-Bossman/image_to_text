#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <opencv2/freetype.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <unistd.h>

static std::vector<cv::Mat> splitImage(const cv::Mat &image, int M, int N);
static void Pcolor(char c, cv::Vec3f fgcolor, cv::Vec3f bgcolor);
static cv::Mat text(int fontHeight, std::string text, const cv::Ptr<cv::freetype::FreeType2> &ft2, cv::Scalar bg, cv::Scalar fg, int &height, int &width);
static cv::Mat test();
static cv::Mat textImage(const cv::Mat &centers);
static cv::Mat img_pallet(cv::Mat img, int pallet, cv::Mat &centers);
static std::vector<cv::Mat> splitImage(const cv::Mat &image, int M, int N);
static cv::Mat combineImage(const std::vector<cv::Mat> &matrix, int N);
static void display(cv::Mat img, int height, int width);

int main(int argc, char **argv)
{
	int height = strtol(argv[1], NULL, 10);
	int width = strtol(argv[2], NULL, 10);
	cv::VideoCapture cap("test.mp4"); // video

	while (1)
	{
		cv::Mat img;
		if (!cap.read(img))
		{
			break;
		}
		display(img, height, width);
	}
	//cv::waitKey(0);
	//cv::destroyAllWindows();

	return 0;
}
static void display(cv::Mat img, int height, int width)
{
	std::vector<cv::Mat> pic;
	auto matrix = splitImage(img, width, height);
	for (auto cut : matrix)
	{
		cv::Mat clusters;
		img_pallet(cut, 2, clusters);
		pic.push_back(clusters);
	}

	for (size_t i = 0; i < pic.size(); i++)
	{
		Pcolor('X', pic[i].at<cv::Vec3f>(0), pic[i].at<cv::Vec3f>(1));
		if (i % width == 0)
			puts("");
		//matrix[i] = textImage(clusters);
	}
	printf("\033[0m\n\033[2J\033[0H");
	//imshow("img", combineImage(matrix,HEIGHT));
}
static void Pcolor(char c, cv::Vec3f fgcolor, cv::Vec3f bgcolor)
{
	printf("\033[38;2;%u;%u;%um", (uint)fgcolor[2], (uint)fgcolor[1], (uint)fgcolor[0]);
	printf("\033[48;2;%u;%u;%um", (uint)bgcolor[2], (uint)bgcolor[1], (uint)bgcolor[0]);
	printf("%c", c);
}

static cv::Mat text(int fontHeight, std::string text, const cv::Ptr<cv::freetype::FreeType2> &ft2, cv::Scalar bg, cv::Scalar fg, int &height, int &width)
{
	int thickness = -1;
	int linestyle = 8;
	int baseline = 0;
	cv::Size textSize = ft2->getTextSize(text, fontHeight, thickness, &baseline);
	if (thickness > 0)
	{
		baseline += thickness;
	}
	height = textSize.height;
	width = textSize.width;
	cv::Mat img(textSize.width, textSize.height, CV_8UC3, bg);
	cv::Point textOrg((img.cols - textSize.width) / 2,
			  (img.rows + textSize.height) / 2);
	ft2->putText(img, text, textOrg, fontHeight,
		     fg, thickness, linestyle, true);
	return img;
}

static cv::Mat test()
{
	cv::Ptr<cv::freetype::FreeType2> ft2 = cv::freetype::createFreeType2();
	ft2->loadFontData("./test.ttf", 0);
	int height, width;
	std::string str = "abcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()";

	for (auto c : str)
	{
		std::string b;
		b = c;
		text(12, b, ft2, cv::Scalar(0, 0, 0), cv::Scalar(0, 0, 0), height, width);
	}
	return cv::Mat(10, 10, CV_8UC3);
}
static cv::Mat textImage(const cv::Mat &centers)
{
	cv::Ptr<cv::freetype::FreeType2> ft2 = cv::freetype::createFreeType2();
	ft2->loadFontData("./test.ttf", 0);
	int height, width;
	std::string b = "X";
	cv::Mat tmp = text(20, b, ft2, centers.at<cv::Vec3f>(0), centers.at<cv::Vec3f>(1), height, width), ret;
	cv::resize(tmp, ret, cv::Size(20, 20), cv::INTER_CUBIC);
	return tmp;
}
static cv::Mat img_pallet(cv::Mat img, int pallet, cv::Mat &centers)
{
	img.convertTo(img, CV_32F);
	cv::Mat samples, label, result(img.size(), CV_8UC3);
	samples = img.reshape(1, img.total());
	double ret = cv::kmeans(samples, pallet, label, cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 10, 0.1),
				10, cv::KMEANS_RANDOM_CENTERS, centers);
	centers = centers.reshape(3, 0);
	label = label.reshape(1, img.rows);
	for (int i = 0; i < centers.rows; i++)
	{
		cv::Scalar color = centers.at<cv::Vec3f>(i);
		cv::Mat mask(label == i);
		result.setTo(color, mask); // set cluster color
	}
	return result;
}

static std::vector<cv::Mat> splitImage(const cv::Mat &image, int M, int N)
{
	// All images should be the same size ...
	int width = image.cols / M;
	int height = image.rows / N;
	// ... except for the Mth column and the Nth row
	int width_last_column = width + (image.cols % width);
	int height_last_row = height + (image.rows % height);

	std::vector<cv::Mat> result;

	for (int i = 0; i < N; ++i)
	{
		for (int j = 0; j < M; ++j)
		{
			// Compute the region to crop from
			cv::Rect roi(width * j,
				     height * i,
				     (j == (M - 1)) ? width_last_column : width,
				     (i == (N - 1)) ? height_last_row : height);
			result.push_back(image(roi));
		}
	}
	return result;
}

static cv::Mat combineImage(const std::vector<cv::Mat> &matrix, int N)
{
	int M = matrix.size() / N;
	int cols = matrix[0].cols * M;
	int rows = matrix[0].rows * N;
	// ... except for the Mth column and the Nth row
	int width_last_column = matrix[0].cols + (cols % matrix[0].cols);
	int height_last_row = matrix[0].rows + (rows % matrix[0].rows);

	cv::Mat result(cv::Size(cols, rows), matrix[0].type());

	for (int i = 0; i < N; ++i)
	{
		for (int j = 0; j < M; ++j)
		{
			cv::Rect roi(matrix[0].cols * j,
				     matrix[0].rows * i,
				     (j == (M - 1)) ? width_last_column : matrix[0].cols,
				     (i == (N - 1)) ? height_last_row : matrix[0].rows);
			matrix[(i * M) + j].copyTo(result(roi));
		}
	}
	return result;
}