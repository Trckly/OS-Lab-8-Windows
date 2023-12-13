#include <iostream>
#include <string>
#include <cmath>
#include <fstream>

const float PI = 3.1415;

float f(int x)
{
    return 25 * x * exp(x / 25) + pow(cos(x * (PI / 180)), 1 / 25);
}

int main(int argc, char *argv[])
{
    int LimitA = std::atoi(argv[1]), LimitB = std::atoi(argv[2]), StepH = std::atoi(argv[3]);

    std::ofstream fout("\\\\Mac\\Home\\Documents\\OC\\OC_Lab_3\\ParentTabulation\\Log.txt", std::ios::trunc);

    if (StepH > fabs(LimitA) + fabs(LimitB)) {
        return -1;
    } else {
        for (int x = LimitA; x < LimitB; x += StepH) {
            fout << f(x) << '\n';
        }
    }
    fout.close();

    getchar();

    return 0;
}
