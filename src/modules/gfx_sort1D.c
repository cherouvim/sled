// Sorting visualizations

#include <types.h>
#include <matrix.h>
#include <timers.h>
#include <random.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define FPS 300
#define FRAMETIME (T_SECOND / FPS)
#define FRAMES (RANDOM_TIME * FPS) * 10

static int modno;
static ulong frame;
static ulong nexttick;
static int mx, my;

static int * data;

static int sorting_algorithm=12;
static int __rval=0;

static int draw_style=0;
static int highlight_style=1;

// highlighting
static int h1;
static int h2;


// pseudo jump and link
static int __yield_value;
#define CONTINUE(x)         \
    if (__yield_value == x) \
        goto label##x;
#define YIELD(x)        \
    __yield_value = x;  \
    return 0;           \
    label##x:

#define swap(a,b)       \
    int __tmp = data[a];\
    data[a] = data[b];  \
    data[b] = __tmp;

#define cmp_swap(a,b)       \
    h1 = a; h2 = b;         \
    if (data[a] < data[b]){ \
        swap(a,b);          \
        inversions++;       \
    }


RGB colorwheel(int angle) {
    angle = angle % 1536;
    int t = (angle / 256)%6;
    int v = angle % 256;
    switch (t) {
    case 0: return RGB(255,v,0);
    case 1: return RGB(255-v,255,0);
    case 2: return RGB(0,255,v);
    case 3: return RGB(0,255-v,255);
    case 4: return RGB(v,0,255);
    case 5: return RGB(255,0,255-v);
    }
}

// sorting algorithm internals
static int i,j;
static int inversions;
static int step,stage,stride;
static int iMin;
static int start,end,child,root,swapable;
static int last;
static int partner,merge;
static int log_size;

static int pairwise_sorting_net(){
    CONTINUE(1);
    CONTINUE(2);
    log_size = 0;
    for (int i=1;i<mx;i*=2)log_size++;
    //printf("ls:%d\n",log_size);
    for (stage = 0;stage<log_size;stage++){
        for (i = 0;i<mx;i++){
            int box_size = 1<<stage;
            int box_start = (i>>(stage+1))<<(stage+1);
            int step_size = 1<<stage;
            partner = (i-step_size < box_start) ? i + step_size : i - step_size;
            //printf("    %d <-> %d b:%d-%d (%d) >%d\n",i,partner,box_start,box_start+box_size-1,box_size,step_size);
            if (partner > i && partner < mx) {
                cmp_swap(i,partner);
                YIELD(1);

            }
        }
    }
    //printf("logsize %d\n",log_size);
    for (stage=log_size-2;stage>=0;stage--){
        //printf("====(%d)====\n",stage);
        for (step=log_size-1-stage;step>=1;step--){
            //printf("--=(%d:%d)=--\n",stage,step);
            for (i = 1<<stage;i+stride<mx;i+=(2<<stage)){
                stride = ((1<<step)-1)<<stage;
                //printf("-(%d:%d) @%d [%d]-\n",stage,step,i,stride);
                for (j=0;j<(1<<stage);j++){
                    if (i+j+stride >= mx) continue;
                    cmp_swap(i+j,i+j+stride);
                    //printf(" %d <-> %d (%d:%d)\n",i+j,i+j+stride);
                    YIELD(2);
                }
            }
        }
    }
    return 1;
}
static int bitonic_sort(){
    CONTINUE(1);
    log_size = 0;
    for (int i=1;i<mx;i*=2)log_size++;
    //printf("ls:%d\n",log_size);
    for (stage = 1;stage<log_size+1;stage++){
        for (step=1;step<=stage;step++){
            for (i = 0;i<mx;i++){
                int box_size = 1<<(stage-step+1);
                int box_start = (i>>(stage-step+1))<<(stage-step+1);
                if (step == 1) partner = box_start + box_size - (i%box_size)-1;
                else {
                    partner = (i-box_size/2 < box_start) ? i + box_size/2 : i - box_size/2;
                }
                //printf("    %d <-> %d b:%d-%d (%d)\n",i,partner,box_start,box_start+box_size,box_size);

                if (partner > i && partner < mx) {
                    cmp_swap(i,partner);
                    YIELD(1);

                }
            }
        }
    }
    return 1;
}

static int odd_even_mergesort(){
    CONTINUE(1);

    log_size = 0;
    for (int i=1;i<mx;i*=2)log_size++;
    //printf("ls:%d\n",log_size);
    for (stage = 1;stage<log_size+1;stage++){
        for (step=1;step<=stage;step++){
            //printf("%d:%d %d\n",stage,step,1<<(stage-step));
            for (i = 0;i<mx;i++){
                if (step == 1) partner = i ^ (1 << (stage-1));
                else {
                    int stride = 1 << (stage-step);
                    int box = (i >> stage) << stage;
                    int box_low = box + stride;
                    int box_hig = box + (1<<stage)-stride-1;
                    if (i < box_low || i >= box_hig) partner = i;
                    else partner = ((i/stride-box/stride)%2) ? i + stride : i - stride;
                    //printf("    %d <-> %d b:%d-%d (%d-%d) st:%d\n",i,partner,box,box+(1<<stage)-1,box_low,box_hig,stride);
                }
                if (partner > i && partner < mx) {
                    cmp_swap(i,partner);
                    YIELD(1);

                }
            }
        }
    }
    return 1;
}

static int coctail_shaker_sort(){
    CONTINUE(1);
    CONTINUE(2);
    start = 0;
    end = mx-1;
    while(1) {
        inversions = 0;
        for (i=start;i<end;i++) {
            if (data[i] < data[i+1]) last = i;
            cmp_swap(i,i+1);
            YIELD(1);
        }
        end = last;
        if (!inversions) break;
        inversions = 0;
        for (i=end-1;i>=start;i--){
            if (data[i] < data[i+1]) last = i;
            cmp_swap(i,i+1);
            YIELD(2);
        }
        start = last;
        if (!inversions) break;
    }
    return 1;
}

static int heap_sort(){
    CONTINUE(1);
    CONTINUE(2);
    //CONTINUE(3);
    CONTINUE(4);
    CONTINUE(5);

    // heapify
    for (start=mx/2; start>=0;start--) {
        end = mx-1;
        root = start;
        // sift down
        while (2*root+1<=end){
            child = 2*root+1;
            swapable = root;
            h1 = swapable; h2 = child; YIELD(1);
            if (data[swapable] > data[child]) swapable = child;
            if (child +1 <= end) {
                h1 = swapable; h2 = child+1; YIELD(2);
                if (data[swapable] > data[child+1]) swapable = child+1;
            }
            if (swapable == root) break;
            else {
                swap(swapable,root);
                root = swapable;
            }
        } 
    }
    // heapsort
    for (end=mx-1;end > 0;) {
        swap(end,0);
        //h1=0;h2=end;YIELD(3);
        end--;
        start = 0;
        root = start;
        // sift down
        while (2*root+1<=end){
            child = 2*root+1;
            swapable = root;
            h1 = swapable; h2 = child; YIELD(4);
            if (data[swapable] > data[child]) swapable = child;
            if (child +1 <= end) {
                h1 = swapable; h2 = child+1; YIELD(5);
                if (data[swapable] > data[child+1]) swapable = child+1;
            }
            if (swapable == root) break;
            else {
                swap(swapable,root);
                root = swapable;
            }
        } 
    }
    return 1;
}


static int bubblesort() {
    CONTINUE(1);
    for (j = mx-1; j>0; j--) {
        inversions = 0;
        for (i = 0; i<j; i++) {
            cmp_swap(i,i+1);
            YIELD(1);
        }
        if (inversions == 0) {
            return 1;
        }
    }
    return 1;
}

static int selection_sort2() {
    CONTINUE(1);
    for (j = 0; j < mx-1; j++) {
        for (i = j+1; i < mx; i++) {
            cmp_swap(j,i);
            YIELD(1);
        }
    }
    return 1;
}

static int selection_sort3() {
    CONTINUE(1);
    CONTINUE(2);
    for (j = 0; j < (mx-1)/2; j++) {
        cmp_swap(j+1,mx-1-j);
        for (i = j+1; i < mx-1-j; i++) {
            cmp_swap(j,i);
            YIELD(2);
            cmp_swap(i,mx-1-j);
            YIELD(1);
        }
    }
    return 1;
}

static int selection_sort() {
    CONTINUE(1);
    CONTINUE(2);
    for (j = 0; j < mx-1; j++) {
        iMin = j;
        h1 = iMin;
        for (i = j+1; i < mx; i++) {
            h2 = i;
            YIELD(1);
            if (data[i] > data[iMin]) {
                iMin = i;
                h1 = iMin;
            }
        }
        h2 = j;
        swap(j, iMin);
        YIELD(2);
    }
    return 1;
}

// Like it's inverse bubblesort, but worse
static int tournament_sort(){
    CONTINUE(1);
    for (j=0;j<mx-1;j++){
        for (stride = 1;stride +j < mx;stride *=2){
            for (i=j;i+stride<mx;i+=stride*2){
                cmp_swap(i,i+stride);
                YIELD(1);
            }
        }
    }
    return 1;
}

static int insertion_sort() {
    CONTINUE(1);
    for (i=1; i<mx; i++) {
        for (j=i; j>0&&data[j-1]<data[j]; j--) {
            cmp_swap(j-1,j);
            YIELD(1);
        }
    }
    return 1;
}

static int comb_sort() {
    CONTINUE(1);
    step = mx;
    do {
        inversions = 0;
        step = (int)floor(step/1.3);
        if (step == 0) step=1;
        for (i=0; i+step<mx; i++) {
            cmp_swap(i,i+step);
            YIELD(1);
        }
    } while (step > 0 && inversions);
    return 1;
}

const static int shellsort_gaps[] = {701,301,132,57,23,10,4,1}; 

static int shell_sort() {
    CONTINUE(1);
    for(stage=0;stage<8;stage++){
        step = shellsort_gaps[stage];
        for (i=1; i<mx; i++) {
            for (j=i; j-step>=0&&data[j-step]<data[j]; j-=step) {
                cmp_swap(j-step,j);
                YIELD(1);
            }
        }
    }
    return 1;
}



static int sort() {
    switch (sorting_algorithm) {
    case 0: return bubblesort(); // mx*mx/2
    case 1: return comb_sort(); // mx*log(mx)*2
    case 2: return insertion_sort(); // mx*mx/4
    case 3: return selection_sort(); // mx*mx/2
    case 4: return heap_sort();       // mx*log(mx)*1.5
    case 5: return coctail_shaker_sort(); //mx*mx /3 
    case 6: return odd_even_mergesort(); //mx*log(mx)*1.5
    case 7: return bitonic_sort(); //mx*log(mx)*1.5
    case 8: return pairwise_sorting_net(); //mx&log(mx)*15
    case 9: return shell_sort();
    case 10: return selection_sort2();
    case 11: return selection_sort3();
    case 12: return tournament_sort(); //mx*mx/2*log(mx)
    default: return bubblesort();
    }
}


void randomize_and_reset(){
    data[0] = 1;
    for (int i=1; i<mx; i++) {
        int other = randn(i);
        data[i] = data[other];
        data[other] = i+1;
    }
    __yield_value = -1;
    sorting_algorithm = randn(12);
    draw_style = randn(1);
    highlight_style = randn(2)+1;
#if 0
    int expected_runtime = 0;
    while (sort() == 0) expected_runtime++;
    p rintf("\nRuntime Algo %d: %d frames\n",sorting_algorithm,expected_runtime);
    data[0] = 1;
    for (int i=1; i<mx; i++) {
        int other = randn(i);
        data[i] = data[other];
        data[other] = i+1;
    }
    __yield_value = -1;
#endif
    frame = 0;
}

static void draw_dots(){
    matrix_clear();
    int x1,x2,y1,y2;

    if (highlight_style & 1)
    for (int i = 0;i<2;i++){
        int hx;
        if (i) hx = h1; else hx = h2;
        if (hx < 0) continue;
        int hy = (data[hx]-1)*my/mx;
        if (hx <= 1) x1 = 0; else x1 = hx-1;
        if (hx >= mx-2) x2 = mx-1; else x2 = hx+1;
        if (hy <= 1) y1 = 0; else y1 = hy-1;
        if (hy >= my-2) y2 = mx-1; else y2 = hy+1;
        for (int x=x1;x<=x2;x++){
            for (int y=y1;y<=y2;y++){
                matrix_set(x,y,RGB(255,255,255));
            }
        }
    }
    if (highlight_style & 2)
    if (h1 >= 0 && h2 >= 0){
        int x1=(h1<h2)?h1:h2;
        int x2=(h1<h2)?h2:h1;
        int y1 = (data[h1]-1)*my/mx;
        int y2 = (data[h2]-1)*my/mx;
        for (int x=x1;x<=x2;x++){
            matrix_set(x,y1,RGB(80,80,80));
            matrix_set(x,y2,RGB(80,80,80));
        }
    }
    for (int x=0; x<mx; x++) {
        int y = (data[x]-1)*my/mx;
        if (y < 0) y=0;
        if (y >= my) y=my-1;
        matrix_set(x,y,colorwheel(data[x]*1000/mx));
    }
}

static void draw_bars(){
    matrix_clear();
    if (h1 >= 0 || h2 >= 0) {
        for (int y=0; y<my; y++) {
            if (h1 >= 0) matrix_set(h1,y,RGB(80,80,80));
            if (h2 >= 0) matrix_set(h2,y,RGB(80,80,80));
        }
    }
    for (int x=0; x<mx; x++) {
        int range = (data[x])*my/mx;
        if (range >= my) range = my;
        if (range < 1) range = 1;
        for (int y=my-1; y>range-2; y--) {
            RGB color = colorwheel(data[x]*1000/mx);
            matrix_set(x,y,color);
        }
    }
}

static void draw_select(){
    switch (draw_style){
        default:
        case 0: return draw_bars();
        case 1: return draw_dots();
    }
}


int draw(int argc, char* argv[]) {
    if (__rval==1){
        randomize_and_reset();
        __rval=0;
    }
    __rval = sort();

    matrix_render();
    draw_select();

    if (__rval > 0) {
        //printf("\nRan for %d frames\n",frame);
        return 1;
    }
    frame++;
    nexttick += FRAMETIME;
    timer_add(nexttick, modno, 0, NULL);
    return 0;
}

void reset(void) {
    randomize_and_reset();
    nexttick = udate();
}

int init(int moduleno, char* argstr) {
    mx = matrix_getx();
    my = matrix_gety();
    data = malloc(mx * sizeof(int));
    modno = moduleno;
    frame = 0;
    return 0;
}

int deinit() {
    free(data);
    return 0;
}
