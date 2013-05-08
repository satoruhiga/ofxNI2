#pragma once

#include "ofMain.h"

namespace ofxNI2
{

// median search algorithm: via http://ndevilla.free.fr/median/median.pdf

template <typename T, int N>
struct Median
{
	inline static T torben(T *m, int n)
	{
		int i, less, greater, equal;
		T min, max, guess, maxltguess, mingtguess;
		min = max = m[0];
		for (i = 1; i < n; i++)
		{
			if (m[i] < min) min = m[i];
			if (m[i] > max) max = m[i];
		}
		while (1)
		{
			guess = (min + max) / 2;
			less = 0;
			greater = 0;
			equal = 0;
			maxltguess = min;
			mingtguess = max;
			for (i = 0; i < n; i++)
			{
				if (m[i] < guess)
				{
					less++;
					if (m[i] > maxltguess) maxltguess = m[i];
				}
				else if (m[i] > guess)
				{
					greater++;
					if (m[i] < mingtguess) mingtguess = m[i];
				}
				else equal++;
			}
			if (less <= (n + 1) / 2 && greater <= (n + 1) / 2) break;
			else if (less > greater) max = maxltguess;
			else min = mingtguess;
		}
		if (less >= (n + 1) / 2) return maxltguess;
		else if (less + equal >= (n + 1) / 2) return guess;
		else return mingtguess;
	}

	inline static T get(T *m)
	{
		return torben(m, N);
	}
};

#define PIX_SORT(a, b) { if ((a) > (b)) std::swap((a), (b)); }

template <typename T>
struct Median<T, 3>
{
	inline static T opt_med3(T * p)
	{
		PIX_SORT(p[0], p[1]);
		PIX_SORT(p[1], p[2]);
		PIX_SORT(p[0], p[1]);
		return(p[1]);
	}

	inline static T get(T *m)
	{
		return opt_med3(m);
	}
};

template <typename T>
struct Median<T, 5>
{
	inline static T opt_med5(T * p)
	{
		PIX_SORT(p[0], p[1]);
		PIX_SORT(p[3], p[4]);
		PIX_SORT(p[0], p[3]);
		PIX_SORT(p[1], p[4]);
		PIX_SORT(p[1], p[2]);
		PIX_SORT(p[2], p[3]);
		PIX_SORT(p[1], p[2]);
		return(p[2]);
	}

	inline static T get(T *m)
	{
		return opt_med5(m);
	}
};

template <typename T>
struct Median<T, 7>
{
	inline static T opt_med7(T * p)
	{
		PIX_SORT(p[0], p[5]);
		PIX_SORT(p[0], p[3]);
		PIX_SORT(p[1], p[6]);
		PIX_SORT(p[2], p[4]);
		PIX_SORT(p[0], p[1]);
		PIX_SORT(p[3], p[5]);
		PIX_SORT(p[2], p[6]);
		PIX_SORT(p[2], p[3]);
		PIX_SORT(p[3], p[6]);
		PIX_SORT(p[4], p[5]);
		PIX_SORT(p[1], p[4]);
		PIX_SORT(p[1], p[3]);
		PIX_SORT(p[3], p[4]);
		return (p[3]);
	}

	inline static T get(T *m)
	{
		return opt_med7(m);
	}
};

#undef PIX_SORT

template <typename T, int NUM_FRAME = 5>
class TimedomainMedianFilter
{
public:

	TimedomainMedianFilter() : current_frame_index(0) {}

	void setup(int w, int h)
	{
		pixels.resize(w * h * NUM_FRAME, 0);
		result.allocate(w, h, OF_IMAGE_GRAYSCALE);
		result.set(0);
	}

	const ofPixels_<T>& update(ofPixels_<T> &pix)
	{
		assert(pix.getNumChannels() == 1);
		
		const int N = pix.getWidth() * pix.getHeight();

		T *pixels_ptr = &pixels[0];

		memcpy(pixels_ptr + current_frame_index * N, pix.getPixels(), N);

		{
			T *result_ptr = result.getPixels();
			T arr[NUM_FRAME];

			for (int n = 0; n < N; n++)
			{
				const T *ptr = pixels_ptr + n;

				for (int i = 0; i < NUM_FRAME; i++)
				{
					arr[i] = *ptr;
					ptr += N;
				}

				result_ptr[n] = Median<T, NUM_FRAME>::get(arr);
			}
		}

		current_frame_index++;
		current_frame_index %= NUM_FRAME;

		return result;
	}
	
	const ofPixels_<T>& get() const { return result; }
	ofPixels_<T>& get() { return result; }

protected:

	int current_frame_index;

	vector<T> pixels;
	ofPixels_<T> result;
};

}