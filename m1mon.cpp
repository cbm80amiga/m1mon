/*

Simple wrapper for the macOS 'powermetrics' command to monitor Apple Silicon performance
(c) 2021 Pawel A. Hernik

Tested on M1 MacBook Air, should work on M1 Pro and Max machines as well
Requires Xcode and ncurses to compile

Compile:
g++ -lncurses -o m1mon m1mon.cpp

+- Machine: MacBookAir10,1 ------ OS: 21A559 ------------------[09:34:26]-[0033]--+
|                                                                                 |
| CPU 0:  1860 of 2064 MHz   OOOOOOOOOOOOOO...............................  32.6% |
| CPU 1:  1857 of 2064 MHz   OOOOOOOOOOOOOO...............................  31.2% |
| CPU 2:  1850 of 2064 MHz   OOOOOOOOOOO..................................  25.6% |
| CPU 3:  1855 of 2064 MHz   OOOOOOOOO....................................  21.7% |
| CPU 4:  3201 of 3204 MHz   OOOOOOOOOOOOOOOOOOOOOOOOOO...................  59.0% |
| CPU 5:  3003 of 3204 MHz   OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO....  91.8% |
| CPU 6:  2503 of 3204 MHz   OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO...  95.4% |
| CPU 7:  3203 of 3204 MHz   OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO....  93.2% |
| GPU      711 of 1278 MHz   OOOOOOOOOOOOOOOOOOOOOOOOOOOOOO...............  67.1% |
| E-Clust 1859 of 2064 MHz   OOOOOOOOOOOOOOOOOOOOOOOOOO...................  58.8% |
| P-Clust 3117 of 3204 MHz   OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO...  93.4% |
|              cur       max                                                      |
| Package:    7651 mW  15186 mW                                                   |
| CPU:        4188 mW  14734 mW                                                   |
| GPU:        1874 mW   2363 mW                                                   |
| ANE:           0 mW      0 mW                                                   |
| DRAM:        927 mW    953 mW                                                   |
| E-Cluster:   246 mW   1255 mW                                                   |
| P-Cluster:  3942 mW  13495 mW                                                   |
|                                                                                 |
+---- Ctrl+C to exit -------------------------------------------------------------+

*/

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <map>
#include <string>

#define MAX(a, b) (a > b ? a : b)

#define DEBUG 0


int powerY = 12;
int powerX = 2;
int cpuY = 2;
int cpuX = 2;

#define MAX_CPU 32
char buf[501],*p;
std::map<std::string, unsigned int> powerMap,powerMaxMap;
std::map<std::string, unsigned int>::iterator powerIter;
int cpuLast=0;
int cpuFreq[MAX_CPU]={0};
int cpuFreqMax[MAX_CPU]={0};
float cpuActive[MAX_CPU]={0};
int gpuFreq=0;
int gpuFreqMax=0;
float gpuActive=0;
int eFreq=0;
int eFreqMax=0;
float eActive=0;
int pFreq=0;
int pFreqMax=0;
int p0Freq=0;
int p0FreqMax=0;
int p1Freq=0;
int p1FreqMax=0;
int p2Freq=0;
int p2FreqMax=0;
int p3Freq=0;
int p3FreqMax=0;
float pActive=0;
float p0Active=0;
float p1Active=0;
float p2Active=0;
float p3Active=0;
unsigned int cnt=0;
int sx=0,sy=0;
int sxOld=0,syOld=0;
int reqx=42,reqy=21;
char *pMachine=0, *pOS=0, timeStamp[9]={0};
int showClust = 1;

// CPU 0 frequency: 1336 MHz
// CPU 0 active residency:  20.79% (600 MHz: .23% 972 MHz:  11% 1332 MHz: 2.6% 1704 MHz: 2.7% 2064 MHz: 4.4%)
// GPU active residency:   4.50% (396 MHz: 4.5% 528 MHz:   0% 720 MHz:   0% 924 MHz:   0% 1128 MHz:   0% 1278 MHz:   0%)
// CPU Power: 128 mW

int parseActive(char *p, float *pAct, int *pMax)
{
    if(strstr(p,"active residency")) {
        p = strstr(p,":");
        if(p) {
            *pAct=atof(p+2);
            if(strlen(p)>10) {
                p = strstr(p+strlen(p)-10,"MHz");
                if(p) *pMax=atoi(p-5);
            }
        }
        return 1;
    }
    return 0;
}

void drawBar(int y, int x, int w, float proc)
{
    int col = proc>80 ? 3 : ( proc>60 ? 2 : 1 );
    attron(COLOR_PAIR(col));
    int i,wd = (w-x)*proc/100.0;
    for(i=0;i<wd;i++) mvprintw(y,x+i,"O");
    attroff(COLOR_PAIR(col));
    for(;i<w-x;i++) mvprintw(y,x+i,".");
}

int colorThrottle(float active, int freq, int freqMax)
{
    if(active<90) return 1;
    int throttle = 100*freq/freqMax;
    return throttle<80 ? 3 : ( throttle<95 ? 2 : 1 );
}

void displayLine(int y,int x, float active, int freq, int freqMax)
{
    int col = colorThrottle(active,freq,freqMax);
    attron(COLOR_PAIR(col));
    mvprintw(y,x,"%4d",freq);
    attroff(COLOR_PAIR(col));
    mvprintw(y,x+5,"of %4d MHz",freqMax);
    mvprintw(y,sx-8,"%5.1f%%",active);
    drawBar(y,x+18,sx-9,active);
}

void displayAll()
{
    getmaxyx(stdscr,sy,sx);
    reqy = cpuLast+2+5+powerMap.size();
    if(sx<reqx || sy<reqy) {
        clear();
        mvprintw(0,0,"Enlarge window to %dx%d, current %dx%d\n",reqx,reqy,sx,sy);
        refresh();
        return;
    }
    if(sx!=sxOld || sy!=syOld) {
        clear();
        wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');
        if(sx>48 && pMachine) mvprintw(0,2," Machine: %s ",pMachine);
        if(sx>65 && pOS) mvprintw(0,33," OS: %s ",pOS);
        mvprintw(sy-1,4," Ctrl+C to exit ");
        sxOld = sx;
        syOld = sy;
    }

    int i=0,maxlen=0;
    for(i=0;i<=cpuLast;i++) {
        mvprintw(cpuY+i,cpuX,"CPU %d:",i);
        displayLine(cpuY+i,cpuX+9,cpuActive[i],cpuFreq[i],cpuFreqMax[i]);
    }
    mvprintw(cpuY+i,cpuX,"GPU:");
    displayLine(cpuY+i,cpuX+9,gpuActive,gpuFreq,gpuFreqMax);
    i++;
    if(showClust && sy>=reqy+2) {
        mvprintw(cpuY+i,cpuX,"E-Clust:");
        displayLine(cpuY+i,cpuX+9,eActive,eFreq,eFreqMax);
        i++;
        if(pFreq>0) {
            mvprintw(cpuY+i,cpuX,"P-Clust:");
            displayLine(cpuY+i,cpuX+9,p0Active,p0Freq,p0FreqMax);
            i++;
        }
        if(p0Freq>0) {
            mvprintw(cpuY+i,cpuX,"P0-Clust:");
            displayLine(cpuY+i,cpuX+9,p0Active,p0Freq,p0FreqMax);
            i++;
        }
        if(p1Freq>0) {
            mvprintw(cpuY+i,cpuX,"P1-Clust:");
            displayLine(cpuY+i,cpuX+9,p1Active,p1Freq,p1FreqMax);
            i++;
        }
        if(p2Freq) {
            mvprintw(cpuY+i,cpuX,"P2-Clust:");
            displayLine(cpuY+i,cpuX+9,p2Active,p2Freq,p2FreqMax);
            i++;
        }
        if(p3Freq) {
            mvprintw(cpuY+i,cpuX,"P3-Clust:");
            displayLine(cpuY+i,cpuX+9,p3Active,p3Freq,p3FreqMax);
            i++;
        }
    }

    powerY = cpuY+i; i=0;
    for ( powerIter = powerMap.begin(); powerIter != powerMap.end(); ++powerIter )
        if(powerIter->first.length()>maxlen) maxlen = powerIter->first.length();

    // show Package, CPU, GPU then the rest
    mvprintw(powerY+i,powerX+13,"cur       max");
    i++;
    mvprintw(powerY+i,powerX,"Package:");
    mvprintw(powerY+i,powerX+maxlen+2,"%5d mW",powerMap["PACKAGE"]);
    mvprintw(powerY+i,powerX+maxlen+2+10,"%5d mW",powerMaxMap["PACKAGE"]);
    i++;
    mvprintw(powerY+i,powerX,"CPU:");
    mvprintw(powerY+i,powerX+maxlen+2,"%5d mW",powerMap["CPU"]);
    mvprintw(powerY+i,powerX+maxlen+2+10,"%5d mW",powerMaxMap["CPU"]);
    i++;
    mvprintw(powerY+i,powerX,"GPU:");
    mvprintw(powerY+i,powerX+maxlen+2,"%5d mW",powerMap["GPU"]);
    mvprintw(powerY+i,powerX+maxlen+2+10,"%5d mW",powerMaxMap["GPU"]);
    i++;
    for ( powerIter = powerMap.begin(); powerIter != powerMap.end(); ++powerIter ) {
        if(powerIter->first!="PACKAGE" && powerIter->first!="CPU" && powerIter->first!="GPU") {
            mvprintw(powerY+i,powerX,"%s:",powerIter->first.c_str());
            mvprintw(powerY+i,powerX+maxlen+2,"%5d mW",powerIter->second);
            mvprintw(powerY+i,powerX+maxlen+2+10,"%5d mW",powerMaxMap[powerIter->first]);
            i++;
        }
    }

    mvprintw(0,sx-10-10,"[%s]-[%04d]",timeStamp,cnt);
    refresh();
}


int main(int argc, char *argv[])
{
    int interv = 1000;
    if(argc>2 && !strcmp(argv[1],"-i")) interv = atoi(argv[2]); else
    if(argc>1) {
        if(!strncmp(argv[1],"-i",2))
            interv = *(argv[1]+2) ? atoi(argv[1]+2) : 1000;
        else {
            printf("Usage: m1mon [-i]\n");
            exit(0);
        }
    }
    snprintf(buf,500,"sudo powermetrics --samplers=cpu_power,gpu_power -i %d",interv);

#if DEBUG==1
    FILE *f = popen("cat pm-dump","r");
#else
    FILE *f = popen(buf,"r");
#endif
    if(!f) {
        printf( "Can't open powermetrics pipe!\n");
        exit(1);
    }
    initscr();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    noecho();
    wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');
    
    fgets(buf,500,f);
    buf[strlen(buf)-1] = 0;
    while(!feof(f)) {
        if(strstr(buf,"Sampled system")) {
            if((p=strstr(buf,":"))) {
                if(p) strncpy(timeStamp,p-2,8);
            }
            cnt++;
#if DEBUG==1
            usleep(200000);
#endif
        }
        if(strstr(buf,"OS ver") && !pOS) {
            p = strstr(buf,":");
            if(p) pOS = strdup(p+2);
        } else
        if(strstr(buf,"Machine model") && !pMachine) {
            p = strstr(buf,":");
            if(p) pMachine = strdup(p+2);
        } else
        if((p=strstr(buf,"Power:"))) {
            *(p-1)=0;
            std::string pow = buf;
            powerMap[pow] = atoi(p+7);
            powerMaxMap[pow] = MAX(powerMaxMap[pow],powerMap[pow]);
        }
        if((p=strstr(buf,"Combined Power (CPU + GPU + ANE):"))) {
            //*(p-1)=0;
            std::string pow = "PACKAGE";
            //printf("pow= %s",pow.c_str());
            p = strstr(p,":");
            if (p) {
                powerMap[pow] = atoi(p+2);
                powerMaxMap[pow] = MAX(powerMaxMap[pow],powerMap[pow]);
            }
        } else
        if( (p=strstr(buf,"CPU")) && isdigit(*(p+4)) ) {
            int cpu = atoi(p+4);
            if(cpuLast<cpu && cpu<MAX_CPU) cpuLast=cpu;
            if(strstr(p,"frequency")) {
                p = strstr(p,":");
                if(p) cpuFreq[cpu]=atoi(p+2);
            } else
                parseActive(p,&cpuActive[cpu],&cpuFreqMax[cpu]);
        } else
        if((p=strstr(buf,"E-Cluster")) && showClust) {
            if(strstr(p,"active frequency")) {
                p = strstr(p,":");
                if(p) eFreq=atoi(p+2);
            } else
               parseActive(p,&eActive,&eFreqMax);
        } else
        if((p=strstr(buf,"P-Cluster")) && showClust) {
            if(strstr(p,"active frequency")) {
                p = strstr(p,":");
                if(p) pFreq=atoi(p+2);
            } else
                parseActive(p,&pActive,&pFreqMax);
        } else
        if((p=strstr(buf,"P0-Cluster")) && showClust) {
            if(strstr(p,"active frequency")) {
                p = strstr(p,":");
                if(p) p0Freq=atoi(p+2);
            } else
               parseActive(p,&p0Active,&p0FreqMax);
        } else
        if((p=strstr(buf,"P1-Cluster")) && showClust) {
            if(strstr(p,"active frequency")) {
                p = strstr(p,":");
                if(p) p1Freq=atoi(p+2);
            } else
                parseActive(p,&p1Active,&p1FreqMax);
        } else
        if((p=strstr(buf,"P2-Cluster")) && showClust) {
            if(strstr(p,"active frequency")) {
                p = strstr(p,":");
                if(p) p2Freq=atoi(p+2);
            } else
                parseActive(p,&p2Active,&p2FreqMax);
        } else
        if((p=strstr(buf,"P3-Cluster")) && showClust) {
            if(strstr(p,"active frequency")) {
                p = strstr(p,":");
                if(p) p3Freq=atoi(p+2);
            } else
                parseActive(p,&p3Active,&p3FreqMax);
        } else
        if((p=strstr(buf,"GPU"))) {
            if(strstr(p,"active frequency")) {
                p = strstr(p,":");
                if(p) gpuFreq=atoi(p+2);
            } else
               parseActive(p,&gpuActive,&gpuFreqMax);

            if(gpuFreqMax>0)
                displayAll();
        }
        fgets(buf,500,f);
        buf[strlen(buf)-1] = 0;
    }
    pclose(f);

#if DEBUG==1
    getchar();
#endif
    endwin();
}
