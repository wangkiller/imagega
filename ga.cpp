#include <cstdio>
#include <vector>
#include <QtGui>

#define IMGSIZE 256
#define NGON 4
#define POLYGONS 50
#define POPULATION 10

uint distance(QImage &img1, QImage &img2);
void calcDistance(uint* distances);
QImage drawImage(double** member);
QImage sourceImg;
void mutate(double** member);
std::vector<double**> population;

void mutate(double** member)
{
	//FIXME: out of bounds maybe?
	int poly = rand()%POLYGONS;
	int field           = rand()%(4+2*NGON);
	member[poly][field] = (double)rand()/RAND_MAX;
}

uint distance(QImage &img1, QImage &img2)
{
	if (img1.numBytes() != img2.numBytes())
		printf("Image Size Mismatch!\n");
	int dist = 0;
	const uchar* bytes1 = img1.bits();
	const uchar* bytes2 = img2.bits();
	for (int i = 0; i < img1.numBytes(); i++) {
		uint dbyte = bytes1[i]-bytes2[i];
		dist      += dbyte * dbyte;
	}
	return dist;
}
//data format, [r,g,b,a,x0,y0,x1,y1,xn,yn]
//all go from 0.0 to 1.0 to ease computation

//draw a member and return it
//TODO: make calcDistance call this
QImage drawImage(double** member)
{
		QImage drawImage(IMGSIZE,IMGSIZE,QImage::Format_ARGB32);
		QPainter painter(&drawImage);
		painter.setPen(Qt::NoPen);
		for (int n = 0; n < POLYGONS; n++) {
			double* polygon = member[n];
			painter.setBrush(QBrush(QColor(polygon[0]*255,polygon[1]*255,polygon[2]*255,polygon[3]*255)));
			int points[NGON*2];
			for (int i = 0; i < NGON*2; i++)
				points[i] = polygon[4+i]*(IMGSIZE-1);
			QPolygon poly;
			poly.setPoints(NGON,points);
			painter.drawPolygon(poly);
		}
		painter.end();
		return drawImage;
}

//calculate the distance from the source image for each member of the
//population storing it in an array (of population size) passed in
void calcDistance(int* distances)
{
	for (int p = 0; p < POPULATION; p++) {
		//draw polygons on image
		QImage drawImg = drawImage(population[p]);
		//distance is distance in 3-space of the colors
		//TODO:maybe use Delta-E? http://www.colorwiki.com/wiki/Delta_E:_The_Color_Difference
		int numbytes = drawImg.numBytes();
		int dist = 0;
		uchar* simg = sourceImg.bits();
		uchar* dimg = drawImg.bits();
		for (int n = 0; n < numbytes; n++) {
			if (n % 4 == 0) //TODO: make sure this works to skip alpha channel
				continue;
			int diff = simg[n]-dimg[n];
			dist += diff*diff;
		}
		distances[p] = dist;
		drawImg.save(QString("out")+QString(p+'0')+QString(".png"));
		//make outputting work for images greater than 10
	}
}

int main(int argc, char* argv[])
{
	if (argc > 1)
		sourceImg = QImage(argv[1]).scaled(IMGSIZE,IMGSIZE);
	else {
		printf("You must specify the image to generate as second option\n");
		exit(1);
	}
	for (int p = 0; p < POPULATION; p++) {
		population.push_back(new double*[POLYGONS]); //TODO: presize vector?
		for (int i = 0; i < POLYGONS; i++) {
			population[p][i] = new double[4+2*NGON];
			for (int j = 0; j < 4+2*NGON; j++)
				population[p][i][j] = (double)rand()/RAND_MAX;
//			population[p][i][3] = 1.0; //FIXME: start opaque for testing
		}
	}
	int* distances = new int[POPULATION];
	int gen = 0;
	while (true) {
		printf("generation %d\n",gen);
		mutate(population[0]);
		calcDistance(distances);
		gen++;
	}
}
