//  Copyright (c) 2011, Auerhaus Development, LLC
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What The Fuck You Want
//  To Public License, Version 2, as published by Sam Hocevar. See
//  http://sam.zoy.org/wtfpl/COPYING for more details.
//
//
// Original source: https://github.com/warrenm/AHEasing
//
// See http://easings.net/ for visualizations
//

#include "easing.h"
#include <math.h>
#include <tnlAssert.h>

#define M_PI FloatPi
#define M_PI_2 FloatHalfPi

#pragma warning(disable: 4244) // Suppress warning about possible loss of data when using floats like 2.5

AHFloat getEasedValue(EasingType easingType, F32 value)
{
   switch(easingType)
   {
      case LINEAR_INTERPOLATION:
         return Easing::LinearInterpolation(value);
      case QUADRATIC_EASE_IN:
         return Easing::QuadraticEaseIn(value);
      case QUADRATIC_EASE_OUT:
         return Easing::QuadraticEaseOut(value);
      case QUADRATIC_EASE_IN_OUT:
         return Easing::QuadraticEaseInOut(value);
      case CUBIC_EASE_IN:
         return Easing::CubicEaseIn(value);
      case CUBIC_EASE_OUT:
         return Easing::CubicEaseOut(value);
      case CUBIC_EASE_IN_OUT:
         return Easing::CubicEaseInOut(value);
      case QUARTIC_EASE_IN:
         return Easing::QuarticEaseIn(value);
      case QUARTIC_EASE_OUT:
         return Easing::QuarticEaseOut(value);
      case QUARTIC_EASE_IN_OUT:
         return Easing::QuarticEaseInOut(value);
      case QUINTIC_EASE_IN:
         return Easing::QuinticEaseIn(value);
      case QUINTIC_EASE_OUT:
         return Easing::QuinticEaseOut(value);
      case QUINTIC_EASE_IN_OUT:
         return Easing::QuinticEaseInOut(value);
      case SINE_EASE_IN:
         return Easing::SineEaseIn(value);
      case SINE_EASE_OUT:
         return Easing::SineEaseOut(value);
      case SINE_EASE_IN_OUT:
         return Easing::SineEaseInOut(value);
      case CIRCULAR_EASE_IN:
         return Easing::CircularEaseIn(value);
      case CIRCULAR_EASE_OUT:
         return Easing::CircularEaseOut(value);
      case CIRCULAR_EASE_IN_OUT:
         return Easing::CircularEaseInOut(value);
      case EXPONENTIAL_EASE_IN:
         return Easing::ExponentialEaseIn(value);
      case EXPONENTIAL_EASE_OUT:
         return Easing::ExponentialEaseOut(value);
      case EXPONENTIAL_EASE_IN_OUT:
         return Easing::ExponentialEaseInOut(value);
      case ELASTIC_EASE_IN:
         return Easing::ElasticEaseIn(value);
      case ELASTIC_EASE_OUT:
         return Easing::ElasticEaseOut(value);
      case ELASTIC_EASE_IN_OUT:
         return Easing::ElasticEaseInOut(value);
      case BACK_EASE_IN:
         return Easing::BackEaseIn(value);
      case BACK_EASE_OUT:
         return Easing::BackEaseOut(value);
      case BACK_EASE_IN_OUT:
         return Easing::BackEaseInOut(value);
      case BOUNCE_EASE_IN:
         return Easing::BounceEaseIn(value);
      case BOUNCE_EASE_OUT:
         return Easing::BounceEaseOut(value);
      case BOUNCE_EASE_IN_OUT:
         return Easing::BounceEaseInOut(value);
      default:
         TNLAssert(false, "Unknown Easing!");
         return Easing::LinearInterpolation(value);
   }
}

namespace Easing {

// Modeled after the line y = x
AHFloat LinearInterpolation(AHFloat p)
{
	return p;
}

// Modeled after the parabola y = x^2
AHFloat QuadraticEaseIn(AHFloat p)
{
	return p * p;
}

// Modeled after the parabola y = -x^2 + 2x
AHFloat QuadraticEaseOut(AHFloat p)
{
	return -(p * (p - 2));
}

// Modeled after the piecewise quadratic
// y = (1/2)((2x)^2)             ; [0, 0.5)
// y = -(1/2)((2x-1)*(2x-3) - 1) ; [0.5, 1]
AHFloat QuadraticEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		return 2 * p * p;
	}
	else
	{
		return (-2 * p * p) + (4 * p) - 1;
	}
}

// Modeled after the cubic y = x^3
AHFloat CubicEaseIn(AHFloat p)
{
	return p * p * p;
}

// Modeled after the cubic y = (x - 1)^3 + 1
AHFloat CubicEaseOut(AHFloat p)
{
	AHFloat f = (p - 1);
	return f * f * f + 1;
}

// Modeled after the piecewise cubic
// y = (1/2)((2x)^3)       ; [0, 0.5)
// y = (1/2)((2x-2)^3 + 2) ; [0.5, 1]
AHFloat CubicEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		return 4 * p * p * p;
	}
	else
	{
		AHFloat f = ((2 * p) - 2);
		return 0.5 * f * f * f + 1;
	}
}

// Modeled after the quartic x^4
AHFloat QuarticEaseIn(AHFloat p)
{
	return p * p * p * p;
}

// Modeled after the quartic y = 1 - (x - 1)^4
AHFloat QuarticEaseOut(AHFloat p)
{
	AHFloat f = (p - 1);
	return f * f * f * (1 - p) + 1;
}

// Modeled after the piecewise quartic
// y = (1/2)((2x)^4)        ; [0, 0.5)
// y = -(1/2)((2x-2)^4 - 2) ; [0.5, 1]
AHFloat QuarticEaseInOut(AHFloat p) 
{
	if(p < 0.5)
	{
		return 8 * p * p * p * p;
	}
	else
	{
		AHFloat f = (p - 1);
		return -8 * f * f * f * f + 1;
	}
}

// Modeled after the quintic y = x^5
AHFloat QuinticEaseIn(AHFloat p) 
{
	return p * p * p * p * p;
}

// Modeled after the quintic y = (x - 1)^5 + 1
AHFloat QuinticEaseOut(AHFloat p) 
{
	AHFloat f = (p - 1);
	return f * f * f * f * f + 1;
}

// Modeled after the piecewise quintic
// y = (1/2)((2x)^5)       ; [0, 0.5)
// y = (1/2)((2x-2)^5 + 2) ; [0.5, 1]
AHFloat QuinticEaseInOut(AHFloat p) 
{
	if(p < 0.5)
	{
		return 16 * p * p * p * p * p;
	}
	else
	{
		AHFloat f = ((2 * p) - 2);
		return  0.5 * f * f * f * f * f + 1;
	}
}

// Modeled after quarter-cycle of sine wave
AHFloat SineEaseIn(AHFloat p)
{
	return sin((p - 1) * M_PI_2) + 1;
}

// Modeled after quarter-cycle of sine wave (different phase)
AHFloat SineEaseOut(AHFloat p)
{
	return sin(p * M_PI_2);
}

// Modeled after half sine wave
AHFloat SineEaseInOut(AHFloat p)
{
	return 0.5 * (1 - cos(p * M_PI));
}

// Modeled after shifted quadrant IV of unit circle
AHFloat CircularEaseIn(AHFloat p)
{
	return 1 - sqrt(1 - (p * p));
}

// Modeled after shifted quadrant II of unit circle
AHFloat CircularEaseOut(AHFloat p)
{
	return sqrt((2 - p) * p);
}

// Modeled after the piecewise circular function
// y = (1/2)(1 - sqrt(1 - 4x^2))           ; [0, 0.5)
// y = (1/2)(sqrt(-(2x - 3)*(2x - 1)) + 1) ; [0.5, 1]
AHFloat CircularEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		return 0.5 * (1 - sqrt(1 - 4 * (p * p)));
	}
	else
	{
		return 0.5 * (sqrt(-((2 * p) - 3) * ((2 * p) - 1)) + 1);
	}
}

// Modeled after the exponential function y = 2^(10(x - 1))
AHFloat ExponentialEaseIn(AHFloat p)
{
	return (p == 0.0) ? p : pow(2, 10 * (p - 1));
}

// Modeled after the exponential function y = -2^(-10x) + 1
AHFloat ExponentialEaseOut(AHFloat p)
{
	return (p == 1.0) ? p : 1 - pow(2, -10 * p);
}

// Modeled after the piecewise exponential
// y = (1/2)2^(10(2x - 1))         ; [0,0.5)
// y = -(1/2)*2^(-10(2x - 1))) + 1 ; [0.5,1]
AHFloat ExponentialEaseInOut(AHFloat p)
{
	if(p == 0.0 || p == 1.0) return p;
	
	if(p < 0.5)
	{
		return 0.5 * pow(2, (20 * p) - 10);
	}
	else
	{
		return -0.5 * pow(2, (-20 * p) + 10) + 1;
	}
}

// Modeled after the damped sine wave y = sin(13pi/2*x)*pow(2, 10 * (x - 1))
AHFloat ElasticEaseIn(AHFloat p)
{
	return sin(13 * M_PI_2 * p) * pow(2, 10 * (p - 1));
}

// Modeled after the damped sine wave y = sin(-13pi/2*(x + 1))*pow(2, -10x) + 1
AHFloat ElasticEaseOut(AHFloat p)
{
	return sin(-13 * M_PI_2 * (p + 1)) * pow(2, -10 * p) + 1;
}

// Modeled after the piecewise exponentially-damped sine wave:
// y = (1/2)*sin(13pi/2*(2*x))*pow(2, 10 * ((2*x) - 1))      ; [0,0.5)
// y = (1/2)*(sin(-13pi/2*((2x-1)+1))*pow(2,-10(2*x-1)) + 2) ; [0.5, 1]
AHFloat ElasticEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		return 0.5 * sin(13 * M_PI_2 * (2 * p)) * pow(2, 10 * ((2 * p) - 1));
	}
	else
	{
		return 0.5 * (sin(-13 * M_PI_2 * ((2 * p - 1) + 1)) * pow(2, -10 * (2 * p - 1)) + 2);
	}
}

// Modeled after the overshooting cubic y = x^3-x*sin(x*pi)
AHFloat BackEaseIn(AHFloat p)
{
	return p * p * p - p * sin(p * M_PI);
}

// Modeled after overshooting cubic y = 1-((1-x)^3-(1-x)*sin((1-x)*pi))
AHFloat BackEaseOut(AHFloat p)
{
	AHFloat f = (1 - p);
	return 1 - (f * f * f - f * sin(f * M_PI));
}

// Modeled after the piecewise overshooting cubic function:
// y = (1/2)*((2x)^3-(2x)*sin(2*x*pi))           ; [0, 0.5)
// y = (1/2)*(1-((1-x)^3-(1-x)*sin((1-x)*pi))+1) ; [0.5, 1]
AHFloat BackEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		AHFloat f = 2 * p;
		return 0.5 * (f * f * f - f * sin(f * M_PI));
	}
	else
	{
		AHFloat f = (1 - (2*p - 1));
		return 0.5 * (1 - (f * f * f - f * sin(f * M_PI))) + 0.5;
	}
}

AHFloat BounceEaseIn(AHFloat p)
{
	return 1 - BounceEaseOut(1 - p);
}

AHFloat BounceEaseOut(AHFloat p)
{
	if(p < 4/11.0)
	{
		return (121 * p * p)/16.0;
	}
	else if(p < 8/11.0)
	{
		return (363/40.0 * p * p) - (99/10.0 * p) + 17/5.0;
	}
	else if(p < 9/10.0)
	{
		return (4356/361.0 * p * p) - (35442/1805.0 * p) + 16061/1805.0;
	}
	else
	{
		return (54/5.0 * p * p) - (513/25.0 * p) + 268/25.0;
	}
}

AHFloat BounceEaseInOut(AHFloat p)
{
	if(p < 0.5)
	{
		return 0.5 * BounceEaseIn(p*2);
	}
	else
	{
		return 0.5 * BounceEaseOut(p * 2 - 1) + 0.5;
	}
}


}