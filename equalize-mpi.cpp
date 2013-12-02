#include "CImg.h"
using namespace cimg_library;

#include <stdio.h>
#include <map>

#define RED 0
#define GREEN 1
#define BLUE 2

std::map<unsigned char, float> make_hist(
        CImg<unsigned char> &img,
        int channel)
{
    int x, y;
    int width = img.width();
    int height = img.height();
    unsigned char val;
    int count;

    std::map<unsigned char, float> hist;

    for (x = 0; x < width; ++x) {
        for (y = 0; y < height; ++y) {
            val = img(x, y, channel);
            count = hist[val];
            hist[val] = count + 1;
        }
    }

    int total = width * height;
    std::map<unsigned char, float>::iterator it;
    for (it = hist.begin(); it != hist.end(); ++it) {
        it->second = it->second / total;
    }

    return hist;
}

std::map<unsigned char, int> make_cdf(std::map<unsigned char, float> &hist,
        int total)
{
    std::map<unsigned char, float>::iterator it;
    std::map<unsigned char, int> cdf;

    int val, j;
    float freq, sum;

    for (it = hist.begin(); it != hist.end(); ++it) {
        val = it->first;
        freq = it->second;
        sum = 0;
        for (j = 0; j <= val; ++j) {
            if (hist.find(j) != hist.end()) {
                sum += (hist[j] * total);
            }
        }
        cdf[val] = (int)sum;
    }

    return cdf;
}

int cdf_min(std::map<unsigned char, int> &cdf)
{
    int min = 99999;
    std::map<unsigned char, int>::iterator it;
    for (it = cdf.begin(); it != cdf.end(); ++it) {
        if (it->second < min) {
            min = it->second;
        }
    }
    return min;
}

int cdf_max(std::map<unsigned char, int> &cdf)
{
    int max = -1;
    std::map<unsigned char, int>::iterator it;
    for (it = cdf.begin(); it != cdf.end(); ++it) {
        if (it->second > max) {
            max = it->second;
        }
    }
    return max;
}

int new_val(int old, int cdfmin, int width, int height,
        std::map<unsigned char, int> &cdf)
{
    int x = (float)(cdf[old] - cdfmin) / (float)((width * height) - cdfmin) * 255.0;
    return x;
}

std::map<unsigned char, unsigned char> make_transform(int cdfmin,
        int width, int height, std::map<unsigned char, int> &cdf)
{
    std::map<unsigned char, int>::iterator it;
    std::map<unsigned char, unsigned char> transform;
    unsigned char old;
    for (it = cdf.begin(); it != cdf.end(); ++it) {
        old = it->first;
        transform[old] = new_val(old, cdfmin, width, height, cdf);
    }
    return transform;
}

void equalize_channel(CImg<unsigned char> &img, int channel)
{
    int width = img.width();
    int height = img.height();

    std::map<unsigned char, float> hist = make_hist(img, channel);
    std::map<unsigned char, int> cdf = make_cdf(hist, width * height);
    int cdfmin = cdf_min(cdf);
    std::map<unsigned char, unsigned char> transform = make_transform(cdfmin,
            width, height, cdf);

    int x, y;
    unsigned char val;
    for (x = 0; x < width; ++x) {
        for (y = 0; y < height; ++y) {
            val = img(x, y, channel);
            img(x, y, channel) = transform[val];
        }
    }
}


int main(int argc, char **argv)
{
    CImg<unsigned char> img("img.jpg");

    equalize_channel(img, RED);
    equalize_channel(img, GREEN);
    equalize_channel(img, BLUE);

    img.save("img-out.jpg");

    return 0;
}

