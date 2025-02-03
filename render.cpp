#include <iostream>
#include <math.h>
#include <string>
#include <sstream>
#include <cmath>
#include <atomic>
#include <thread>
#include <algorithm>
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
static void Pcolor_256(char c, cv::Vec3f fgcolor, cv::Vec3f bgcolor);
static unsigned int calc_256_color(cv::Vec3f color);
static cv::Mat text(int fontHeight,const std::string &text, const cv::Ptr<cv::freetype::FreeType2> &ft2, cv::Scalar bg, cv::Scalar fg);
static char textImage(const cv::Mat& copy, bool &swap_pallet);
static cv::Mat img_pallet(cv::Mat &img, int pallet, cv::Mat &centers);
static std::vector<cv::Mat> splitImage(const cv::Mat &image, int M, int N);
static cv::Mat combineImage(const std::vector<cv::Mat> &matrix, int N);
static void display(const cv::Mat &img, int height, int width);
static std::vector<std::pair<cv::Mat,char>>fillchars(int width, int height);

static cv::Ptr<cv::freetype::FreeType2> ft2;
static std::atomic_flag canRead = ATOMIC_FLAG_INIT;
static std::vector<std::pair<cv::Mat,char>> chars;

int main(int argc, char **argv)
{
	if (argc != 6)
	{
		std::cout << "./" << argv[0] << " {video/image} {width of terminal (chars)} {height of terminal (chars)} {font width (px)} {font height (px)}" << std::endl;
		exit(-1);
	}
	int width = strtol(argv[2], NULL, 10);
	int height = strtol(argv[3], NULL, 10);
	int Twidth = strtol(argv[4], NULL, 10);
	int Theight = strtol(argv[5], NULL, 10);
	cv::VideoCapture cap(argv[1]);
	ft2 = cv::freetype::createFreeType2();
	ft2->loadFontData("./test.ttf", 0);
	chars = fillchars(Twidth,Theight);
	cv::Mat img;

	while (1)
	{
		if (!cap.read(img))
			break;
		cv::resize(img,img,cv::Size(width*Twidth,height*Theight),cv::INTER_CUBIC);
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
	#pragma omp parallel
	#pragma omp for
	for (size_t i = 0; i < matrix.size(); i++)
	{
		bool swap_pallet = 0;
		cv::Mat clusters,tmp;
		tmp = img_pallet(matrix[i], 2, clusters);
		cha[i] = textImage(tmp, swap_pallet);
		fg[i] = clusters.at<cv::Vec3f>(swap_pallet?0:1);
		bg[i] = clusters.at<cv::Vec3f>(swap_pallet?1:0);
	}
	for (size_t i = 0; i < fg.size(); i++) {
		if (i && i % width == 0)
			puts("");
		Pcolor_256(cha[i],fg[i], bg[i]);
	}
	printf("\033[0m\n");
}

static void Pcolor(char c, cv::Vec3f fgcolor, cv::Vec3f bgcolor)
{
	printf("\033[38;2;%u;%u;%um", (uint)fgcolor[2], (uint)fgcolor[1], (uint)fgcolor[0]);
	printf("\033[48;2;%u;%u;%um", (uint)bgcolor[2], (uint)bgcolor[1], (uint)bgcolor[0]);
	printf("%c", c);
}

static unsigned int calc_256_color(cv::Vec3f color)
{
	auto r = std::clamp(color[0], 0.0f, 255.0f);
	auto g = std::clamp(color[1], 0.0f, 255.0f);
	auto b = std::clamp(color[2], 0.0f, 255.0f);
	unsigned int ri = round(r * 5.0 / 255.0);
	unsigned int gi = round(g * 5.0 / 255.0);
	unsigned int bi = round(b * 5.0 / 255.0);
	return 16 + (36 * ri) + (6 * gi) + bi;
}

static void Pcolor_256(char c, cv::Vec3f fgcolor, cv::Vec3f bgcolor)
{
	printf("\033[38;5;%um\033[48;5;%um%c", calc_256_color(fgcolor), calc_256_color(bgcolor), c);
}

static cv::Mat text(int fontHeight, const std::string &txt, const cv::Ptr<cv::freetype::FreeType2> &ft2, cv::Scalar bg, cv::Scalar fg)
{
	int thickness = -1;
	int linestyle = 8;
	int baseline = 0;
	const cv::Size textSize = ft2->getTextSize(txt, fontHeight, thickness, &baseline);
	if (thickness > 0)
		baseline += thickness;
	int height = abs(textSize.height * 1.25);
	int width = textSize.width * 1.25;
	cv::Mat img(height,width, CV_8UC3, cv::Scalar::all(bg[0])),out(height,width, CV_8UC1, bg);
	cv::Point textOrg(0,height);
	ft2->putText(img, txt, textOrg, fontHeight,
		     cv::Scalar::all(fg[0]), thickness, linestyle, true);
	cv::cvtColor(img,out,cv::COLOR_BGR2GRAY);

	return out;
}

static char textImage(const cv::Mat &copy, bool &swap_pallet)
{
	std::pair<char,double> best(' ',1.0);
	std::atomic_flag update = ATOMIC_FLAG_INIT;
	#pragma omp parallel
	#pragma omp for
	for (auto c : chars)
	{
		cv::Mat tmp;
		cv::bitwise_xor(c.first,copy,tmp);

		const double perc1 = (double)cv::countNonZero(tmp) / ((double)tmp.size().area() * (double)(tmp.size().area() - cv::countNonZero(c.first)));
		const double perc2 = (double)(tmp.size().area() - cv::countNonZero(tmp)) / ((double)tmp.size().area() * (double)(tmp.size().area() - cv::countNonZero(c.first)));
		swap_pallet = perc1 < perc2;
		const double perc = std::max(perc1,perc2);
		while(update.test_and_set()){std::this_thread::yield();}
		if(best.second > perc){
			best.first = c.second;
			best.second = perc;
		}
		update.clear();
	}
	return best.first;
}

static std::vector<std::pair<cv::Mat,char>> fillchars(int width, int height){
	const std::string str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()[]{}\\|:;>.<,?`~/-_=+";
	std::vector<std::pair<cv::Mat,char>> ret;
	for (auto c : str)
	{
		std::string b;
		b = c;
		cv::Mat tmp,cmp = text(height,b,ft2,cv::Scalar(255),cv::Scalar(0));
		cv::resize(cmp,tmp,cv::Size(width,height),cv::INTER_CUBIC);
		ret.push_back(std::pair<cv::Mat,char>(tmp,c));
	}
	return ret;
}

static cv::Mat img_pallet(cv::Mat& img, int pallet, cv::Mat &centers)
{
	img.convertTo(img, CV_32F);
	cv::Mat samples, label, result(img.size(), CV_8UC1);
	samples = img.reshape(1, img.total());
	const double ret = cv::kmeans(samples, pallet, label, cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 4, 1),
				4, cv::KMEANS_RANDOM_CENTERS, centers);
	centers = centers.reshape(3, 0);
	label = label.reshape(1, img.rows);
	/*for (int i = 0; i < centers.rows; i++)
	{
		cv::Scalar color = centers.at<cv::Vec3f>(i);
		cv::Mat mask(label == i);
		result.setTo(color, mask);
	}*/
	// i dont think they are sorted
	cv::Mat maska(label == 1);
	result.setTo(cv::Scalar(255), maska);
	cv::Mat maskb(label == 0);
	result.setTo(cv::Scalar(0), maskb);
	return result;
}

static std::vector<cv::Mat> splitImage(const cv::Mat &image, int M, int N)
{
	int width = image.cols / M;
	int height = image.rows / N;
	int width_last_column = width + (image.cols % width);
	int height_last_row = height + (image.rows % height);

	std::vector<cv::Mat> result;

	for (int i = 0; i < N; ++i)
	{
		for (int j = 0; j < M; ++j)
		{
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
