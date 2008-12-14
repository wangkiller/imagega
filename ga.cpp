#include <cstdio>
#include <vector>
#include <QtGui>

#define IMGSIZE 256
#define NGON 4
#define POLYGONS 50
#define POPULATION 12
#define SIZEOFPOLY (4+2*NGON)

void calcDistance(long long* distances);
QImage drawImage(double** member);
void mutate(double** member);
std::vector<double**> population;
double** breed(double** parent1, double** parent2);
long long distance(QImage img);
void initga();
void gastep();

QImage sourceImg;

void mutate(double** member)
{
	int poly = rand()%POLYGONS;
	int field           = rand()%(4+2*NGON);
	if (rand()%2) {
		member[poly][field] = (double)rand()/RAND_MAX;
	} else {
		member[poly][field] += (double)rand()/RAND_MAX*0.2 - 0.1;
		if (member[poly][field] < 0.0)
			member[poly][field] = 0.0;
		if (member[poly][field] > 1.0)
			member[poly][field] = 1.0;
	}
}

double** breed(double** parent1, double** parent2)
{
	double** child = new double*[POLYGONS];
	#pragma omp parallel for
	for (int p = 0; p < POLYGONS; p++) {
		child[p] = new double[SIZEOFPOLY];
		int r = rand()&1;
		if (r) {
			memcpy(child[p],parent1[p],SIZEOFPOLY*sizeof(double));
		} else {
			memcpy(child[p],parent2[p],SIZEOFPOLY*sizeof(double));
		}
	}
	return child;
}

//data format, [r,g,b,a,x0,y0,x1,y1,xn,yn]
//all go from 0.0 to 1.0 to ease computation

//draw a member and return it
//TODO: make calcDistance call this
QImage drawImage(double** member)
{
		QImage drawImage(IMGSIZE,IMGSIZE,QImage::Format_RGB32);
		QPainter painter(&drawImage);
		painter.setPen(Qt::NoPen);
		for (int n = 0; n < POLYGONS; n++) {
			double* polygon = member[n];
			painter.setBrush(QBrush(QColor(polygon[0]*255,polygon[1]*255,polygon[2]*255,polygon[3]*255)));
			int points[NGON*2];
			for (int i = 0; i < NGON*2; i++)
				points[i] = polygon[4+i]*IMGSIZE;
			QPolygon poly;
			poly.setPoints(NGON,points);
			painter.drawPolygon(poly);
		}
		painter.end();
		return drawImage;
}

//calculate the distance from the source image
long long distance(QImage img)
{
		//distance is distance in 3-space of the colors
		//TODO:maybe use Delta-E? http://www.colorwiki.com/wiki/Delta_E:_The_Color_Difference
		int numbytes = img.numBytes();
		long long dist = 0;
		uchar* simg = sourceImg.bits();
		uchar* dimg = img.bits();
		for (int n = 0; n < numbytes; n++) {
			int diff = simg[n]-dimg[n];
			dist += diff*diff;
		}
		return dist;
}

//calculate the distance from the source image for each member of the
//population storing it in an array (of population size) passed in
void calcDistance(long long* distances)
{
	#pragma omp parallel for
	for (int p = 0; p < (int)population.size(); p++) {
		//draw polygons on image
		QImage drawImg = drawImage(population[p]);
		distances[p] = distance(drawImg);
	}
}

void initga()
{
	for (int p = 0; p < POPULATION; p++) {
		population.push_back(new double*[POLYGONS]); //TODO: presize vector?
		for (int i = 0; i < POLYGONS; i++) {
			population[p][i] = new double[4+2*NGON];
			for (int j = 0; j < 4+2*NGON; j++)
				population[p][i][j] = 0.0;//(double)rand()/RAND_MAX;
		}
	}
}

void gastep() {
	long long* distances = new long long[population.size()];
	calcDistance(distances);
	//FIXME: use faster sort than selection
	{
		for (int n = 1; n < population.size(); n++) {
			int min_i = n-1;
			for (int m = n; m < population.size(); m++)
				if (distances[m] < distances[min_i])
					min_i = m;
			if (n-1 != min_i) {
				double** tmp_p = population[n-1];
				long long tmp_d = distances[n-1];
				population[n-1] = population[min_i];
				distances[n-1] = distances[min_i];
				population[min_i] = tmp_p;
				distances[min_i] = tmp_d;
			}	
		}
	}
#define TOKILL 6
	for (int i = 0; i < TOKILL; i++) {
		for (int p = 0; p < POLYGONS; p++)
			delete population[population.size()-1][p];
		delete population[population.size()-1];
		population.pop_back();
	}
	//breed new ones
	//FIXME: use better breeding strategy than this one
	for (int n = 1; n <= TOKILL; n++) {
		population.push_back(breed(population[0],population[n]));
	}
	//mutate them all
	#pragma omp parallel for
	for (int n = 0; n < population.size(); n++)
		mutate(population[n]);
}

int main(int argc, char* argv[])
{
	if (argc > 1)
		sourceImg = QImage(argv[1]).scaled(IMGSIZE,IMGSIZE);
	else {
		printf("You must specify the image to generate as second option\n");
		exit(1);
	}
	initga();
	int gen = 0;
	while (true) {
		printf("generation %d\n",gen);
		gastep();
		if (!(gen%100)) {
			drawImage(population[0]).save(QString("out") + QString::number(gen) + QString(".png"));
		}
		gen++;
	}
}
