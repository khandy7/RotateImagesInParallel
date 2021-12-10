#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
using namespace std;


int main() {
    int testR[3][4] = {{1,2,3,4}, {5,6,7,8}, {9,10,11,12}};
    int R[3][4];


    for (int i = 0;i < 3; i++) {
        for (int j = 0; j < 4;j++) {
            cout << testR[i][j]<< " ";
        }
        cout << endl;
    }

        //ROTATE IMAGE 180 DEGREES
            //ROTATE IMAGE 180 DEGREES
            for(int i=0; i<3; i++)
            {
                for(int j=0; j<4; j++)
                {
                    R[i][j] = testR[3 - i - 1][4 - j - 1];
                }
            }
    cout << "ROTATED\n";

    for (int i = 0;i < 3; i++) {
        for (int j = 0; j < 4;j++) {
            cout << R[i][j]<< " ";
        }
        cout << endl;
    }

    return 0;
}