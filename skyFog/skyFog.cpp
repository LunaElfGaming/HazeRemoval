#include <opencv2/opencv.hpp>  
#include <iostream>  
#include <string.h>
#include<vector>
#include "guidedfilter.h"

using namespace cv;
using namespace std;

float M[3][3] = { {0.0255,-0.1275,1.0965},{-0.3315,1.5045,-0.1785},{0.5610,0.3825,0.0510} };
float Minv[3][3] = { {-0.1381,-0.4055,1.5492},{0.07925,0.5845,0.3418},{0.9244,0.07740,0.003715} };

int mindata(int bgr[3]) {
	int min = bgr[0];
	for (int i = 1;i <= 2;i++) {
		if (min > bgr[i])
			min = bgr[i];
	}
	return min;
}

int abs(int a) {
	if (a < 0)
		return -a;
	return a;
}

float ffabs(float a) {
	if (a < 0)
		return -a;
	return a;
}

typedef struct Pixel
{
	int x;
	int y;
	int bgr[3];
	int dark;
}Pixel;

bool cmp(const Pixel &a, const Pixel &b) {
	return a.dark > b.dark;
}

Mat BGRtoT(Mat src) {
	Mat srcT = src.clone();
	int RGB[3];
	int T[3];
	for (int i = 0;i < src.rows;i++) {
		for (int j = 0;j < src.cols;j++) {
			for (int k = 0;k < 3;k++) {
				RGB[k] = src.at<Vec3b>(i, j)[2-k];
			}
			for (int k = 0;k < 3;k++) {
				float a=0.0;
				for (int l = 0;l < 3;l++) {
					a += (float)RGB[l] * M[k][l];
				}
				T[k] = ffabs(a);
			}
			for (int k = 0;k < 3;k++) {
				srcT.at<Vec3b>(i,j)[k] = T[k];
			}
		}
	}
	return srcT;
}

Mat MinFilter(Mat src, int scale)

{
	int radius = (scale - 1) / 2;
	Mat border;
	//����Ҫ����Сֵ����������ı߽�����ø��Ʊ߽����  
	copyMakeBorder(src, border, radius, radius, radius, radius, BORDER_REPLICATE);

	//��Сֵ�˲�  
	Mat dark = src.clone();
	for (int i = 0; i < src.cols; i++)
	{
		for (int j = 0; j < src.rows; j++)
		{
			//ѡȡ��Ȥ����  
			Mat roi;
			roi = border(Rect(i, j, scale, scale));

			//����Ȥ�������Сֵ  
			double minVal = 0; double maxVal = 0;
			Point minLoc = 0; Point maxLoc = 0;
			minMaxLoc(roi, &minVal, &maxVal, &minLoc, &maxLoc, noArray());

			dark.at<uchar>(Point(i, j)) = (uchar)minVal;
		}
	}
	return dark;
}

void HazeRemove(char* name) {
	Mat src = imread(name);
	Mat srcT;
	src = BGRtoT(src);
	//��ͨ������
	Mat dim(src.rows, src.cols, CV_8UC1);  //��С��ͨ��ͼ
	int r = src.rows, c = src.cols;
	int bgr[3];
	vector<Pixel> pic, darkpic;                   //ͼ���������أ��밵ͨ�����0.1%����

	for (int i = 0;i < r;i++) {
		for (int j = 0;j < c;j++) {
			int bgr[3];
			bgr[0] = src.at<Vec3b>(i, j)[0];
			bgr[1] = src.at<Vec3b>(i, j)[1];
			bgr[2] = src.at<Vec3b>(i, j)[2];
			dim.at<uchar>(i, j) = mindata(bgr);
		}
	}
	dim = MinFilter(dim,7);

	for (int i = 0;i < r;i++) {
		for (int j = 0;j < c;j++) {
			Pixel pix;
			pix.x = i;
			pix.y = j;
			pix.bgr[0] = src.at<Vec3b>(i, j)[0];
			pix.bgr[1] = src.at<Vec3b>(i, j)[1];
			pix.bgr[2] = src.at<Vec3b>(i, j)[2];
			pix.dark = dim.at<uchar>(i, j);
			pic.push_back(pix);                        //�������ء���ͨ��ֵ��RGBֵ�б�
		}
	}

	//���ƴ�����ֵ
	int darknum = pic.size() / 1000;
	sort(pic.begin(), pic.end(), cmp);
	int max = 0;
	Pixel max_pix;           //�ҵ���ͨ����С�����е����������Ϊ������ֵ
	for (int i = 0;i < darknum;i++) {
		int lumin = pic.at(i).bgr[2] * 0.30 + pic.at(i).bgr[1] * 0.59 + pic.at(i).bgr[0] * 0.11;  //��������
		if (max < lumin) {
			max_pix.x = pic.at(i).x;
			max_pix.y = pic.at(i).y;
			max = lumin;
		}
	}
	//max_pix.dark = max;
	//max_pix.bgr[0] = src.at<Vec3b>(max_pix.x, max_pix.y)[0];
	//max_pix.bgr[1] = src.at<Vec3b>(max_pix.x, max_pix.y)[1];
	//max_pix.bgr[2] = src.at<Vec3b>(max_pix.x, max_pix.y)[2];
	//float Alumin = max;

	//�������͸����
	Mat trans(r, c, CV_32FC1);
	for (int i = 0;i < r;i++) {
		for (int j = 0;j < c;j++) {
			float min = 10000.0;
			for (int k = 0;k < 2;k++)
			{
				float kk = src.at<Vec3b>(i, j)[k] / ((float)max_pix.bgr[k]);
				if (kk < min)
					min = kk;
			}
			trans.at<float>(i, j) =ffabs(1-0.95*min);
		}
	}

	//����ԭͼ
	Mat J = src.clone();
	for (int i = 0;i < r;i++) {
		for (int j = 0;j < c;j++) {
			float t = trans.at<float>(i, j);
			for (int k = 0;k < 3;k++) {
				J.at<Vec3b>(i, j)[k] = abs(max_pix.bgr[k] - (max_pix.bgr[k] - src.at<Vec3b>(i, j)[k]) / t);
			}
		}
	}
	//cout << r << " " << c << " " << max_pix.x << " " << max_pix.y << endl;
	circle(src, Point(max_pix.y, max_pix.x), 15, Scalar(0, 0, 255),5);
	namedWindow("origin");
	imshow("origin", src);
	imwrite("output.bmp", J);
	imwrite("output_dim.bmp", dim);
	imwrite("output_origin.bmp", src);
	namedWindow("remove");
	imshow("remove", J);
	waitKey(0);
}

int main()
{
	//��ȡһ��ͼƬ  
	char* name;
	name = "forest-input.bmp";
	HazeRemove(name);
	return 0;
}