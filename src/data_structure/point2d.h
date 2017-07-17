#pragma once

#include<ostream>

class Point2D
{
	double x = 0.;
	double y = 0.;
public:
	Point2D(double x, double y) : x(x), y(y) {}
	Point2D() = default;
	Point2D(const Point2D& other) = default;


	double getX() const { return x; }
	double getY() const { return y; }

	void setX(double v) { x = v; }
	void setY(double v) { y = v; }

	double euklidDist(const Point2D& p) const;

	void print(std::ostream& stream) const { stream << '(' << x << "; " << y << ')'; }
};


inline std::ostream& operator<<(std::ostream& stream, const Point2D& p) { p.print(stream); return stream; }


