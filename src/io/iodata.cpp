#include "iodata.h"


iodata::iodata(int interpolorder, int geointerpolorder, bool isitscalardata, std::vector<double> timevals)
{
	if (interpolorder <= 0 || geointerpolorder <= 0)
	{
        std::cout << "Error in 'iodata' object: cannot have a negative or zero interpolation order" << std::endl;
        abort();
	}

	myinterpolorder = interpolorder;
	mygeointerpolorder = geointerpolorder;
	isscalardata = isitscalardata;
	mytimevals = timevals;

 	mycoords = std::vector<std::vector<std::vector<densematrix>>>(3, std::vector<std::vector<densematrix>>(8, std::vector<densematrix>(0)));
 	if (isscalardata)
		mydata = std::vector<std::vector<std::vector<densematrix>>>(1, std::vector<std::vector<densematrix>>(8, std::vector<densematrix>(0)));
	else
		mydata = std::vector<std::vector<std::vector<densematrix>>>(3, std::vector<std::vector<densematrix>>(8, std::vector<densematrix>(0)));
}

void iodata::combine(void)
{
	// Loop on every element type:
	for (int i = 0; i < 8; i++)
	{
		// Skip if there is a single block:
		if (mycoords[0][i].size() <= 1)
			continue;
	
		for (int s = 0; s < 3; s++)
			mycoords[s][i] = {densematrix(mycoords[s][i])};
		for (int comp = 0; comp < mydata.size(); comp++)
			mydata[comp][i] = {densematrix(mydata[comp][i])};
	}
}

bool iodata::isscalar(void) { return isscalardata; }
int iodata::getinterpolorder(void) { return myinterpolorder; };
int iodata::getgeointerpolorder(void) { return mygeointerpolorder; };

std::vector<double> iodata::gettimetags(void) { return mytimevals; };

bool iodata::ispopulated(int elemtypenum)
{
	return (mycoords[0][elemtypenum].size() > 0);
}

std::vector<int> iodata::getactiveelementtypes(void)
{
	std::vector<int> activeelementtypes = {};
	for (int i = 0; i < 8; i++)
	{
		if (ispopulated(i) == true)
			activeelementtypes.push_back(i);
	}
	return activeelementtypes;
}

void iodata::addcoordinates(int elemtypenum, densematrix xcoords, densematrix ycoords, densematrix zcoords)
{
	mycoords[0][elemtypenum].push_back(xcoords);
	mycoords[1][elemtypenum].push_back(ycoords);
	mycoords[2][elemtypenum].push_back(zcoords);
}

void iodata::adddata(int elemtypenum, std::vector<densematrix> vals)
{
	// The non-provided components are set to 0:
	int vallen = vals.size();
	if (isscalardata == false && vallen < 3)
	{
		for (int i = vallen; i < 3; i++)
			vals.push_back(densematrix(vals[0].countrows(), vals[0].countcolumns(), 0.0));
	}

	for (int comp = 0; comp < vals.size(); comp++)
		mydata[comp][elemtypenum].push_back(vals[comp]);
}

std::vector<densematrix> iodata::getcoordinates(int elemtypenum)
{
	combine();
	return {mycoords[0][elemtypenum][0], mycoords[1][elemtypenum][0], mycoords[2][elemtypenum][0]};
}

std::vector<densematrix> iodata::getdata(int elemtypenum)
{
	combine();
	if (isscalardata == true)
		return {mydata[0][elemtypenum][0]};
	else
		return {mydata[0][elemtypenum][0], mydata[1][elemtypenum][0], mydata[2][elemtypenum][0]};
}
