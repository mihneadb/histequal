#include <mpi.h>
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
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int channels[] = {-1, RED, GREEN, BLUE};

    if (rank == 0) {
        CImg<unsigned char> img("img.jpg");

        int width = img.width();
        int height = img.height();
        int size = width * height * 3;
        MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(img.data(), size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);


        // receive the 3 channels
        MPI_Status status;

        int i;
        for (i = 1; i < 4; ++i) {
            MPI_Recv(img.data() + width * height * (i - 1), width * height,
                    MPI_UNSIGNED_CHAR, i, i - 1, MPI_COMM_WORLD, &status);
        }
        img.save("img-out.jpg");
    } else {
        int size;
        int width, height;

        int channel = channels[rank];

        MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);

        size = width * height * 3;

        unsigned char *data = (unsigned char *) malloc(size * sizeof(unsigned char));
        MPI_Bcast(data, size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

        CImg<unsigned char> img(data, width, height, 1, 3);

        equalize_channel(img, channel);

        // send back only own channel
        MPI_Send(img.data() + width * height * channel, width * height,
                MPI_UNSIGNED_CHAR, 0, channel, MPI_COMM_WORLD);
    }


    MPI_Finalize();
    return 0;
}

