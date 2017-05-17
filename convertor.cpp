#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>
#include <QDataStream>
#include <QFile>
#include <QString>
#include <QFileDialog>
#include <QApplication>

typedef struct channelData {
    uint16_t energy;
    int64_t time;
    } channelData;

QDataStream out;
QFile outFile;
std::ifstream dataFile;
uint16_t nofBufferWords, moduleNumber, formatD;
uint16_t timeHi,timeMi,timeLo;
std::vector<std::vector<struct channelData> > buffer;
int wordsRead;
std::vector<bool> readChannel;
std::vector<uint64_t> readPointer;
std::vector<uint16_t> cache;
uint16_t written;
QStringList filenames;
uint16_t runnum=0;
int z;

void removeHeader()
{
    char word;
    std::string line;
    std::string textMarker("USELOCALTIME");
    std::size_t found;
    int counter=0;
    do{
    std::getline(dataFile,line);
    found=line.find(textMarker);
    if (found!=std::string::npos)
        counter++;
    }while(counter<15);

    std::getline(dataFile,line);
    for(int i=0;i<13;i++)
        dataFile.read(&word,1);

}

uint16_t readWord()
{
    char word;
    uint8_t value[2];
    dataFile.read(&word,1);
    value[0]=(int)word;
    dataFile.read(&word,1);
    value[1]=(int)word;
    return value[0]|value[1]<<8;
}

void readBufferHeader()
{
    nofBufferWords=readWord();
    moduleNumber=readWord();
    formatD=readWord();
    timeHi=readWord();
    timeMi=readWord();
    timeLo=readWord();
}

int getChannel(int chan)
{
    if(moduleNumber==1416)
    {
        if(chan==1)
            return 3;
         else if(chan==3)
            return 2;
         else return 1;
    }
    else if(moduleNumber==1415)
        return (12+chan);
    else if(moduleNumber==1413)
        return (8+chan);
    else return (4+chan);
}

int readEvent()
{
    uint16_t pattern,evTimeHi,evTimeLo,channelNo;
    uint64_t tempWord;
    int mask, masked,read;
    read=0;
    pattern=readWord();
    evTimeHi=readWord();
    evTimeLo=readWord();
    tempWord=timeHi;
    tempWord=tempWord<<16;
    tempWord=tempWord+evTimeHi;
    tempWord=tempWord<<16;
    struct channelData temp;

    for(int i=0;i<4;i++)
    {
        mask = 1<<i;
        masked=pattern & mask;
        if(masked!=0)
        {
            channelNo=getChannel(i);
            temp.time=readWord();
            temp.energy=readWord();
            temp.time=temp.time+tempWord;
            buffer[channelNo].push_back(temp);
            read++;
        }
    }
    return (3+2*read);
}

void openNewFile(){
    // If necessary, close old file
    if(outFile.isOpen()) {
        outFile.close();
    }
    // Open new file
    QString outputFile= filenames[z];
    outputFile.chop(4);
        outFile.setFileName(outputFile);
        outFile.open(QIODevice::WriteOnly);

     out.setDevice(&outFile);
     out.setByteOrder(QDataStream::LittleEndian);
        std::vector<uint16_t> fhead;
        QString aux, aux1, aux2, aux3, comments;
        int l,w=0;
        fhead.resize(16);
        comments.resize(16348);

        for(int k=0;k<16348;k++)
            comments[k]=0;

        for(int k=0;k<16;k++)
            fhead[k]=0;

        fhead[0]= 16;
        fhead[1]= 0;
        fhead[2]= runnum;
        fhead[3]= 18248;
        fhead[4]= 16;

        for(int k=0;k<16;k++)
            out<<fhead[k];

        //l=filePrefix.size();

        //for(int k=w;k<w+l;k++)
       //     comments[k]=filePrefix[k];
        //w+=l;

        comments[w]= 0;
        w+=2;

         aux="| Header (1 param) |";
        l=aux.size();
        for(int k=w;k<w+l;k++)
            comments[k]=aux[k-w];;
        w+=l;

        aux1="DetType";
        aux2=" det , 2 param)";
        aux3="(";
        for(uint32_t j=1;j<=1;j++)
        {
            l=aux1.size();
            for(int k=w;k<w+l;k++)
               comments[k]=aux1[k-w];
            w+=l;
            comments[w]= j;
            comments[w+1]=aux3[0];
            comments[w+2]= 3;
            w+=3;
            l=aux2.size();
            for(int k=w;k<w+l;k++)
               comments[k]=aux2[k-w];
            w+=l;
        }
        out<<comments.toUtf8();

    for(int k=0;k<8176;k++)
        cache[k]=0;
    written=0;
}

void writeToFile(){
    if(outFile.isOpen()) {
            uint16_t *shead;
            shead=( u_int16_t *) calloc(16, sizeof(u_int16_t *));
            uint16_t runnum=0;
            shead[0]= 16;
            shead[1]= 0;
            shead[2]= runnum;
            shead[3]= 18264;
            shead[4]= 16;
            for(int k=0;k<16;k++)
                out<<shead[k];
            for(int k=0;k<8176;k++)
            out<<cache[k];

            for(int k=0;k<8176;k++)
                cache[k]=0;

            written=0;

    }
    else  printf("File is not open for writing.\n");

}

void writeToCache(std::vector<bool> marker)
{
    uint16_t header = 0xF002;
    uint16_t zero = 0;
    int64_t timeSum=0;
    int channelsRead=0;
    int sumElements=0;

    for(int k=4;k<16;k++)
        if(marker[k])
            channelsRead++;

    if(written+3+channelsRead*4>8176)
        writeToFile();

    if(channelsRead!=0)
    {
        header+=5*channelsRead;
        cache[written++] = header;
        cache[written++] = zero;
        cache[written++] = channelsRead;

        for(int i=1;i<16;i++)
            if(marker[i])
            {
                timeSum+=buffer[i][readPointer[i]].time;
                sumElements++;
            }

        timeSum=timeSum/sumElements;
        timeSum=timeSum-2000;

        for(int i=4;i<16;i++)
            if(marker[i])
            {
                cache[written++]=i-4;
                cache[written++]=buffer[i][readPointer[i]].energy;
                cache[written++]=buffer[i][readPointer[i]].time-timeSum;
                if(marker[i/4])
                {
                cache[written++]=buffer[i/4][readPointer[i/4]].energy;
                cache[written++]=buffer[i/4][readPointer[i/4]].time-timeSum;
                }
                else
                {
                    cache[written++]=0;
                    cache[written++]=0;
                }
            }
    }

}

int buildEvents()
{
    int64_t leastTime=0;
    int toBeRead=0;
    for(int k=1;k<16;k++)
        readChannel[k]=0;

    for(int k=1;k<16;k++)
    {
            if(buffer[k].size()>readPointer[k])
            {
                //hasData=1;
                if(leastTime==0) leastTime=buffer[k][readPointer[k]].time;
                else if(leastTime>buffer[k][readPointer[k]].time) leastTime=buffer[k][readPointer[k]].time;
            }
    }
    /*for(int k=4;k<8;k++)
    {
            if(buffer[k].size()>readPointer[k])
            {
                //hasData=1;
                if(leastTime==0) leastTime=buffer[k][readPointer[k]].time+offset;
                else if(leastTime>buffer[k][readPointer[k]].time+offset) leastTime=buffer[k][readPointer[k]].time+offset;
            }
    }*/

    for(int k=1;k<16;k++)
    {
        if(buffer[k].size()>readPointer[k])
            if(buffer[k][readPointer[k]].time<(leastTime+80))
            {
                toBeRead++;
                readChannel[k]=1;
            }
    }
   /* for(int k=4;k<8;k++)
    {
        if(buffer[k].size()>readPointer[k])
            if(buffer[k][readPointer[k]].time+offset<(leastTime+80))
            {
                toBeRead++;
                readPointer[k]++;
            }
    }*/

    writeToCache(readChannel);

    for(int i=1;i<16;i++)
        if(readChannel[i])
            readPointer[i]++;

    return toBeRead;

}

void readBuffer()
{
    readBufferHeader();
    wordsRead=6;
    while(wordsRead<nofBufferWords)
        wordsRead+=readEvent();
    //std::cout<<bufferTime<<std::endl;
}

int main()
{
    QApplication* a= new QApplication(0,0);
    filenames =QFileDialog::getOpenFileNames(0,"Choose runs", "/home","Text (*)");

    for(z=0;z<filenames.size();z++)
    {
    QByteArray ba = filenames[z].toLatin1();
    const char *c_str2 = ba.data();
    dataFile.open (c_str2,std::ios::binary|std::ios::in);
    int lastSlash=filenames[z].lastIndexOf("/");
    QString runName=filenames[z];
    std::cout<<"Converting "<<runName.toStdString()<<std::endl;
    runName.remove(0,lastSlash+1);
    QRegExp xRegExp("(-?\\d+(?:[\\.,]\\d+(?:e\\d+)?)?)");
    xRegExp.indexIn(runName);
    QStringList xList = xRegExp.capturedTexts();
    runnum=xList[0].toInt();
    uint64_t read=0;
    uint64_t tbd=0;
    int good=0;
    int readEv;
    readPointer.resize(16);
    for(int i=0;i<16;i++)
        readPointer[i]=0;

    cache.resize(8176);
    buffer.resize(16);
    readChannel.resize(16);

    openNewFile();
    removeHeader();

    while(!dataFile.eof())
        readBuffer();


    for(int i=1;i<16;i++)
    {
          tbd+=buffer[i].size();
    }

    while(read<tbd)
    {
        readEv=buildEvents();
        read+=readEv;
        if(readEv>1)
            good++;
    }

   /* for(int i=-1000;i<1000;i++)
    {
        read=0;
        good=0;
        for(int j=0;j<16;j++)
            readPointer[j]=0;
        while(read<tbd)
        {
            readEv=buildEvents(i);
            read+=readEv;
            if(readEv>1)
                good++;
        }
        chris<<good<<std::endl<<std::flush;
    }

   for(int i=1;i<16;i++)
       std::cout<<buffer[i][0].time<<std::endl;*/
   dataFile.close();
    }
}

