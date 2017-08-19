#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


struct UOPAnimationData
{
    std::string *path;
    std::string binpath;    // trivial
    uint32_t animID;
    uint32_t groupID;
    uint32_t dataBlockAddress;
    uint32_t compressedSize;
    uint32_t decompressedSize;
    uint32_t dataBlockLength, skip1;    // trivial
    uint16_t skip2; // trivial
    uint64_t hash;  // trivial
};


struct UOPFrameData
{
    size_t dataStart;
    uint16_t frameId;
    uint32_t pixelDataOffset;
};


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_spinBox_animid_valueChanged(int arg1);
    void on_spinBox_framenumber_valueChanged(int arg1);
    void on_spinBox_groupid_valueChanged(int arg1);
    void on_spinBox_direction_valueChanged(int arg1);
    void on_pushButton_browse_clicked();
    void on_pushButton_index_pressed();

private:
    Ui::MainWindow *ui;
    int m_anim      = 0;
    int m_group     = 0;
    int m_frame     = 0;
    int m_dir       = 0;

    UOPAnimationData* animationObjects[2048][100] = {{nullptr}};   //animationObjects[animID][groupID]

    void resetValues();
    void buildAnimTable();
    void loadAnimationData();
    void drawFrame(UOPFrameData &frameData, char *decData, size_t dcSize);
};


#endif // MAINWINDOW_H
