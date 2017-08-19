#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <QFileDialog>
#include <QImage>
#include <QGraphicsPixmapItem>

//#define ZLIB_DLL  // causes error (undefined reference to _imp__uncompress), even if this define is suggested in zlib.h...
#include "zlib.h"

#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_spinBox_animid_valueChanged(int arg1)
{
    m_anim = arg1;
    loadAnimationData();
}

void MainWindow::on_spinBox_groupid_valueChanged(int arg1)
{
    m_group = arg1;
    loadAnimationData();
}


void MainWindow::on_spinBox_framenumber_valueChanged(int arg1)
{
    if (arg1 < 0)
        return;
    m_frame = arg1;
    loadAnimationData();
}

void MainWindow::on_spinBox_direction_valueChanged(int arg1)
{
    if (arg1 < 0)
        return;
    m_dir = arg1;
    loadAnimationData();
}

void MainWindow::on_pushButton_browse_clicked()
{
    QFileDialog *dlg = new QFileDialog();
    dlg->setOption(QFileDialog::ShowDirsOnly);
    dlg->setFileMode(QFileDialog::Directory);
    dlg->setAcceptMode(QFileDialog::AcceptOpen);
    dlg->setViewMode(QFileDialog::List);

    if (dlg->exec())
        ui->lineEdit->setText(dlg->directory().absolutePath());

    delete dlg;
}


void MainWindow::resetValues()
{
    ui->label_status_val->setText("Idle");

    ui->label_animfile_val->setText("-");
    ui->label_binfile_val->setText("-");
    ui->label_binfilehash_val->setText("-");

    ui->label_datablockaddress_val->setText("-");
    ui->label_datablocklength_val->setText("-");
    ui->label_compressedsize_val->setText("-");
    ui->label_decompressedsize_val->setText("-");
    ui->label_skip1_val->setText("-");
    ui->label_skip2_val->setText("-");

    ui->label_formatid_val->setText("-");
    ui->label_version_val->setText("-");
    ui->label_bindecompressedsize_val->setText("-");
    ui->label_animid_val->setText("-");
    ui->label_unknown1_val->setText("-");
    ui->label_unknown2_val->setText("-");
    ui->label_unknown3_val->setText("-");
    ui->label_headerlength_val->setText("-");

    ui->label_framecount_val->setText("-");
    ui->label_frameid_val->setText("-");
    ui->label_width_val->setText("-");
    ui->label_height_val->setText("-");
    ui->label_xcenter_val->setText("-");
    ui->label_ycenter_val->setText("-");

    if (ui->graphicsView->scene() != nullptr)
        delete ui->graphicsView->scene();
    QGraphicsScene* scene = new QGraphicsScene();
    ui->graphicsView->setScene(scene);
    ui->graphicsView->scene()->clear();
}

void MainWindow::on_pushButton_index_pressed()
{
    buildAnimTable();
}



uint64_t createHash(std::string s)
{
    uint32_t eax, ecx, edx, ebx, esi, edi;

    eax = ecx = edx = ebx = esi = edi = 0;
    ebx = edi = esi = (uint32_t) s.length() + 0xDEADBEEF;

    unsigned int i = 0;

    for ( i = 0; i + 12 < s.length(); i += 12 )
    {
        edi = (uint32_t) ( ( s[ i + 7 ] << 24 ) | ( s[ i + 6 ] << 16 ) | ( s[ i + 5 ] << 8 ) | s[ i + 4 ] ) + edi;
        esi = (uint32_t) ( ( s[ i + 11 ] << 24 ) | ( s[ i + 10 ] << 16 ) | ( s[ i + 9 ] << 8 ) | s[ i + 8 ] ) + esi;
        edx = (uint32_t) ( ( s[ i + 3 ] << 24 ) | ( s[ i + 2 ] << 16 ) | ( s[ i + 1 ] << 8 ) | s[ i ] ) - esi;

        edx = ( edx + ebx ) ^ ( esi >> 28 ) ^ ( esi << 4 );
        esi += edi;
        edi = ( edi - edx ) ^ ( edx >> 26 ) ^ ( edx << 6 );
        edx += esi;
        esi = ( esi - edi ) ^ ( edi >> 24 ) ^ ( edi << 8 );
        edi += edx;
        ebx = ( edx - esi ) ^ ( esi >> 16 ) ^ ( esi << 16 );
        esi += edi;
        edi = ( edi - ebx ) ^ ( ebx >> 13 ) ^ ( ebx << 19 );
        ebx += esi;
        esi = ( esi - edi ) ^ ( edi >> 28 ) ^ ( edi << 4 );
        edi += ebx;
    }

    if ( s.length() - i > 0 )
    {
        switch ( s.length() - i )
        {
            case 12:    esi += (uint32_t) s[ i + 11 ] << 24;
            case 11:    esi += (uint32_t) s[ i + 10 ] << 16;
            case 10:    esi += (uint32_t) s[ i + 9  ] << 8;
            case 9:     esi += (uint32_t) s[ i + 8  ];
            case 8:     edi += (uint32_t) s[ i + 7  ] << 24;
            case 7:     edi += (uint32_t) s[ i + 6  ] << 16;
            case 6:     edi += (uint32_t) s[ i + 5  ] << 8;
            case 5:     edi += (uint32_t) s[ i + 4  ];
            case 4:     ebx += (uint32_t) s[ i + 3  ] << 24;
            case 3:     ebx += (uint32_t) s[ i + 2  ] << 16;
            case 2:     ebx += (uint32_t) s[ i + 1  ] << 8;
            case 1:     ebx += (uint32_t) s[ i ];
            break;
        }

        esi = ( esi ^ edi ) - ( ( edi >> 18 ) ^ ( edi << 14 ) );
        ecx = ( esi ^ ebx ) - ( ( esi >> 21 ) ^ ( esi << 11 ) );
        edi = ( edi ^ ecx ) - ( ( ecx >> 7  ) ^ ( ecx << 25 ) );
        esi = ( esi ^ edi ) - ( ( edi >> 16 ) ^ ( edi << 16 ) );
        edx = ( esi ^ ecx ) - ( ( esi >> 28 ) ^ ( esi << 4  ) );
        edi = ( edi ^ edx ) - ( ( edx >> 18 ) ^ ( edx << 14 ) );
        eax = ( esi ^ edi ) - ( ( edi >> 8  ) ^ ( edi << 24 ) );

        return ( (uint64_t) edi << 32 ) | eax;
    }

    return ( (uint64_t) esi << 32 ) | eax;
}



// A UOP file can have multiple blocks and each one can have multiple files.
// The uop format is the same for Classic and Enhanced Client, but the files inside it are different.

// References for source code for general UOP parsing: Malganis' Mythic Package Editor.
// References for source code for AnimationFrame*.uop parsing/handling and all: OrionUO.
// Got also some extra infos from Kons' wiki.


void MainWindow::buildAnimTable()
{
    std::unordered_map<uint64_t, UOPAnimationData*> hashes;

    for (int uopFile_i = 1; uopFile_i <= 4; uopFile_i++)
    {
        uint32_t magic;         // M Y P 0
        uint32_t version;
        uint32_t length;
        uint64_t nextBlockAddress;
        uint32_t blockSize;
        uint32_t filesInUOP;


        std::string *path = new std::string( ui->lineEdit->text().toStdString());
        std::replace(path->begin(), path->end(), '\\', '/');
        if ((*path)[path->length()-1] != '/')
            *path += '/';
        *path += "AnimationFrame" + std::to_string(uopFile_i) + ".uop";

        //qDebug() << path->c_str();
        std::fstream animFile (*path, std::ios::binary | std::ios::in);
        //if (!FileExists(*path))
        //	continue;
        if (!animFile)
            continue;

        /* [1] Starting to read the UOP HEADER */

        animFile.read(reinterpret_cast<char*>(&magic), 4);
        if ( magic != 0x50594D )
        {
            ui->label_status_val->setText(QString("AnimationFrame%1.uop: invalid file").arg(uopFile_i));
            return;
        }

        animFile.read(reinterpret_cast<char*>(&version), 4);
        if ( version > 5 )
        {
            ui->label_status_val->setText(QString("Unsupported version: %1").arg(version));
            return;
        }

        animFile.read(reinterpret_cast<char*>(&length), 4);
        animFile.read(reinterpret_cast<char*>(&nextBlockAddress), 8);     // address of the first block

        animFile.read(reinterpret_cast<char*>(&blockSize), 4);
        animFile.read(reinterpret_cast<char*>(&filesInUOP), 4);

        /* [1] End of the UOP HEADER */


        animFile.seekg(nextBlockAddress, std::ifstream::beg);

        do
        {
            uint32_t filesInBlock;
            uint64_t dataBlockAddress;
            uint32_t dataBlockLength;
            uint32_t compressedSize;
            uint32_t decompressedSize;
            uint64_t fileHash;
            uint32_t skip1;     // Adler32 of [4a] Data Header in little endian, unknown in Version 5 [datablockhash]
            uint16_t skip2;     // Compression type (0 - no compression, 1 - zlib)

            /* [2] Starting to read the FILES BLOCK HEADER */

            animFile.read(reinterpret_cast<char*>(&filesInBlock), 4);
            animFile.read(reinterpret_cast<char*>(&nextBlockAddress), 8);

            /* [2] End of the stored FILES BLOCK HEADER */


            for (unsigned int files_i = 0; files_i < filesInBlock; files_i++)
            {
                /* [3] Starting to read the FILE HEADER */

                animFile.read(reinterpret_cast<char*>(&dataBlockAddress), 8);
                animFile.read(reinterpret_cast<char*>(&dataBlockLength), 4);
                animFile.read(reinterpret_cast<char*>(&compressedSize), 4);
                animFile.read(reinterpret_cast<char*>(&decompressedSize), 4);
                animFile.read(reinterpret_cast<char*>(&fileHash), 8);
                animFile.read(reinterpret_cast<char*>(&skip1), 4);
                animFile.read(reinterpret_cast<char*>(&skip2), 2);

                /* [3] End of the FILE HEADER */

                if (dataBlockAddress == 0)
                    continue;

                UOPAnimationData* dataStruct = new UOPAnimationData;
                dataStruct->path = path;
                dataStruct->animID = (uint)-1;
                dataStruct->groupID = (uint)-1;
                dataStruct->dataBlockAddress = static_cast<unsigned int>(dataBlockAddress + dataBlockLength);
                dataStruct->dataBlockLength = dataBlockLength;
                dataStruct->compressedSize = compressedSize;
                dataStruct->decompressedSize = decompressedSize;
                dataStruct->skip1 = skip1;
                dataStruct->skip2 = skip2;

                dataStruct->path = path;
                hashes[fileHash] = dataStruct;
            }

            animFile.seekg(nextBlockAddress, std::ifstream::beg);
        } while (nextBlockAddress != 0);

        animFile.close();
    }

    ui->listWidget->clear();

    for (int animId = 0; animId < 2048; animId++)
    {
        int groupTotal = 0;
        for (int grpId = 0; grpId < 100; grpId++)
        {
            char hashString[100];
            sprintf(hashString, "build/animationlegacyframe/%06i/%02i.bin", animId, grpId);
            uint64_t hash = createHash(hashString);
            if (hashes.find(hash) == hashes.end())
                continue;

            ui->listWidget->addItem(QString("AnimID: %1 | GroupID: %2").arg(QString::number(animId), QString::number(grpId)));
            UOPAnimationData* dataStruct = hashes.at(hash);
            dataStruct->animID = animId;
            dataStruct->groupID = grpId;

            dataStruct->binpath = std::string(hashString);
            dataStruct->hash = hash;

            animationObjects[animId][grpId] = dataStruct;
            groupTotal++;
        }
        if (groupTotal != 0)
            ui->listWidget->addItem(QString("AnimID: %1 | Total Groups: %2").arg(QString::number(animId), QString::number(groupTotal)));
    }
}


void MainWindow::loadAnimationData()
{
    resetValues();

    if (animationObjects[m_anim][m_group] == nullptr)
    {
        ui->label_status_val->setText("Not found");
        return;
    }
    UOPAnimationData* animData = animationObjects[m_anim][m_group];

    ui->label_animfile_val->setText(QString(animData->path->c_str()));
    ui->label_binfile_val->setText(QString(animData->binpath.c_str()));
    ui->label_binfilehash_val->setText(QString::number(animData->hash));

    ui->label_datablockaddress_val->setText(QString::number(animData->dataBlockAddress - animData->dataBlockLength));
    ui->label_datablocklength_val->setText(QString::number(animData->dataBlockLength));
    ui->label_compressedsize_val->setText(QString::number(animData->compressedSize));
    ui->label_decompressedsize_val->setText(QString::number(animData->decompressedSize));

    ui->label_skip1_val->setText(QString::number(animData->skip1));
    ui->label_skip2_val->setText(QString::number(animData->skip2));


    std::fstream fs_anim(*animData->path, std::ios::binary | std::ios::in);

    //reading compressed data from uop file stream  
    char *buf = new char[animData->compressedSize];
    fs_anim.seekg(animData->dataBlockAddress, std::fstream::beg);
    fs_anim.read(buf, animData->compressedSize);

    //decompressing here
    char* decData = new char[animData->decompressedSize];
    uLongf cLen = animData->compressedSize;
    uLongf dLen = animData->decompressedSize;

    int z_err = uncompress(reinterpret_cast<unsigned char *>(decData), &dLen, reinterpret_cast<unsigned char const*>(buf), cLen);
    delete[] buf;
    fs_anim.close();

    if (z_err != Z_OK)
    {
        qDebug() << "UOP anim decompression failed" << z_err;
        qDebug() << "Anim file:" << animData->path->c_str();
        qDebug() << "Anim offset" << animData->dataBlockAddress;
        ui->label_status_val->setText("Decompression failed");
        return;
    }

    size_t decDataOff = 0;      // offset for reading inside decData (array with the decompressed data)

    //format id?
    //trivial
    uint32_t fid;
    memcpy(&fid, decData + decDataOff, 4);
    decDataOff += 4;
    ui->label_formatid_val->setText(QString::number(fid));

    //version
    //trivial
    uint32_t vers;
    memcpy(&vers, decData + decDataOff, 4);
    decDataOff += 4;
    ui->label_version_val->setText(QString::number(vers));

    //decompressed data size
    uint32_t dcsize;
    memcpy(&dcsize, decData + decDataOff, 4);
    decDataOff += 4;
    ui->label_bindecompressedsize_val->setText(QString::number(dcsize));

    //anim id
    //trivial
    uint32_t animId;
    memcpy(&animId, decData + decDataOff, 4);
    decDataOff += 4;
    ui->label_animid_val->setText(QString::number(animId));

    //8 bytes unknown [1]
    // File time (number of 100-nanosecond intervals since January 1, 1601 UTC)?
    //trivial
    uint64_t unk1;
    memcpy(&unk1, decData + decDataOff, 8);
    decDataOff += 8;
    ui->label_unknown1_val->setText(QString::number(unk1));

    //unknown [2]
    //trivial
    uint16_t unk2;
    memcpy(&unk2, decData + decDataOff, 2);
    decDataOff += 2;
    ui->label_unknown2_val->setText(QString::number(unk2));

    //unknown [3]
    //trivial
    uint16_t unk3;
    memcpy(&unk3, decData + decDataOff, 2);
    decDataOff += 2;
    ui->label_unknown3_val->setText(QString::number(unk3));

    //header length
    //trivial
    uint32_t headlen;
    memcpy(&headlen, decData + decDataOff, 4);
    decDataOff += 4;
    ui->label_headerlength_val->setText(QString::number(headlen));

    //framecount (total frame number, for every direction)
    uint32_t frameCount;
    memcpy(&frameCount, decData + decDataOff, 4);
    decDataOff += 4;

    if ((uint)m_frame > frameCount)
    {
        m_frame = 0;
        ui->spinBox_framenumber->setValue(m_frame);
    }
    ui->label_framecount_val->setText(QString::number(frameCount));
    // framecount / 5 is the number of frames for each direction
    ui->spinBox_framenumber->setMaximum( (frameCount != 0) ? ((frameCount/5) - 1) : 0 );

    //address of the first frame
    uint32_t frameAddress;
    memcpy(&frameAddress, decData + decDataOff, 4);

    decDataOff = frameAddress;


    // reading frame data
    std::vector<UOPFrameData> pixelDataOffsets;

    for (unsigned int frame_i = 0; frame_i < frameCount; frame_i++)
    {
        UOPFrameData curFrameData;
        curFrameData.dataStart = decDataOff;

        //anim group
        decDataOff += 2;

        //frame id
        memcpy(&curFrameData.frameId, decData + decDataOff, 2);
        decDataOff += 2;

        //8 bytes unknown
        decDataOff += 8;

        //offset (starting from dataStart) to this frame image data
        memcpy(&curFrameData.pixelDataOffset, decData + decDataOff, 4);
        decDataOff += 4;

        /*
        int vsize = pixelDataOffsets.size();
        if (vsize + 1 != curFrameData.frameId)
        {
            while (vsize + 1 != curFrameData.frameId)
            {
                pixelDataOffsets.push_back(UOPFrameData{ });
                vsize++;
            }
        }
        */
        pixelDataOffsets.push_back(curFrameData);
    }
    int vectorSize = pixelDataOffsets.size();
    if (vectorSize < 50)
    {
        while (vectorSize != 50)
        {
            pixelDataOffsets.push_back(UOPFrameData{ });
            vectorSize++;
        }
    }

    //unsigned int dirFrameCount = pixelDataOffsets.size() / 5;   // 5 = number of directions
    //unsigned int dirFrameStartIdx = dirFrameCount * m_dir;
    unsigned int dirFrameStartIdx = (frameCount / 5) * m_dir;


    UOPFrameData &frameData = pixelDataOffsets[m_frame + dirFrameStartIdx];
    //UOPFrameData &frameData = curFrameData;

    drawFrame(frameData, decData, animData->decompressedSize);

    delete[] decData;
}



void MainWindow::drawFrame(UOPFrameData &frameData, char* decData, size_t dcSize)
{
    if (frameData.dataStart == 0)
    {
        ui->label_status_val->setText("Empty anim");
        return;
    }

    size_t decDataOff = frameData.dataStart + frameData.pixelDataOffset;

    ui->label_frameid_val->setText(QString::number(frameData.frameId));

    int16_t palette[256];
    memcpy(&palette, decData + decDataOff, 512);
    decDataOff += 512;

    int16_t xCenter, yCenter;
    uint16_t width, height;

    memcpy(&xCenter, decData + decDataOff, 2);
    decDataOff += 2;
    ui->label_xcenter_val->setText(QString::number(xCenter));

    memcpy(&yCenter, decData + decDataOff, 2);
    decDataOff += 2;
    ui->label_ycenter_val->setText(QString::number(yCenter));

    memcpy(&width, decData + decDataOff, 2);
    decDataOff += 2;
    ui->label_width_val->setText(QString::number(width));

    memcpy(&height, decData + decDataOff, 2);
    decDataOff += 2;
    ui->label_height_val->setText(QString::number(height));

    if (height == 0 || width == 0 || height > 800 || width > 800)
    {
        ui->label_status_val->setText(QString("Invalid width/height"));
        return;
    }

    QImage* img = new QImage((int)width, (int)height, QImage::Format_ARGB32);
    img->fill(0);

    while ( decDataOff < dcSize )
    {
        //HEADER:
        //
        //1F	1E	1D	1C	1B	1A	19	18	17	16 | 15	14	13	12	11	10	0F	0E	0D	0C | 0B	0A	09	08	07	06	05	04	03	02	01	00
        //        xOffset (10 bytes)             |          yOffset (10 bytes)           |            xRun (12 bytes)
        //The documentation(*) is wrong, because it inverted the position of x and y offsets. The one above is correct.
        //*http://wpdev.sourceforge.net/docs/formats/csharp/animations.html
        //
        //xOffset and yOffset are relative to xCenter and yCenter.
        //xOffset and yOffset are signed: see the compensation in the code below.
        //xRun indicates how many pixels are contained in this line.
        //
        //For this piece of code, the MulPatcher source helped A LOT!
        uint32_t header;
        memcpy(&header, decData + decDataOff, 4);
        decDataOff += 4;
        if ( header == 0x7FFF7FFF )
            break;

        uint32_t xRun = header & 0xFFF;              // take first 12 bytes
        uint32_t xOffset = (header >> 22) & 0x3FF;   // take 10 bytes
        uint32_t yOffset = (header >> 12) & 0x3FF;   // take 10 bytes
        // xOffset and yOffset are signed, so we need to compensate for that
        if (xOffset & 512)                  // 512 = 0x200
            xOffset |= (0xFFFFFFFF - 511);  // 511 = 0x1FF
        if (yOffset & 512)
            yOffset |= (0xFFFFFFFF - 511);

        int X = xOffset + xCenter;
        int Y = yOffset + yCenter + height;

        if (X < 0 || Y < 0 || Y >= height || X >= width)
            continue;

        for ( unsigned int k = 0; k < xRun; k++ )
        {
            uint8_t palette_index = 0;
            memcpy(&palette_index, decData + decDataOff, 1);
            decDataOff += 1;
            uint16_t color_argb16 = palette[palette_index];

            uint8_t a, r, g, b;
            a = 255;
            r = (uint8_t) (( color_argb16 & 0xF800) >> 10);
            g = (uint8_t) (( color_argb16 & 0x07E0) >> 5);
            b = (uint8_t) (( color_argb16 & 0x001F));
            r *= 8;
            g *= 8;
            b *= 8;
            uint32_t color_argb32 = (a << 24) | (r << 16) | (g << 8) | b;

            img->setPixel(X + k, Y, color_argb32);
        }
    }

    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(*img));
    delete img;
    ui->graphicsView->scene()->addItem(item);

}


/*
        // Scale the view / do the zoom
        double scaleFactor = 1.15;
        if(event->delta() > 0) {
            // Zoom in
            ui->graphicsView-> scale(scaleFactor, scaleFactor);

        } else {
            // Zooming out
             ui->graphicsView->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        }
*/

