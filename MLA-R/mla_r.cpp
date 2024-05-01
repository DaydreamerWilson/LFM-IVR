#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <windows.h>
#include <chrono>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif

class vector3;
class vector4;

class _hitRecord;
class _inverseHitRecord;

static stbi_uc* loadImage(char *filename, int &x, int &y);
static int saveImage(char *filename, stbi_uc *data, int x, int y, int n);

static int assignPixel(stbi_uc* data, int x, int y, int R, int G, int B, int A, int mode);
static int getPixel(stbi_uc* data, int x, int y, int &R, int &G, int &B, int &A);
static short int getPixelSingleChannel(short int* data, int x, int y);
static void setPixelSingleChannel(short int* data, int x, int y, int val);
static void addPixelSingleChannel(short int* data, int x, int y, int val);
static int pixelColorToAlpha(stbi_uc* data, int x, int y);
static int invertPixel(stbi_uc* data, int x, int y);
static void removeBackground(stbi_uc* data);
static int inverseTransformMap(stbi_uc* data);
static void incAvg(double &R, double &G, double &B, double &A, double xR, double xG, double xB, double xA, int n);
static int _stackPixel(stbi_uc* data, int x, int y, double &R, double &G, double &B, double &A, int n);
static int stackPixel(stbi_uc *src, int x, int y, stbi_uc *dst, int t_x, int t_y, int n);
static int drawPixel(stbi_uc* data, int x, int y, int R, int G, int B, int A, int mode);

static int index_rectify(int i, int fs);
static int load_all_maps(char* folder, stbi_uc *&top_map, stbi_uc **&x_map, stbi_uc **&y_map, int fs);
static int saveAllMap(char* folder, char *new_folder, stbi_uc *top_map, stbi_uc **x_map, stbi_uc **y_map, int fs);
static int saveStack(char* folder, char* new_folder, stbi_uc **stack, int z_n);
static void inverseTransformAllMap(stbi_uc *top_map, stbi_uc **x_map, stbi_uc **y_map, int fs);
static void allMap2Monochrome(stbi_uc *top_map, stbi_uc **x_map, stbi_uc **y_map, int fs);
static void inverseImageStack(stbi_uc **stacks, int z_n);

static int hit(int tx, int ty, int ix, int iy, int x_off, int y_off, int z);
static _hitRecord hittable(int tx, int ty, int x_off, int y_off, int z_n);
static _inverseHitRecord inverseHittable(int x, int y, int z, int fs);

static stbi_uc** stackCopy(stbi_uc **data, int z_n);
static stbi_uc** stackCreate(int z_n);
static void deleteStack(stbi_uc **stacks, int z_n);

static void init_stacks(stbi_uc **stacks, stbi_uc *top_map, stbi_uc *x_map, stbi_uc *y_map, int fs);
static stbi_uc** stackIterate(stbi_uc **stacks, stbi_uc *top_map, stbi_uc **x_map, stbi_uc **y_map, int fs, int z_n, double lr);

int h = 0;
int w = 0;

static stbi_uc* loadImage(char *filename, int &x, int &y){
    stbi_uc* data = (stbi_uc*)malloc(h*w*4*sizeof(char));
    int n;
    //printf("Loading image: ");
    stbi_uc* t_data = stbi_load(filename, &x, &y, &n, STBI_rgb_alpha);
    memcpy(data, t_data, x*y*4*sizeof(char));
    //printf("%d %d %d at %p\n", x, y, n, data);
    if(data != NULL){return data;}
    return NULL;
}

static int saveImage(char *filename, stbi_uc *data, int x, int y, int n){
    if(data != NULL){
        if(!stbi_write_png(filename, x, y, 4, data, x*4)){
            printf("Failed to write image!\n");
            return false;
        }
        return true;
    }
    else{
        return false;
    }
}

enum{DW_MODE_REPLACE, DW_MODE_ADDITION, DW_MODE_SUBTRACTION, DW_MODE_MULTIPLY};

static int assignPixel(stbi_uc* data, int x, int y, int R, int G, int B, int A, int mode){
    int tR, tG, tB, tA;
    if(mode == DW_MODE_REPLACE){
        tR = R;
        tG = G;
        tB = B;
        tA = A;
    }
    if(mode == DW_MODE_ADDITION){
        tR = (int)data[4*(x+w*y)]+R;
        tG = (int)data[4*(x+w*y)+1]+G;
        tB = (int)data[4*(x+w*y)+2]+B;
        tA = (int)data[4*(x+w*y)+3]+A;
    }
    if(mode == DW_MODE_SUBTRACTION){
        tR = (int)data[4*(x+w*y)]-R;
        tG = (int)data[4*(x+w*y)+1]-G;
        tB = (int)data[4*(x+w*y)+2]-B;
        tA = (int)data[4*(x+w*y)+3]-A;
    }
    if(mode == DW_MODE_MULTIPLY){
        tR = (int)data[4*(x+w*y)]*R;
        tG = (int)data[4*(x+w*y)+1]*G;
        tB = (int)data[4*(x+w*y)+2]*B;
        tA = (int)data[4*(x+w*y)+3]*A;
    }
    // Overflow check
    if(tR>255){tR=255;}
    if(tG>255){tG=255;}
    if(tB>255){tB=255;}
    if(tA>255){tA=255;}
    // Underflow check
    if(tR<0){tR=0;}
    if(tG<0){tG=0;}
    if(tB<0){tB=0;}
    if(tA<0){tA=0;}
    // Assign value
    data[4*(x+w*y)] = tR;
    data[4*(x+w*y)+1] = tG;
    data[4*(x+w*y)+2] = tB;
    data[4*(x+w*y)+3] = tA;
    return true;
}

static int getPixel(stbi_uc* data, int x, int y, int &R, int &G, int &B, int &A){
    if(x>=0 && x<w && y>=0 && y<h){
        R = (int)data[4*(x+w*y)];
        G = (int)data[4*(x+w*y)+1];
        B = (int)data[4*(x+w*y)+2];
        A = (int)data[4*(x+w*y)+3];
        return true;
    }
    else{
        return false;
    }
}

static short int getPixelSingleChannel(short int* data, int x, int y){
    //printf("Fetching %d from %d %d\n", data[(x+w*y)], x, y);
    return data[(x+w*y)];
}

static void setPixelSingleChannel(short int* data, int x, int y, int val){
    //printf("Setting %d %d to %d\n", x, y, val);
    data[(x+w*y)] = val;
}

static void addPixelSingleChannel(short int* data, int x, int y, int val){
    //printf("Adding %d to %d %d\n", val, x, y);
    data[(x+w*y)] += (short int)val;
}

static int pixelColorToAlpha(stbi_uc* data, int x, int y){
    int R, G, B, A;
    if(!getPixel(data, x, y, R, G, B, A)){return false;}
    if(A!=0){
        A = (R+B+G)/3;
        if(!drawPixel(data, x, y, R, G, B, A, DW_MODE_REPLACE)){return false;}
        return true;
    }
    return true;
}

static int invertPixel(stbi_uc* data, int x, int y){
    int R, G, B, A;
    if(!getPixel(data, x, y, R, G, B, A)){return false;}
    if(!drawPixel(data, x, y, 255-R, 255-G, 255-B, A, DW_MODE_REPLACE)){return false;}
    return true;
}

static void removeBackground(stbi_uc* data){
    double R = 0;
    double G = 0;
    double B = 0;
    double A = 0;
    int counter = 0;

    // Obtain background power
    for(int x = 0; x<w; x++){
        for(int y = 0; y<h; y++){
            if(_stackPixel(data, x, y, R, G, B, A, counter)){counter++;}
        }
    }

    // Subtract background
    for(int x = 0; x<w; x++){
        for(int y = 0; y<h; y++){
            assignPixel(data, x, y, R, G, B, 0, DW_MODE_SUBTRACTION);
        }
    }
}

static int inverseTransformMap(stbi_uc* data){
    for(int i = 0; i<w; i++){
        for(int j = 0; j<h; j++){
            invertPixel(data, i, j);
            pixelColorToAlpha(data, i, j);
        }
    }
    removeBackground(data);
    return true;
}

// Extract the n-th item (R,G,B,A) given the average of 1-st to (n-1)-th items (xR,xG,xB,xA) and the average of the n items (R,G,B,A)
static void invAvg(double &R, double &G, double &B, double &A, double xR, double xG, double xB, double xA, int n){
    double _n = (double)n;
    R = xR*(_n)-R*(_n-1);
    G = xG*(_n)-G*(_n-1);
    B = xB*(_n)-B*(_n-1);
    A = xA*(_n)-A*(_n-1);
    if(R<0){R=0;} if(R>255){R=255;}
    if(G<0){G=0;} if(G>255){G=255;}
    if(B<0){B=0;} if(B>255){B=255;}
    if(A<0){A=0;} if(A>255){A=255;}
}

// Minimal loss step averaging function, taking the average of the (n+1) items given the (n+1)-th item and the average of the n items
static void incAvg(double &xR, double &xG, double &xB, double &xA, double R, double G, double B, double A, int n){
    double _n = (double)n;
    if(n!=0){
        xR = (xR*(_n/(_n+1)))+(R/(_n+1));
        xG = (xG*(_n/(_n+1)))+(G/(_n+1));
        xB = (xB*(_n/(_n+1)))+(B/(_n+1));
        xA = (xA*(_n/(_n+1)))+(A/(_n+1));
    }
    else{
        xR = R;
        xG = G;
        xB = B;
        xA = A;
    }
}

static int _stackPixel(stbi_uc* data, int x, int y, double &R, double &G, double &B, double &A, int n){
    double _n = (double)n;
    if(x>=0 && x<w && y>=0 && y<h && (int)data[4*(x+w*y)+3]!=0){
        if(n!=0){
            R = R*(_n/(_n+1))+(double)data[4*(x+w*y)]/(_n+1);
            G = G*(_n/(_n+1))+(double)data[4*(x+w*y)+1]/(_n+1);
            B = B*(_n/(_n+1))+(double)data[4*(x+w*y)+2]/(_n+1);
            A = A*(_n/(_n+1))+(double)data[4*(x+w*y)+3]/(_n+1);
        }
        else{
            R = (double)data[4*(x+w*y)];
            G = (double)data[4*(x+w*y)+1];
            B = (double)data[4*(x+w*y)+2];
            A = (double)data[4*(x+w*y)+3];
        }
        return true;
    }
    else{
        return false;
    }
}

static int stackPixel(stbi_uc *src, int x, int y, stbi_uc *dst, int t_x, int t_y, int n){
    int R, G, B, A;
    getPixel(src, x, y, R, G, B, A);
    double _n = (double)n;
    if(t_x>=0 && t_x<w && t_y>=0 && t_y<h && A!=0){
        if(n!=0){
            dst[4*(t_x+w*t_y)] = dst[4*(t_x+w*t_y)]*(_n/(_n+1))+(double)R/(_n+1);
            dst[4*(t_x+w*t_y)+1] = dst[4*(t_x+w*t_y)+1]*(_n/(_n+1))+(double)G/(_n+1);
            dst[4*(t_x+w*t_y)+2] = dst[4*(t_x+w*t_y)+2]*(_n/(_n+1))+(double)B/(_n+1);
            dst[4*(t_x+w*t_y)+3] = dst[4*(t_x+w*t_y)+3]*(_n/(_n+1))+(double)A/(_n+1);
        }
        else{
            dst[4*(t_x+w*t_y)] = R;
            dst[4*(t_x+w*t_y)+1] = G;
            dst[4*(t_x+w*t_y)+2] = B;
            dst[4*(t_x+w*t_y)+3] = A;
        }
        return true;
    }
    else{
        return false;
    }
}

static int drawPixel(stbi_uc* data, int x, int y, int R, int G, int B, int A, int mode){
    if(x>=0 && x<w && y>=0 && y<h){
        assignPixel(data, x, y, R, G, B, A, mode);
        return true;
    }
    else{
        return false;
    }
}

static int index_rectify(int i, int fs){
    if(i>0){return i-1;}
    if(i<0){return fs*2+i;}
    return -1;
}

static int stack_index(int i, int z_n){
    if(i>=0){return i;}
    if(i<0){return z_n+i;}
    return -1;
}

static int load_config(char* folder, int &fs){
    char* tempPath = new char[256];

    sprintf(tempPath, "%s/config", folder);
    FILE * config = fopen(tempPath, "r");
    fscanf(config, "%d %d\n%d\n", &w, &h, &fs);

    return true;
}

static int load_all_maps(char* folder, stbi_uc *&top_map, stbi_uc **&x_map, stbi_uc **&y_map, int fs){
    int t_x, t_y;

    char *tempPath = new char[256];
    sprintf(tempPath, "%s/0_0.png", folder);
    top_map = loadImage(tempPath, t_x, t_y);
    if(top_map == NULL){
        return false;
    }
    if(t_x != w || t_y != h){
        printf("Error: Image size mismatch with config!");
        return false;
    }

    //printf("Loading x maps\n");
    x_map = (stbi_uc**)malloc(fs*2*sizeof(stbi_uc*));
    for(int x = -fs; x < fs+1; x++){
        if(x==0){continue;}
        sprintf(tempPath, "%s/%d_%d.png", folder, x, 0);
        //printf("Accessing x map: %d -> %d: ", x, index_rectify(x, fs));
        x_map[index_rectify(x, fs)]=loadImage(tempPath, t_x, t_y);
        if(x_map[index_rectify(x, fs)] == NULL){
            return false;
        }
        if(t_x != w || t_y != h){
            printf("Error: Image size mismatch with config!");
            return false;
        }
    }

    //printf("Loading y maps\n");
    y_map = (stbi_uc**)malloc(fs*2*sizeof(stbi_uc*));
    for(int y = -fs; y < fs+1; y++){
        if(y==0){continue;}
        sprintf(tempPath, "%s/%d_%d.png", folder, 0, y);
        //printf("Accessing y map: %d -> %d: ", y, index_rectify(y, fs));
        y_map[index_rectify(y, fs)]=loadImage(tempPath, t_x, t_y);
        if(y_map[index_rectify(y, fs)] == NULL){
            return false;
        }
        if(t_x != w || t_y != h){
            printf("Error: Image size mismatch with config!");
            return false;
        }
    }

    return true;
}

static int saveAllMap(char* folder, char* new_folder, stbi_uc *top_map, stbi_uc **x_map, stbi_uc **y_map, int fs){
    char *tempPath = new char[256];
    sprintf(tempPath, "%s/%s", folder, new_folder);
    CreateDirectory(tempPath, NULL);

    sprintf(tempPath, "%s/%s/%d_%d.png", folder, new_folder, 0, 0);
    if(!saveImage(tempPath, top_map, w, h, 4)){return false;}

    for(int x = -fs; x < fs+1; x++){
        if(x==0){continue;}
        sprintf(tempPath, "%s/%s/%d_%d.png", folder, new_folder, x, 0);
        if(!saveImage(tempPath, x_map[index_rectify(x, fs)], w, h, 4)){return false;}
    }

    for(int y = -fs; y < fs+1; y++){
        if(y==0){continue;}
        sprintf(tempPath, "%s/%s/%d_%d.png", folder, new_folder, 0, y);
        if(!saveImage(tempPath, y_map[index_rectify(y, fs)], w, h, 4)){return false;}
    }

    return true;
}

static int saveStack(char* folder, char* new_folder, stbi_uc **stack, int z_n){
    char *tempPath = new char[256];
    sprintf(tempPath, "%s/%s", folder, new_folder);
    CreateDirectory(tempPath, NULL);

    for(int i = -z_n/2; i < z_n/2+1; i++){
        sprintf(tempPath, "%s/%s/%d.png", folder, new_folder, i+z_n/2);
        if(!saveImage(tempPath, stack[stack_index(i, z_n)], w, h, 4)){return false;}
    }

    return true;
}

static void inverseTransformAllMap(stbi_uc *top_map, stbi_uc **x_map, stbi_uc **y_map, int fs){
    inverseTransformMap(top_map);
    for(int x = -fs; x < fs+1; x++){
        if(x==0){continue;}
        inverseTransformMap(x_map[index_rectify(x, fs)]);
    }
    for(int y = -fs; y < fs+1; y++){
        if(y==0){continue;}
        inverseTransformMap(y_map[index_rectify(y, fs)]);
    }
}

static void allMap2Monochrome(stbi_uc *top_map, stbi_uc **x_map, stbi_uc **y_map, int fs){}

static void inverseImageStack(stbi_uc **stacks, int z_n){
    for(int z = -z_n/2; z < z_n/2+1; z++){
        for(int i = 0; i<w; i++){
            for(int j = 0; j<h; j++){
                invertPixel(stacks[stack_index(z, z_n)], i, j);
            }
        }
    }
}

// Stores the hit record for a ray
class vector3 {
    public:
    short int x, y, z;
    vector3(int x, int y, int z){
        this->x = x;
        this->y = y;
        this->z = z;
    }
    vector3(){}
};

class vector4 {
    public:
    short int x, y, i, j;
    vector4(int x, int y, int i, int j){
        this->x = x;
        this->y = y;
        this->i = i;
        this->j = j;
    }
    vector4(){}
};

class _hitRecord {
    private:
    int _pushCounter = 0;
    void _expand(){
        vector3 *temp = new vector3[this->size*2];
        this->size *= 2;
        memcpy(temp, this->hits, this->counter*sizeof(vector3));
        delete[] this->hits;
        this->hits = temp;
    }

    public:
    vector3 *hits;
    int size = 1;
    int counter = 0;
    _hitRecord(){
        this->hits = new vector3[size];
    }

    void freeRecord(){
        if(this->hits!=NULL){
            delete[] this->hits;
        }
    }

    void push(int x, int y, int z){
        if(counter==size){
            this->_expand();
        }
        this->hits[counter] = vector3(x, y, z);
        counter++;
    }

    int next(vector3 &val){
        if(_pushCounter==counter){
            return false;
        }
        else{
            val = this->hits[_pushCounter++];
            return true;
        }
    }

    void resetNext(){
        _pushCounter = 0;
    }

    void fit(){
        vector3 *temp = new vector3[counter];
        size = counter;
        memcpy(temp, this->hits, counter*sizeof(vector3));
        delete[] this->hits;
        this->hits = temp;
    }
};

class _inverseHitRecord {
    private:
    int _pushCounter = 0;
    void _expand(){
        vector4 *temp = new vector4[this->size*2];
        this->size *= 2;
        memcpy(temp, this->hits, this->counter*sizeof(vector4));
        delete[] this->hits;
        this->hits = temp;
    }

    public:
    vector4 *hits;
    int size = 1;
    int counter = 0;
    _inverseHitRecord(){
        this->hits = new vector4[size];
    }

    void freeRecord(){
        if(this->hits!=NULL){
            delete[] this->hits;
        }
        this->hits = NULL;
    }

    void push(int x, int y, int i, int j){
        if(counter==size){
            this->_expand();
        }
        this->hits[counter] = vector4(x, y, i, j);
        counter++;
    }

    int next(vector4 &val){
        if(_pushCounter==counter){
            return false;
        }
        else{
            val = this->hits[_pushCounter++];
            return true;
        }
    }

    void resetNext(){
        _pushCounter = 0;
    }

    void fit(){
        vector4 *temp = new vector4[counter];
        size = counter;
        memcpy(temp, this->hits, counter*sizeof(vector4));
        delete[] this->hits;
        this->hits = temp;
    }
};

// Return if the ray of ix, iy will hit the voxel of tx, ty, z
// t is target planar coordinate, i is coordinate on angle map, off is the axis offset, z is the target plane
static int hit(int tx, int ty, int ix, int iy, int x_off, int y_off, int z){
    if(tx==ix+(x_off*z) && ty==iy+(y_off*z)){
        return true;
    }
    return false;
}

// Write the corresponding value to R, G, B, A from the oblique maps
static void angleMap(int ix, int iy, int x_off, int y_off, int &R, int &G, int &B, int &A, int fs, stbi_uc *top_map, stbi_uc **x_map, stbi_uc **y_map){
    if(x_off == 0 && y_off == 0){
        //printf("0");
        getPixel(top_map, ix, iy, R, G, B, A);
    }
    if(x_off >= -fs && x_off <= fs && x_off != 0){
        //printf("%d x:%d\n", index_rectify(x_off, fs), x_off);
        getPixel(x_map[index_rectify(x_off, fs)], ix, iy, R, G, B, A);
    }
    if(y_off >= -fs && y_off <= fs && y_off != 0){
        //printf("%d y:%d\n", index_rectify(y_off, fs), y_off);
        getPixel(y_map[index_rectify(y_off, fs)], ix, iy, R, G, B, A);
    }
}

// Returns a list of voxels that will be hit by the ray
// i is coordinate on angle map, off is the axis offset
static _hitRecord hittable(int ix, int iy, int x_off, int y_off, int z_n){
    _hitRecord temp;
    int counter = 0;
    for(int z = -z_n/2; z < z_n/2+1; z++){
        int x = ix+(x_off*z);
        int y = iy+(y_off*z);
        if(x>=0 && x<w && y>=0 && y<h){
            temp.push(x, y, z);
        }
    }
    return temp;
}

// Returns a list of the rays that will pass through the voxel
static _inverseHitRecord inverseHittable(int x, int y, int z, int fs){
    _inverseHitRecord temp;
    int counter = 0;
    temp.push(x, y, 0, 0);
    for(int i = -fs; i < fs+1; i++){
        if(i==0){continue;}
        int t_x = x+(i*z);
        if(t_x>=0 && t_x<w){
            temp.push(t_x, y, i, 0);
        }
    }
    for(int j = -fs; j < fs+1; j++){
        if(j==0){continue;}
        int t_y = y+(j*z);
        if(t_y>=0 && t_y<h){
            temp.push(x, t_y, 0, j);
        }
    }
    return temp;
}

// Initiate the volume space
// Estimate the inital volume
static void init_stacks(stbi_uc **stacks, stbi_uc *top_map, stbi_uc **x_map, stbi_uc **y_map, int fs, int z_n){
    int total_hit_count = 0;
    short **counter_map = new short *[z_n];
    for(int z = 0; z < z_n; z++){
        counter_map[z] = (short*)calloc(h*w, sizeof(short));
    }

    _hitRecord tempRecord;
    vector3 val;

    printf("Starting hit scan\n");

    for(int ix = 0; ix<w; ix++){
        for(int iy = 0; iy<h; iy++){
            tempRecord.freeRecord();
            tempRecord = hittable(ix, iy, 0, 0, z_n);
            while(tempRecord.next(val)){
                total_hit_count++;
                if(stackPixel(top_map, ix, iy, stacks[stack_index(val.z, z_n)], val.x, val.y, getPixelSingleChannel(counter_map[stack_index(val.z, z_n)], val.x, val.y)))
                {addPixelSingleChannel(counter_map[stack_index(val.z, z_n)], val.x, val.y, (short)1);}
            }
        }
    }
    printf("%.2f%% completed\n", 100.0f/(fs*4+1));

    for(int i = -fs; i < fs+1; i++){
        if(i==0){continue;}
        for(int ix = 0; ix<w; ix++){
            for(int iy = 0; iy<h; iy++){
                tempRecord.freeRecord();
                tempRecord = hittable(ix, iy, i, 0, z_n);
                while(tempRecord.next(val)){
                    total_hit_count++;
                    if(stackPixel(x_map[index_rectify(i, fs)], ix, iy, stacks[stack_index(val.z, z_n)], val.x, val.y, getPixelSingleChannel(counter_map[stack_index(val.z, z_n)], val.x, val.y)))
                    {addPixelSingleChannel(counter_map[stack_index(val.z, z_n)], val.x, val.y, 1);}
                }
            }
        }
    }
    printf("%.2f%% completed\n", (fs*2+1)*100.0f/(fs*4+1));
    

    for(int j = -fs; j < fs+1; j++){
        if(j==0){continue;}
        for(int ix = 0; ix<w; ix++){
            for(int iy = 0; iy<h; iy++){
                tempRecord.freeRecord();
                tempRecord = hittable(ix, iy, 0, j, z_n);
                while(tempRecord.next(val)){
                    total_hit_count++;
                    if(stackPixel(y_map[index_rectify(j, fs)], ix, iy, stacks[stack_index(val.z, z_n)], val.x, val.y, getPixelSingleChannel(counter_map[stack_index(val.z, z_n)], val.x, val.y)))
                    {addPixelSingleChannel(counter_map[stack_index(val.z, z_n)], val.x, val.y, 1);}
                }
            }
        }
    }

    tempRecord.freeRecord();

    for(int z = 0; z < z_n; z++){
        delete[] counter_map[z];
    }

    printf("%.2f%% completed\n", (fs*4+1)*100.0f/(fs*4+1));
    printf("Total hit count with hit: %d\n", total_hit_count);
}

// Copy the stack and return a pointer to the deep copied stack
// Notes this does not delete the original stack
static stbi_uc** stackCopy(stbi_uc **stacks, int z_n){
    stbi_uc **retStack = new stbi_uc*[z_n];
    for(int i = 0; i<z_n; i++){
        retStack[i] = (stbi_uc*)calloc(h*w*4, 1);
        memcpy(retStack[i], stacks[i], h*w*4);
    }
    return retStack;
}

// Create an empty stack of the corresponding size
static stbi_uc** stackCreate(int z_n){
    stbi_uc **retStack = new stbi_uc*[z_n];
    for(int i = 0; i<z_n; i++){
        retStack[i] = (stbi_uc*)calloc(h*w*4, 1);
    }
    return retStack;
}

// Delete the stack to free the memory
static void deleteStack(stbi_uc **stacks, int z_n){
    for(int i = 0; i<z_n; i++){
        delete[] stacks[i];
    }
    delete[] stacks;
}

static stbi_uc** stackIterate(stbi_uc **stacks, stbi_uc *top_map, stbi_uc **x_map, stbi_uc **y_map, int fs, int z_n, double lr){
    stbi_uc** new_stack = stackCopy(stacks, z_n);
    int hit_counter = 0;
    _inverseHitRecord iRecord;
    _hitRecord record;
    vector4 ray;
    vector3 v0;
    
    // !!! This algorithm is flawed !!! Black volume will be reconstructed in empty space !!! //
    // This algorithm will need to be reimplemented!!! //
    // Iterate through every voxel
    for(int z = -z_n/2; z < z_n/2+1; z++){
        for(int x = 0; x < w; x++){
            for(int y = 0; y < h; y++){
                double tR = 0; double tG = 0; double tB = 0; double tA = 0;
                int counter_R = 0;
                iRecord.freeRecord();
                // Compute the rays that will hit the voxel
                iRecord = inverseHittable(x, y, z, fs);
                while(iRecord.next(ray)){
                    // Compute the expected ray value from the estimated volume
                    double R = 0; double G = 0; double B = 0; double A = 0;
                    int counter_v = 0;
                    record.freeRecord();
                    // Obtain the voxels that will be hit by the ray
                    record = hittable(ray.x, ray.y, ray.i, ray.j, z_n);
                    while(record.next(v0)){
                        // Compute the expected ray value by averaging over the voxel hitted by the ray
                         _stackPixel(stacks[stack_index(v0.z, z_n)], v0.x, v0.y, R, G, B, A, counter_v++);
                    }
                    int mR = 0; int mG = 0; int mB = 0; int mA = 0;
                    angleMap(ray.x, ray.y, ray.i, ray.j, mR, mG, mB, mA, fs, top_map, x_map, y_map);
                    // Compute the loss by subtracting the actual ray value with the reconstructed ray value
                    incAvg(tR, tG, tB, tA, ((double)mR-R), ((double)mG-G), ((double)mB-B), ((double)mA-A), counter_R++);
                    hit_counter += counter_v;
                }
                // Compensate for average losses for each voxel
                assignPixel(new_stack[stack_index(z, z_n)], x, y, (int)round(tR*lr), (int)round(tG*lr), (int)round(tB*lr), (int)round(tA*lr), DW_MODE_ADDITION);
                hit_counter += counter_R;
            }
        }
    }

    // Free the memory to prevent memory leak
    iRecord.freeRecord();
    record.freeRecord();

    //printf("Total hit count with hittable: %d\n", hit_counter);
    deleteStack(stacks, z_n);
    return new_stack;
}

std::chrono::steady_clock frame_clock;
std::chrono::time_point<std::chrono::steady_clock> frameStart = frame_clock.now();
std::chrono::time_point<std::chrono::steady_clock> programStart = frame_clock.now();

int frame_end(){
    int runTime = std::chrono::duration_cast<std::chrono::milliseconds>(frame_clock.now()-frameStart).count();
    return runTime;
}

int program_end(){
    int runTime = std::chrono::duration_cast<std::chrono::milliseconds>(frame_clock.now()-programStart).count();
    return runTime;
}

void frame_start(){
    frameStart = frame_clock.now();
}

int main(int argc, char **argv){
    printf("Input number of stack (odd): ");
    int z_n;
    scanf("%d", &z_n);
    printf("Input number of iterations: ");
    int i_n;
    scanf("%d", &i_n);
    printf("Input folder path: ");
    char folder[256];
    scanf("%s", folder);
    
    printf("\n>>>\nGenerating stack = %d at [./%s/] for %d iterations\n", z_n, folder, i_n);
    
    int fs;
    stbi_uc **image_stack = NULL;
    stbi_uc *top_map = NULL;
    stbi_uc **x_map = NULL;
    stbi_uc **y_map = NULL;
    
    if(!load_config(folder, fs)){
        printf("Failed to load config!");
        return 0;
    }

    printf("Image of height=%d, width=%d, sampling frequency of %d\n", h, w, fs+1);
    if(!load_all_maps(folder, top_map, x_map, y_map, fs)){
        printf("Failed to load angle maps!\n");
        return 0;
    }

    inverseTransformAllMap(top_map, x_map, y_map, fs);
    // allMap2Monochrome(top_map, x_map, y_map, fs);
    saveAllMap(folder, (char*)"inverse", top_map, x_map, y_map, fs);

    printf("Saving inverse maps\n");

    image_stack = new stbi_uc*[z_n];
    for(int i = 0; i<z_n; i++){
        image_stack[i] = (stbi_uc*)calloc(h*w*4, 1);
    }
    
    printf("Initializing image stack\n");
    init_stacks(image_stack, top_map, x_map, y_map, fs, z_n);

    //inverseImageStack(image_stack, z_n);
    printf("Saving refocusing stack\n");
    saveStack(folder, (char*)"refocus/t_0", image_stack, z_n);

    // Learning rate of iterate
    double lr = 0.05;

    frame_start();
    int runtime;

    // Calculate remaining time every itr_chk_num iterations
    int itr_chk_num = 10;
    
    for(int i = 1; i<i_n; i++){
        image_stack = stackIterate(image_stack, top_map, x_map, y_map, fs, z_n, lr);
        char temp_name[256];
        sprintf(temp_name, "refocus/t_%d", i);
        if(i%itr_chk_num == 0){
            saveStack(folder, temp_name, image_stack, z_n);
            runtime = frame_end();
            printf("Last %d iterations took %d seconds, estimated remaining time: %d minutes\n", itr_chk_num, runtime/1000, (i_n-i)/itr_chk_num*runtime/1000/60);
            frame_start();
            printf("Finished %d-th iteration\n", i);
        }
        //printf("Finished %d-th iteration\n", i);
    }

    printf("Program took %d minutes in total\n", program_end()/1000/60);
}