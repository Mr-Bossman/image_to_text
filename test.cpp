#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <atomic>
#include <opencv2/opencv.hpp>
#include <opencv2/freetype.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <unistd.h>

static std::vector<cv::Mat> splitImage(const cv::Mat &image, int M, int N);
static void Pcolor(char c, cv::Vec3f fgcolor, cv::Vec3f bgcolor);
static cv::Mat text(int fontHeight,const std::string &text, const cv::Ptr<cv::freetype::FreeType2> &ft2, cv::Scalar bg, cv::Scalar fg);
static char textImage(const cv::Mat& copy);
static cv::Mat img_pallet(cv::Mat &img, int pallet, cv::Mat &centers);
static std::vector<cv::Mat> splitImage(const cv::Mat &image, int M, int N);
static cv::Mat combineImage(const std::vector<cv::Mat> &matrix, int N);
static void display(const cv::Mat &img, int height, int width);
static cv::Ptr<cv::freetype::FreeType2> ft2;
int main(int argc, char **argv)
{
	int height = strtol(argv[1], NULL, 10);
	int width = strtol(argv[2], NULL, 10);
	cv::VideoCapture cap("test.mp4"); // video
	ft2 = cv::freetype::createFreeType2();
	ft2->loadFontData("./test.ttf", 0);
	cv::Mat img;
	while (1)
	{
		if (!cap.read(img))
		{
			break;
		}
		display(img, height, width);
	}
	cv::waitKey(0);
	cv::destroyAllWindows();

	return 0;
}
static void display(const cv::Mat& img, int height, int width)
{
	printf("\033[2J\033[0H");
	auto matrix = splitImage(img, width, height);
	std::vector<cv::Vec3f> bg(matrix.size());
	std::vector<cv::Vec3f> fg(matrix.size());
	std::string cha;
	cha.resize(matrix.size(),' ');
	//#pragma omp parallel
	//#pragma omp for
	for (size_t i = 0; i < matrix.size(); i++)
	{
		cv::Mat clusters,tmp;
		tmp = img_pallet(matrix[i], 2, clusters);
		fg[i] = clusters.at<cv::Vec3f>(1);
		bg[i] = clusters.at<cv::Vec3f>(0);
		cha[i] = textImage(tmp);
	}

	for (size_t i = 0; i < fg.size(); i++)
	{
		if (i % width == 0)
			puts("");
		Pcolor(cha[i],fg[i], bg[i]);

	}
	printf("\033[0m\n");
	//cv::imshow("img", combineImage(matrix,height));
}
static void Pcolor(char c, cv::Vec3f fgcolor, cv::Vec3f bgcolor)
{
	printf("\033[38;2;%u;%u;%um", (uint)fgcolor[2], (uint)fgcolor[1], (uint)fgcolor[0]);
	printf("\033[48;2;%u;%u;%um", (uint)bgcolor[2], (uint)bgcolor[1], (uint)bgcolor[0]);
	printf("%c", c);
}

static cv::Mat text(int fontHeight, const std::string &text, const cv::Ptr<cv::freetype::FreeType2> &ft2, cv::Scalar bg, cv::Scalar fg)
{
	int thickness = -1;
	int linestyle = 8;
	int baseline = 0;
	const cv::Size textSize = ft2->getTextSize(text, fontHeight, thickness, &baseline);
	if (thickness > 0)
	{
		baseline += thickness;
	}
	int height = textSize.height * 1.25;
	int width = textSize.width * 1.25;
	cv::Mat img(height,width, CV_8UC3, cv::Scalar::all(bg[0])),out(height,width, CV_8UC1, bg);
	cv::Point textOrg(0,height);
	ft2->putText(img, text, textOrg, fontHeight,
		     cv::Scalar::all(fg[0]), thickness, linestyle, true);
	cv::cvtColor(img,out,cv::COLOR_BGR2GRAY);

	return out;
}
static char textImage(const cv::Mat &copy)
{
	cv::Mat tmp;

	const std::string str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()";
	std::pair<char,double> best(' ',1.0);
	std::string b;
	//#pragma omp parallel
	//#pragma omp for
	for (auto c : str)
	{
		b = c;
		cv::resize(text(copy.size().height,b,ft2,cv::Scalar(255),cv::Scalar(0)),tmp,copy.size(),cv::INTER_CUBIC);
		cv::bitwise_xor(tmp,copy,tmp);
		const double perc = (double)cv::countNonZero(tmp) / (double)tmp.size().area();
		if(best.second > perc){
			best.first = c;
			best.second = perc;
		}
	}
	return best.first;
}
static cv::Mat img_pallet(cv::Mat& img, int pallet, cv::Mat &centers)
{
	img.convertTo(img, CV_32F);
	cv::Mat samples, label, result(img.size(), CV_8UC1);
	samples = img.reshape(1, img.total());
	const double ret = cv::kmeans(samples, pallet, label, cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 10, 0.1),
				10, cv::KMEANS_RANDOM_CENTERS, centers);
	centers = centers.reshape(3, 0);
	label = label.reshape(1, img.rows);
	/*for (int i = 0; i < centers.rows; i++)
	{
		cv::Scalar color = centers.at<cv::Vec3f>(i);
		cv::Mat mask(label == i);
		result.setTo(color, mask);
	}*/
	cv::Mat maska(label == 1);
	result.setTo(cv::Scalar(255), maska);
	cv::Mat maskb(label == 0);
	result.setTo(cv::Scalar(0), maskb);
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