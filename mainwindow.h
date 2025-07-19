// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>
#include <vector>
#include <queue>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// mainwindow.h
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:


    Ui::MainWindow *ui;
    static const int BOARD_SIZE = 19;
    static const int CELL_SIZE = 30;
    static const int MARGIN = 50;        // 左侧和顶部边距
    static const int RIGHT_PANEL_WIDTH = 200;  // 右侧面板宽度

    enum Stone { EMPTY, BLACK, WHITE };
    std::vector<std::vector<Stone>> board;
    Stone currentPlayer;

    // 记录上一次提子信息（用于劫争判断 以及 悔棋回溯等）
    struct CaptureInfo {
        std::vector<std::pair<int, int>> positions;  // 被提子的位置
        int count = 0;                               // 提子数量
        // 显式初始化，避免未定义行为
        CaptureInfo() : positions(), count(0) {}
    };

    // 记录上一步位置（用于劫争判断）
    std::pair<int, int> lastMove;

    CaptureInfo lastBlackCapture;  // 黑方上一次提子信息
    CaptureInfo lastWhiteCapture;  // 白方上一次提子信息

    // 检测并移除没有气的棋子
    void checkAndRemoveDeadStones(int x, int y);

    // 检测一组相连棋子的气
    bool hasLiberty(int x, int y, Stone color, std::vector<std::pair<int, int>>& visited);

    // 移除一组棋子
    void removeStones(const std::vector<std::pair<int, int>>& stones);

    // 判断位置是否合法
    bool isValidPosition(int x, int y) const;

    // 检查劫争（简单实现）
    bool isKo(int x, int y);
};
#endif // MAINWINDOW_H
