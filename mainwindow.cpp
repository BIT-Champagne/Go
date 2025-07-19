// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("围棋对弈");

    // 计算窗口总宽度（棋盘宽度 + 右侧面板宽度）
    int totalWidth = MARGIN * 2 + CELL_SIZE * (BOARD_SIZE - 1) + RIGHT_PANEL_WIDTH;
    int totalHeight = MARGIN * 2 + CELL_SIZE * (BOARD_SIZE - 1);

    setFixedSize(totalWidth, totalHeight);

    // 初始化棋盘
    board.resize(BOARD_SIZE, std::vector<Stone>(BOARD_SIZE, EMPTY));
    currentPlayer = BLACK;
    lastMove = {-1, -1};
    lastBlackCapture.count = 0;
    lastWhiteCapture.count = 0;

}

MainWindow::~MainWindow()
{
    delete ui;
}

// 绘制棋盘和棋子
void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 计算棋盘总尺寸
    const int totalSize = CELL_SIZE * (BOARD_SIZE - 1);

    // 绘制棋盘背景（仅左侧区域）
    painter.fillRect(
        MARGIN - CELL_SIZE/2,
        MARGIN - CELL_SIZE/2,
        totalSize + CELL_SIZE,
        totalSize + CELL_SIZE,
        QColor(240, 180, 120)
        );

    // 绘制网格线
    QPen gridPen(Qt::black, 1);
    gridPen.setCapStyle(Qt::FlatCap);
    painter.setPen(gridPen);

    for (int i = 0; i < BOARD_SIZE; ++i) {
        // 横线
        painter.drawLine(
            MARGIN,
            MARGIN + i * CELL_SIZE,
            MARGIN + totalSize,
            MARGIN + i * CELL_SIZE
            );
        // 竖线
        painter.drawLine(
            MARGIN + i * CELL_SIZE,
            MARGIN,
            MARGIN + i * CELL_SIZE,
            MARGIN + totalSize
            );
    }

    // 绘制星位（天元和四星）
    QVector<QPoint> starPoints = {
        QPoint(3, 3), QPoint(3, 9), QPoint(3, 15),
        QPoint(9, 3), QPoint(9, 9), QPoint(9, 15),
        QPoint(15, 3), QPoint(15, 9), QPoint(15, 15)
    };
    painter.setBrush(Qt::black);
    painter.setPen(Qt::NoPen);  // 无边框

    for (const auto& p : starPoints) {
        painter.drawEllipse(
            MARGIN + p.x() * CELL_SIZE - 4,
            MARGIN + p.y() * CELL_SIZE - 4,
            8, 8
            );
    }

    // 绘制棋子（根据棋盘状态）
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (board[i][j] != EMPTY) {
                QColor color = (board[i][j] == BLACK) ? Qt::black : Qt::white;
                painter.setBrush(color);
                painter.setPen(QPen(Qt::gray, 1));

                // 棋子居中绘制
                painter.drawEllipse(
                    MARGIN + i * CELL_SIZE - CELL_SIZE/2,
                    MARGIN + j * CELL_SIZE - CELL_SIZE/2,
                    CELL_SIZE, CELL_SIZE
                    );
            }
        }
    }
}
// 处理鼠标点击（落子）
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    int boardRight = MARGIN + CELL_SIZE * (BOARD_SIZE - 1);
    if (event->x() < boardRight) {
        int x = (event->x() - MARGIN + CELL_SIZE/2) / CELL_SIZE;
        int y = (event->y() - MARGIN + CELL_SIZE/2) / CELL_SIZE;

        if (isValidPosition(x, y) && board[x][y] == EMPTY) {
            // 1. 先判断劫争（此时 currentPlayer 是即将落子的颜色）
            if (isKo(x, y)) {
                QMessageBox::information(this, "提示", "这是劫争，需先在其他地方落子");
                return;
            }

            // 2. 落子
            board[x][y] = currentPlayer;

            // 3. 检查提子并更新劫争信息
            checkAndRemoveDeadStones(x, y);

            // 4. 切换玩家
            currentPlayer = (currentPlayer == BLACK) ? WHITE : BLACK;

            // 5. 重绘
            update();
        }
    }
}

// 检测并移除没有气的棋子
void MainWindow::checkAndRemoveDeadStones(int x, int y)
{
    Stone currentColor = board[x][y];
    Stone opponentColor = (currentColor == BLACK) ? WHITE : BLACK;

    // 存储本次提子信息
    CaptureInfo currentCapture;

    // 检查四个方向的对手棋子
    QVector<QPoint> directions = {
        QPoint(-1, 0), QPoint(1, 0), QPoint(0, -1), QPoint(0, 1)
    };

    for (const auto& dir : directions) {
        int nx = x + dir.x();
        int ny = y + dir.y();

        if (isValidPosition(nx, ny) && board[nx][ny] == opponentColor) {
            // 检查这组棋子是否有气
            std::vector<std::pair<int, int>> visited;
            if (!hasLiberty(nx, ny, opponentColor, visited)) {
                // 记录被提子的位置
                for (const auto& pos : visited) {
                    currentCapture.positions.push_back(pos);
                }

                // 移除这组棋子
                removeStones(visited);
            }
        }
    }

    // 更新提子数量
    currentCapture.count = currentCapture.positions.size();

    // 更新对应玩家的上一次提子信息
    if (currentColor == BLACK) {
        lastBlackCapture = currentCapture;
    } else {
        lastWhiteCapture = currentCapture;
    }

    // 检查自己是否因为落子导致被提（自杀棋）
    std::vector<std::pair<int, int>> visitedSelf;
    if (!hasLiberty(x, y, currentColor, visitedSelf)) {
        // 自杀棋，回退操作
        board[x][y] = EMPTY;
        update();
        return;
    }

    // 记录当前落子位置
    lastMove = {x, y};
}

// 检测一组相连棋子的气（使用BFS）
bool MainWindow::hasLiberty(int x, int y, Stone color, std::vector<std::pair<int, int>>& visited)
{
    if (!isValidPosition(x, y) || board[x][y] != color)
        return false;

    std::queue<std::pair<int, int>> q;
    std::vector<std::vector<bool>> visitedGrid(BOARD_SIZE, std::vector<bool>(BOARD_SIZE, false));

    q.push({x, y});
    visitedGrid[x][y] = true;

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();
        visited.push_back({cx, cy});

        // 检查四个方向
        QVector<QPoint> directions = {
            QPoint(-1, 0), QPoint(1, 0), QPoint(0, -1), QPoint(0, 1)
        };

        for (const auto& dir : directions) {
            int nx = cx + dir.x();
            int ny = cy + dir.y();

            if (!isValidPosition(nx, ny))
                continue;

            if (board[nx][ny] == EMPTY) {
                // 找到气
                return true;
            } else if (board[nx][ny] == color && !visitedGrid[nx][ny]) {
                // 相连的同色棋子，加入队列
                q.push({nx, ny});
                visitedGrid[nx][ny] = true;
            }
        }
    }

    // 没有找到气
    return false;
}

// 移除一组棋子
void MainWindow::removeStones(const std::vector<std::pair<int, int>>& stones)
{
    for (const auto& [x, y] : stones) {
        board[x][y] = EMPTY;

        // 从视觉上移除棋子
        // （如果使用QGraphicsScene实现会更优雅，这里简化处理）
        update();
    }
}

// 判断位置是否合法
bool MainWindow::isValidPosition(int x, int y) const
{
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

// 简单劫争判断（防止无限循环提子）
bool MainWindow::isKo(int x, int y)
{
    // 关键修正：当前准备落子的颜色是 currentPlayer（而非 board[x][y]，此时还未落子）
    Stone currentColor = currentPlayer;
    Stone opponentColor = (currentColor == BLACK) ? WHITE : BLACK;

    // 获取对手的上一次提子信息
    const CaptureInfo& opponentLastCapture =
        (opponentColor == BLACK) ? lastBlackCapture : lastWhiteCapture;

    // 劫争条件：
    // 1. 对手上一次只提了1颗子
    // 2. 当前落子位置正好是那颗被提子的位置
    if (opponentLastCapture.count == 1 &&
        !opponentLastCapture.positions.empty() &&
        opponentLastCapture.positions[0] == std::make_pair(x, y)) {
        return true;
    }

    return false;
}
