// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket(new QTcpSocket(this))
{
    ui->setupUi(this);
    setWindowTitle("围棋对弈");

    // TCP连接
    socket->connectToHost("127.0.0.1", 1234);
    connect(socket, &QTcpSocket::connected, this, &MainWindow::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readServer);

    // 窗口尺寸计算
    int totalWidth = MARGIN * 2 + CELL_SIZE * (BOARD_SIZE - 1) + RIGHT_PANEL_WIDTH;
    int totalHeight = MARGIN * 2 + CELL_SIZE * (BOARD_SIZE - 1);
    setFixedSize(totalWidth, totalHeight);

    // 初始化棋盘
    board.resize(BOARD_SIZE, std::vector<Stone>(BOARD_SIZE, EMPTY));

    // 初始化button等
    QPushButton *btn_over = new QPushButton("申请数子", this);
    btn_over->setGeometry(700, MARGIN, 120, 30);
    connect(btn_over, &QPushButton::clicked, this, &MainWindow::onBtnOver);

    myColor = EMPTY;  // 初始化为未分配，等待服务器分配
    currentTurn = BLACK;  // 黑方先行
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
        // 画横线
        painter.drawLine(
            MARGIN,
            MARGIN + i * CELL_SIZE,
            MARGIN + totalSize,
            MARGIN + i * CELL_SIZE
            );
        // 画竖线
        painter.drawLine(
            MARGIN + i * CELL_SIZE,
            MARGIN,
            MARGIN + i * CELL_SIZE,
            MARGIN + totalSize
            );
    }

    // 绘制9个星位
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
    // 若尚未分配颜色，不处理落子
    if (myColor == EMPTY) return;

    int boardRight = MARGIN + CELL_SIZE * (BOARD_SIZE - 1);
    if (event->x() < boardRight) {
        int x = (event->x() - MARGIN + CELL_SIZE/2) / CELL_SIZE;
        int y = (event->y() - MARGIN + CELL_SIZE/2) / CELL_SIZE;

        // 检查：是否是自己的回合 + 位置合法 + 无棋子
        if (currentTurn != myColor) {
            QMessageBox::information(this, "提示", "不是你的回合");
            return;
        }
        if (!isValidPosition(x, y) || board[x][y] != EMPTY) {
            return;  // 位置不合法或已有棋子
        }
        // 劫争判断
        if (isKo(x, y)) {
            QMessageBox::information(this, "提示", "这是劫争，需先在其他地方落子");
            return;
        }

        // 自己落子（用 myColor 表示自身颜色，固定不变）
        board[x][y] = myColor;
        // 检查提子
        checkAndRemoveDeadStones(x, y);
        // 发送落子信息给服务器
        QJsonObject obj;
        obj["x"] = x;
        obj["y"] = y;
        socket->write(QJsonDocument(obj).toJson());

        // 切换回合（当前回合交给对方）
        currentTurn = (myColor == BLACK) ? WHITE : BLACK;
        // 重绘棋盘
        update();
    }
}

// TCP连接后回调函数
void MainWindow::onConnected(){

}

void MainWindow::readServer()
{
    QByteArray data = socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qDebug() << "无效的服务器消息";
        return;
    }
    QJsonObject obj = doc.object();

    // 1. 处理服务器分配颜色（仅第一次连接时）
    if (obj.contains("color")) {
        QString color = obj["color"].toString();
        myColor = (color == "black") ? BLACK : WHITE;  // 固定自己的颜色
        currentTurn = BLACK;  // 黑方先行
        update();  // 刷新界面显示自己的颜色
        return;
    }

    // if(obj.contains("over")){
    //     QMessageBox::StandardButton overresult = QMessageBox::question(this, "通知", "是否同意数子？", QMessageBox::Ok | QMessageBox::Cancel);
    //     if(overresult == QMessageBox::Ok){

    //     }
    //     else {

    //     }
    // }
    // 2. 处理对方落子（完全独立于本地鼠标事件）
    if (obj.contains("x") && obj.contains("y")) {
        int x = obj["x"].toInt();
        int y = obj["y"].toInt();
        // 对方颜色 = 与自己颜色相反
        Stone opponentColor = (myColor == BLACK) ? WHITE : BLACK;

        // 直接在本地棋盘绘制对方的棋子（不依赖任何本地鼠标事件）
        if (isValidPosition(x, y) && board[x][y] == EMPTY) {
            board[x][y] = opponentColor;
            // 检查对方落子后的提子（对方可能提掉自己的子）
            checkAndRemoveDeadStones(x, y);
            // 切换回合（当前回合交给自己）
            currentTurn = myColor;
            // 强制重绘，显示对方的落子
            update();
        }
    }
}

// 申请数子
void MainWindow::onBtnOver(){

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

// 检测一组棋子是否有气（BFS）
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
    // 当前落子的是自己（myColor 固定）
    Stone opponentColor = (myColor == BLACK) ? WHITE : BLACK;
    // 获取对手上一次的提子信息
    const CaptureInfo& opponentLastCapture =
        (opponentColor == BLACK) ? lastBlackCapture : lastWhiteCapture;

    // 劫争条件：对手上一次只提了1颗子，且当前位置是被提的位置
    return (opponentLastCapture.count == 1
            && !opponentLastCapture.positions.empty()
            && opponentLastCapture.positions[0] == std::make_pair(x, y));
}
