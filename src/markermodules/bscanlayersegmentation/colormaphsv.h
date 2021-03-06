/*
 * Copyright (c) 2018 Kay Gawlik <kaydev@amarunet.de> <kay.gawlik@beuth-hochschule.de> <kay.gawlik@charite.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include<cstdint>

#include<helper/convertcolorspace.h>


class Colormap
{
protected:
	double minValue = 100;
	double maxValue = 500;
public:
	virtual ~Colormap() = default;

	virtual void getColor(double value, uint8_t& r, uint8_t& g, uint8_t& b) const = 0;

	virtual void   setMaxValue(double value)                        { maxValue = value; }
	virtual double getMaxValue() const                              { return maxValue; }

	virtual void   setMinValue(double value)                        { minValue = value; }
	virtual double getMinValue() const                              { return minValue; }
};

class ColormapHSV : public Colormap
{
	double fadeIn  = 0.15;
	double fadeOut = 0.35;

	const double maxHue = 300;
	const double minHue = 0;
	const double fadeInHue = 0;
	const double fadeOutHue = 260;
public:

	void getColor(double value, uint8_t& r, uint8_t& g, uint8_t& b) const override
	{
		/*if(value < minValue)
		{
			r = g = b = 0;
		}
		else */if(value > maxValue)
		{
			r = g = b = 255;
		}
		else
		{
			double relValue   = 1.-(value-minValue)/(maxValue-minValue)*(1+fadeIn);
			double saturation = 1; // relValue>0.8?(0.8-relValue)*5:1;
			double hsvValue   = 1; // relValue<0.2?(relValue*5):1;
			double hue        = relValue*360;

			if(hue < fadeInHue)
			{
				saturation = 1+(hue-fadeInHue)/360./fadeIn;
				if(saturation < 0) saturation = 0;
			}

			if(hue > fadeOutHue)
			{
				hsvValue = 1-(hue-fadeOutHue)/360./fadeOut;
				if(hsvValue < 0) hsvValue = 0;
			}

			if(hue > maxHue)
				hue = maxHue;
			else if(hue < minHue)
				hue = 0;


			double rd, gd, bd;
// 			std::tie(rd, gd, bd) = hsv2rgb(hue, saturation, hsvValue);
			hsv2rgb(hue, saturation, hsvValue, rd, gd, bd);

			r = static_cast<uint8_t>(rd*255);
			g = static_cast<uint8_t>(gd*255);
			b = static_cast<uint8_t>(bd*255);
		}
	}

};


class ColormapYellow : public Colormap
{
public:
	void getColor(double value, uint8_t& r, uint8_t& g, uint8_t& b) const override
	{
		if(value < minValue)
		{
			r = g = b = 0;
			return;
		}
		if(value > maxValue)
		{
			r = g = b = 255;
			return;
		}

		double relValue   = 1.-(value-minValue)/(maxValue-minValue);
		double saturation = 1; // relValue>0.8?(0.8-relValue)*5:1;
		double hsvValue   = 1; // relValue<0.2?(relValue*5):1;
		double hue        = (1-relValue)*60;

		if(relValue < 0.5)
		{
			saturation = relValue*2;
			if(saturation < 0)
				saturation = 0;
		}
		else
		{
			hsvValue = 2-relValue*2;
			if(hsvValue < 0)
				hsvValue = 0;
		}


		double rd, gd, bd;
// 		std::tie(rd, gd, bd) = hsv2rgb(hue, saturation, hsvValue);
		hsv2rgb(hue, saturation, hsvValue, rd, gd, bd);

		r = static_cast<uint8_t>(rd*255);
		g = static_cast<uint8_t>(gd*255);
		b = static_cast<uint8_t>(bd*255);
	}

};
