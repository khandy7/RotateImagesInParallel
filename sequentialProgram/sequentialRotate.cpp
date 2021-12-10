//Final Parallel Program: Image Rotations (Assignment 2 Redo for mastery)
//By. Kyle Handy

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

//Function to get stdout from running system call
//Used to get information on images (ie. height and width)
string exec(string command) {
   char buffer[128];
   string result = "";

   // Open pipe to file
   FILE* pipe = popen(command.c_str(), "r");
   if (!pipe) {
      return "popen failed!";
   }

   // read till end of process:
   while (!feof(pipe)) {

      // use buffer to read and add to result
      if (fgets(buffer, 128, pipe) != NULL)
         result += buffer;
   }

   pclose(pipe);
   return result;
}

//parses the ouput of exec and saves the heigh and width of the image in the height and width variables
void getHeightAndWidth(string parse, string &height, string &width, string orig) {
    bool flag = false;

    for (int i = orig.length() + 28; i < (int)parse.length(); i++) {
        if (parse[i] == 'x') {
            flag = true;
        } else if (parse[i] == ',') {
            return;
        } else if (parse[i] == ' ') {
            continue;
        } else {
            if (flag) {
                width += parse[i];
            } else {
                height += parse[i];
            }
        }
    }
}


int main(int argc, char* argv[]) {

    //get rid of old results and create a fresh dir to store rotated images
    system("rm -rf results");
    system("mkdir results");
    struct dirent *entry = nullptr;

    //FOR TIMING
    double fstart = 0.0, fnow = 0.0;
    struct timespec start, now;

    char header[22];
    vector<string> files;
    int bytesLeft, bytesRead = 0, bytesWritten = 0, writecnt = 0, fdin, fdout, readcnt = 0, rc;

    //second argument is how many degrees to rotate the image (90, 180, or 270)
    if (argc != 2) {
        cout << "USAGE: ./sequentialRotate numOfDegreesToRotate\nThis program will rotate PPM images to the right by either 90, 180, or 270 degrees.\nnumOfDegreesToRotate should be 90, 180, or 270\n";
        exit(1);
    }

    int numDegrees = stoi(argv[1]);
    
    //if numDegrees not 90, 180, or 270 then exit
    if (numDegrees != 90 && numDegrees != 180 && numDegrees != 270) {
        cout << "ERROR: numOfDegrees should be 90, 180, or 270.\n";
        exit(1);
    }


    //All cards to be rotated should be in the cardsToRotate directory
    DIR* dp = opendir("cardsToRotate");

    //read all card names from the cardsToRotate directory and store them onto a vector
    if (dp != nullptr) {
        while ((entry = readdir(dp))) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".DS_Store")) {
                files.push_back(entry->d_name);
            }
        }
    }

    //START TIMING
    clock_gettime(CLOCK_MONOTONIC, &start);
    fstart = (double)start.tv_sec  + (double)start.tv_nsec / 1000000000.0;
    fnow = (double)start.tv_sec  + (double)start.tv_nsec / 1000000000.0;

    //----------------->START CARD ROTATIONS<------------------------
    //for loop loops through all cards in the files vector, rotates them one at a time
    for (int e = 0; e < int(files.size()); e++) {
        //append dir name to beginning of file name
        string originalFile = "cardsToRotate/" + files[e];

        //get width and height for current image
        string widthAndHeight = exec("file " + originalFile);
        string height = "";
        string width = "";

        //parses widthAndHeight to get image dimensions, swaps height and width
        //height and width passed by reference to get new values
        getHeightAndWidth(widthAndHeight, height, width, originalFile);

        //if numDegrees is 180, should be original dimensions, switch back height and width
        if (numDegrees == 180) {
            string tmp = height;
            height = width;
            width = tmp;
        }

        //ensure image file can be opened, if not exit program
        if((fdin = open(originalFile.c_str(), O_RDONLY, 0644)) < 0)
        {
            printf("Error opening original file %s\n", originalFile.c_str());
            return 1;
        }

        //create name of new file, same name just added to the results directory
        string newFile = "results/" + files[e];

        //ensure output file for image can be opened, if not exit program 
        if ((fdout = open(newFile.c_str(), (O_RDWR | O_CREAT), 0666)) < 0) {
            printf("Error opening new image file %s\n", newFile.c_str());
            return 1;
        }

        //number of bytes in header
        bytesLeft = 21;

        //read header of PPM file
        do
        {
            bytesRead = read(fdin, (void *)header, bytesLeft);
            bytesLeft -= bytesRead;
        } while (bytesLeft > 0);


        //clear header, data is not needed
        memset(header, 0, sizeof(header));

        //create header for new file
        //swap height and width on the header
        string newHeader = "P6\n" + width + " " + height + "\n255\n";
        strcat(header, newHeader.c_str());
        header[21] = '\0';
        bytesRead=0;

        //total number of bytes left to be read
        bytesLeft=stoi(width)*stoi(height)*3;
        readcnt=0;

        //create char arrays to hold all image data
        char R[stoi(width)][stoi(height)], testR[numDegrees == 180 ? stoi(width) : stoi(height)][numDegrees == 180 ? stoi(height) : stoi(width)];
        char G[stoi(width)][stoi(height)], testG[numDegrees == 180 ? stoi(width) : stoi(height)][numDegrees == 180 ? stoi(height) : stoi(width)];
        char B[stoi(width)][stoi(height)], testB[numDegrees == 180 ? stoi(width) : stoi(height)][numDegrees == 180 ? stoi(height) : stoi(width)];
        char RGB[stoi(height)*stoi(width)*3];
        char rotatedRGB[stoi(height)*stoi(width)*3];

        // Read in RGB data in large chunks, requesting all and reading residual
        do
        {
            bytesRead=read(fdin, (void *)&RGB[bytesRead], bytesLeft);
            bytesLeft -= bytesRead;
            readcnt++;
        } while((bytesLeft > 0) && (readcnt < 3));


        // create in memory copy from input by channel
        //creates individual matrices for red, blue, and green
        for(int i=0, pixel=0; i<stoi(width); i++)
        {
            for(int j=0; j<stoi(height); j++)
            {
                R[i][j]=RGB[pixel+0];
                G[i][j]=RGB[pixel+1];
                B[i][j]=RGB[pixel+2];
                pixel+=3;
            }
        }

        //if 90, rotate 90 degrees to the right
        if (numDegrees == 90) {
            //ROTATE IMAGE MATRIX TO THE RIGHT 90 DEGREES
            for(int i=0; i<stoi(width); i++)
            {
                for(int j=0; j<stoi(height); j++)
                {
                    testR[j][stoi(width)-1-i] = R[i][j];
                    testG[j][stoi(width)-1-i] = G[i][j];
                    testB[j][stoi(width)-1-i] = B[i][j];
                }
            }

            //if 270, rotate image left 90 degrees (ie. 90 degrees left turns into 270 degrees right)
        } else if (numDegrees == 270) {

            //ROTATE IMAGE MATRIX TO THE LEFT 90 DEGREES
            for(int i=0; i<stoi(width); i++)
            {
                for(int j=0; j<stoi(height); j++)
                {
                    testR[stoi(height)-1-j][i] = R[i][j];
                    testG[stoi(height)-1-j][i] = G[i][j];
                    testB[stoi(height)-1-j][i] = B[i][j];
                }
            }

            //otherwise rotate image 180 degrees (flip upside down)
        } else {
            //ROTATE IMAGE 180 DEGREES
            for(int i=0; i<stoi(width); i++)
            {
                for(int j=0; j<stoi(height); j++)
                {
                    testR[i][j] = R[stoi(width) - i - 1][stoi(height) - j - 1];
                    testG[i][j] = G[stoi(width) - i - 1][stoi(height) - j - 1];
                    testB[i][j] = B[stoi(width) - i - 1][stoi(height) - j - 1];
                }
            }
        }

        //dimensions for writing rotated matrices back into 1d vector
        int outer = stoi(height);
        int inner = stoi(width);

        //if rotating 180 degrees, width and height should stay the same
        if (numDegrees == 180) {
            outer = stoi(width);
            inner = stoi(height);
        }

        // create in memory copy from input by channel
        for(int i=0, pixel=0; i<outer; i++)
        {
            for(int j=0; j<inner; j++)
            {
                rotatedRGB[pixel+0]=testR[i][j];
                rotatedRGB[pixel+1]=testG[i][j];
                rotatedRGB[pixel+2]=testB[i][j];
                pixel+=3;
            }
        }


        //MATRICES HAVE BEEN ROTATED AND STORED BACK INTO A 1D ARRAY,
        //NOW WRITE ALL DATA INTO THE NEW FILE

        //write header to new image file
        rc=write(fdout, (void *)header, 21);

        if (!rc) {
            cout << "ERROR: could not write to file" << endl;
            return 1;
        }

        bytesWritten=0;
        bytesLeft=stoi(height)*stoi(width)*3;
        writecnt=0;

        // Write RGB data in large chunks
        do
        {
            bytesWritten=write(fdout, (void *)&rotatedRGB[bytesWritten], bytesLeft);
            bytesLeft -= bytesWritten;
            writecnt++;
        } while((bytesLeft > 0) && (writecnt < 3));

        //close input and output files
        close(fdin);
        close(fdout);
    }
    //----------------->END CARD ROTATIONS<------------------------


    //Get total time taken to rotate all cards
    clock_gettime(CLOCK_MONOTONIC, &now);
    fnow = (double)now.tv_sec  + (double)now.tv_nsec / 1000000000.0;

    printf("Completed rotating 52 cards in %f seconds\n", fnow - fstart);

    return 0;
}