#include <graphics.h>
#include <iostream>
#include <cstdio>
#include <vector>
#include <utility>
using namespace std;

#define GO_SIZE 19
#define GO_GRID 30
#define GO_OFFSET 20
#define GO_RADIUS 13

// ========== 按钮坐标定义 ==========
#define BTN_Y  GO_OFFSET + GO_SIZE * GO_GRID + 10
#define BTN_H 35
#define BTN1_X1 30
#define BTN1_X2 160
#define BTN2_X1 180
#define BTN2_X2 310
#define BTN3_X1 330
#define BTN3_X2 460

// 落子历史结构体：记录每一步完整信息
struct Step {
	int r, c;               // 落子坐标
	int color;              // 落子颜色 1黑 2白
	vector<pair<int, int>> captureList; // 本次提走的棋子
};

static int goBoard[GO_SIZE][GO_SIZE] = {0};
static int goPlayer = 1;
static bool backMenu = false;
static int captureBlack = 0;
static int captureWhite = 0;
static int passCount = 0;
static bool gameEnd = false;
static vector<Step> stepStack;
static int blackTotal, whiteTotal; // 终局计分
static int winner = 0; // 0未结束 1黑胜 2白胜

static void goDrawBg();
static void goDrawBoard();
static void goDrawAllChess();
static bool goGetGrid(int mx, int my, int &r, int &c);
static void goDrawPreview(int x, int y, int player);
static int getQi(int r, int c, int color, bool visited[GO_SIZE][GO_SIZE]);
static void removeDead(int color, vector<pair<int, int>> &delList);
static bool isForbidden(int r, int c, int color);
static void drawButton();
static int checkButtonClick(int mx, int my);
static void undoStep();
static int bfsTerritory(int startR, int startC, bool vis[GO_SIZE][GO_SIZE]);
static void calcScore(int &blackTotal, int &whiteTotal);
static void clearGroup(int r, int c, int color);
static void judgeWinner();

// 重置围棋对局
void resetGoGame() {
	for(int i=0; i<GO_SIZE; i++)
		for(int j=0; j<GO_SIZE; j++)
			goBoard[i][j] = 0;
	goPlayer = 1;
	backMenu = false;
	captureBlack = 0;
	captureWhite = 0;
	passCount = 0;
	gameEnd = false;
	winner = 0;
	stepStack.clear(); // 清空所有历史落子，支持悔棋
}
static int bfsTerritory(int startR, int startC, bool vis[GO_SIZE][GO_SIZE]) {
	vector<pair<int,int>> q;
	q.emplace_back(startR, startC);
	vis[startR][startC] = true;
	bool hasBlack = false, hasWhite = false;
	int dir[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
	while(!q.empty()) {
		auto p = q.back();
		q.pop_back();
		int r = p.first, c = p.second;
		for(int d=0; d<4; d++) {
			int nr = r + dir[d][0];
			int nc = c + dir[d][1];
			if(nr<0||nr>=GO_SIZE||nc<0||nc>=GO_SIZE) continue;
			if(vis[nr][nc]) continue;
			if(goBoard[nr][nc] == 1) hasBlack = true;
			else if(goBoard[nr][nc] == 2) hasWhite = true;
			else if(goBoard[nr][nc] == 0) {
				vis[nr][nc] = true;
				q.emplace_back(nr, nc);
			}
		}
	}
	if(hasBlack && !hasWhite) return 1;
	if(hasWhite && !hasBlack) return 2;
	return 0;
}
// 绘制三个功能按钮
void drawButton() {
	setlinecolor(WHITE);
	setlinestyle(PS_SOLID,1);
	// 按钮1
	setfillcolor(RGB(80, 140, 255));
	solidrectangle(BTN1_X1, BTN_Y, BTN1_X2, BTN_Y + BTN_H);
	settextcolor(WHITE);
	settextstyle(16, 0, "宋体");
	setbkmode(TRANSPARENT);
	outtextxy(BTN1_X1 + 30, BTN_Y + 8, "跳过回合");

	// 按钮2
	setfillcolor(RGB(90, 200, 110));
	solidrectangle(BTN2_X1, BTN_Y, BTN2_X2, BTN_Y + BTN_H);
	settextcolor(WHITE);
	settextstyle(16, 0, "宋体");
	setbkmode(TRANSPARENT);
	outtextxy(BTN2_X1 + 40, BTN_Y + 8, "悔棋");

	// 按钮3
	setfillcolor(RGB(235, 75, 75));
	solidrectangle(BTN3_X1, BTN_Y, BTN3_X2, BTN_Y + BTN_H);
	settextcolor(WHITE);
	settextstyle(16, 0, "宋体");
	setbkmode(TRANSPARENT);
	outtextxy(BTN3_X1 + 32, BTN_Y + 8, "返回菜单");
}

// 执行一次悔棋，恢复上一步棋盘状态
void undoStep() {
	if(stepStack.empty()) return; // 无落子，不能悔棋
	Step last = stepStack.back();
	stepStack.pop_back();

	// 1. 移除刚下的棋子
	goBoard[last.r][last.c] = 0;

	// 2. 恢复被提走的棋子，修正提子计数
	for(auto p : last.captureList) {
		int cr = p.first;
		int cc = p.second;
		goBoard[cr][cc] = (last.color == 1) ? 2 : 1;
	}
	if(last.color == 1)
		captureWhite -= last.captureList.size();
	else
		captureBlack -= last.captureList.size();

	// 3. 回退玩家到上一手
	goPlayer = last.color;
	passCount = 0; // 清空连续Pass计数
}



// 判断鼠标是否点击按钮，返回对应编号 0无点击 1Pass 2悔棋 3返回菜单
int checkButtonClick(int mx, int my) {
	if(my < BTN_Y || my > BTN_Y + BTN_H)
		return 0;
	if(mx >= BTN1_X1 && mx <= BTN1_X2)
		return 1;
	if(mx >= BTN2_X1 && mx <= BTN2_X2)
		return 2;
	if(mx >= BTN3_X1 && mx <= BTN3_X2)
		return 3;
	return 0;
}

// 绘制背景
void goDrawBg() {
	setfillcolor(RGB(180,180,180));
	solidrectangle(0,0,getwidth(),getheight());
}

// 绘制19路棋盘
void goDrawBoard() {
	setlinecolor(BLACK);
	setlinestyle(PS_SOLID, 1);
	// 横线
	for(int i=0; i<GO_SIZE; i++) {
		int y = GO_OFFSET + i * GO_GRID;
		line(GO_OFFSET, y, GO_OFFSET + (GO_SIZE-1)*GO_GRID, y);
	}
	// 竖线
	for(int i=0; i<GO_SIZE; i++) {
		int x = GO_OFFSET + i * GO_GRID;
		line(x, GO_OFFSET, x, GO_OFFSET + (GO_SIZE-1)*GO_GRID);
	}
	// 星位点（9个）
	int star[3] = {3,9,15};
	for(int i=0; i<3; i++) {
		for(int j=0; j<3; j++) {
			int sx = GO_OFFSET + star[j] * GO_GRID;
			int sy = GO_OFFSET + star[i] * GO_GRID;
			setfillcolor(BLACK);
			solidcircle(sx, sy, 4);
		}
	}
}

// 绘制所有棋子
void goDrawAllChess() {
	for(int i=0; i<GO_SIZE; i++) {
		for(int j=0; j<GO_SIZE; j++) {
			int x = GO_OFFSET + j * GO_GRID;
			int y = GO_OFFSET + i * GO_GRID;
			if(goBoard[i][j] == 1) {
				setfillcolor(BLACK);
				solidcircle(x, y, GO_RADIUS);
			} else if(goBoard[i][j] == 2) {
				setfillcolor(WHITE);
				solidcircle(x, y, GO_RADIUS);
				setlinecolor(BLACK);
				circle(x, y, GO_RADIUS);
			}
		}
	}
}

// 鼠标坐标转棋盘行列
bool goGetGrid(int mx, int my, int &r, int &c) {
	r = -1;
	c = -1;
	int col = (mx - GO_OFFSET + GO_GRID/2) / GO_GRID;
	int row = (my - GO_OFFSET + GO_GRID/2) / GO_GRID;
	if(col >=0 && col < GO_SIZE && row >=0 && row < GO_SIZE) {
		r = row;
		c = col;
		return goBoard[row][col] == 0;
	}
	return false;
}

// 预览棋子
void goDrawPreview(int x, int y, int player) {
	if(player == 1)
		setfillcolor(RGB(60,60,60));
	else
		setfillcolor(RGB(220,220,220));
	solidcircle(x,y,GO_RADIUS);
	if(player == 2) {
		setlinecolor(RGB(60,60,60));
		circle(x,y,GO_RADIUS);
	}
}

// 计算一块棋的气
int getQi(int r, int c, int color, bool visited[GO_SIZE][GO_SIZE]) {
	if(r<0||r>=GO_SIZE||c<0||c>=GO_SIZE) return 0;
	if(visited[r][c]) return 0;
	if(goBoard[r][c] == 0) return 1;
	if(goBoard[r][c] != color) return 0;

	visited[r][c] = true;
	int qi = 0;
	qi += getQi(r-1,c,color,visited);
	qi += getQi(r+1,c,color,visited);
	qi += getQi(r,c-1,color,visited);
	qi += getQi(r,c+1,color,visited);
	return qi;
}

// 移除无气棋子
// 移除无气棋子，同时返回被清除棋子坐标列表
void removeDead(int color, vector<pair<int, int>> &delList) {
	delList.clear();
	bool vis[GO_SIZE][GO_SIZE] = {false};
	for(int i=0; i<GO_SIZE; i++) {
		for(int j=0; j<GO_SIZE; j++) {
			if(goBoard[i][j] == color && !vis[i][j]) {
				bool tmpVis[GO_SIZE][GO_SIZE] = {false};
				int qi = getQi(i,j,color,tmpVis);
				if(qi == 0) {
					// 收集整块死棋
					for(int x=0; x<GO_SIZE; x++)
						for(int y=0; y<GO_SIZE; y++)
							if(tmpVis[x][y]) {
								goBoard[x][y] = 0;
								delList.emplace_back(x, y);
							}
				}
			}
		}
	}
}

// 判断落子是否为禁着点
bool isForbidden(int r, int c, int color) {
	goBoard[r][c] = color;
	bool vis[GO_SIZE][GO_SIZE] = {false};
	int selfQi = getQi(r,c,color,vis);
	// 先提对方棋子
	int enemy = (color == 1) ? 2 : 1;
	vector<pair<int, int>> tempDel;
	removeDead(enemy, tempDel);
	// 重新计算自身气
	bool vis2[GO_SIZE][GO_SIZE] = {false};
	int newQi = getQi(r,c,color,vis2);
	// 恢复棋盘
	goBoard[r][c] = 0;
	// 下进去无气且不能提对方 = 禁着点
	if(selfQi == 0 && newQi == 0)
		return true;
	return false;
}
// 终局计算双方总地盘（棋子+空点）
static void calcScore(int &blackTotal, int &whiteTotal) {
	blackTotal = 0;
	whiteTotal = 0;
	bool vis[GO_SIZE][GO_SIZE] = {false};
	// 1. 统计棋盘上所有己方棋子
	for(int r=0; r<GO_SIZE; r++) {
		for(int c=0; c<GO_SIZE; c++) {
			if(goBoard[r][c] == 1) blackTotal++;
			if(goBoard[r][c] == 2) whiteTotal++;
			if(vis[r][c] || goBoard[r][c] != 0) continue;
			// 空白区域BFS，判断归属
			int owner = bfsTerritory(r,c,vis);
			if(owner == 1) blackTotal++;
			else if(owner == 2) whiteTotal++;
		}
	}
	// 加上提子数量（提对方一子等于多占1点）
	blackTotal += captureWhite;
	whiteTotal += captureBlack;
}
// 对局结束后计算贴子、判定胜负
static void judgeWinner() {
	calcScore(blackTotal, whiteTotal);
	// 中国规则：棋盘中点180.5，黑贴3.75子
	double blackReal = blackTotal - 3.75;
	double whiteReal = whiteTotal;
	double mid = 180.5;
	if(blackReal > mid)
		winner = 1;
	else if(whiteReal > mid)
		winner = 2;
	else
		winner = 0; // 和棋（极少出现）
}

// 围棋主窗口循环，对外接口
// 围棋对外入口函数（完整干净版本）
void runGo() {
	resetGoGame();
	int winW = GO_OFFSET*2 + GO_SIZE*GO_GRID;
	int winH = GO_OFFSET*2 + GO_SIZE*GO_GRID + 60;
	initgraph(winW, winH);
	HWND hwnd = GetHWnd(); // EasyX获取窗口句柄
	SetWindowText(hwnd, "围棋");
	HICON hIcon = (HICON)LoadImage(NULL, "Icons/go.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	MOUSEMSG m;
	char text[128];
	int r, c;
	while (!backMenu) {
		// ========== 【唯一一套双缓冲，整帧只画一次】==========
		BeginBatchDraw();
		goDrawBg();
		goDrawBoard();
		goDrawAllChess();

		// 计算鼠标落点，绘制预览（统一在这渲染，不再额外刷新）
		bool canPut = goGetGrid(m.x, m.y, r, c);
		if (!gameEnd && canPut) {
			int px = GO_OFFSET + c * GO_GRID;
			int py = GO_OFFSET + r * GO_GRID;
			goDrawPreview(px, py, goPlayer);
		}

		drawButton();
		// 底部文字
		settextstyle(18, 0, "宋体");
		setbkmode(TRANSPARENT);
		if (gameEnd) {
			char resStr[200];
			if (winner == 1)
				sprintf(resStr, "对局结束！黑棋胜 黑:%d 白:%d（黑贴3.75）", blackTotal, whiteTotal);
			else if (winner == 2)
				sprintf(resStr, "对局结束！白棋胜 黑:%d 白:%d（黑贴3.75）", blackTotal, whiteTotal);
			else
				sprintf(resStr, "对局结束！双方和棋 黑:%d 白:%d", blackTotal, whiteTotal);
			settextcolor(RED);
			outtextxy(10, winH - 50, resStr);
			outtextxy(10, winH - 28, "点击棋盘任意位置重新开局");
		} else {
			if (goPlayer == 1)
				settextcolor(BLACK);
			else
				settextcolor(WHITE);
			sprintf(text, "当前：%s棋 | 点击落子 | 右键跳过回合", goPlayer == 1 ? "黑" : "白");
			outtextxy(10, winH - 30, text);
		}
		EndBatchDraw(); // 整帧绘制完毕一次性刷新屏幕，无闪烁

		// ========== 鼠标消息处理（只读取一次鼠标）==========
		m = GetMouseMsg();
// 对局结束逻辑保持原样
		if(gameEnd) {
			if(m.uMsg == WM_LBUTTONDOWN) resetGoGame();
			continue;
		}

// 仅左键点击时，才做按钮命中校验
		int btnId = 0;
		if (m.uMsg == WM_LBUTTONDOWN) {
			btnId = checkButtonClick(m.x, m.y);
			// Pass按钮
			if(btnId == 1) {
				passCount++;
				if(passCount >= 2) {
					gameEnd = true;
					judgeWinner();
				}
				goPlayer = goPlayer == 1 ? 2 : 1;
				continue;
			}
			// 悔棋按钮
			else if(btnId == 2) {
				undoStep();
				continue;
			}
			// 返回主菜单按钮
			else if(btnId == 3) {
				backMenu = true;
				break;
			}
		}

		// 2. 右键 = 跳过回合
		if (m.uMsg == WM_RBUTTONDOWN) {
			passCount++;
			if (passCount >= 2) {
				gameEnd = true;
				judgeWinner();
			}
			goPlayer = goPlayer == 1 ? 2 : 1;
			continue;
		}

		// 3. 左键落子
		if (m.uMsg == WM_LBUTTONDOWN && canPut) {
			if (isForbidden(r, c, goPlayer))
				continue;

			Step newStep;
			newStep.r = r;
			newStep.c = c;
			newStep.color = goPlayer;
			goBoard[r][c] = goPlayer;

			int enemy = goPlayer == 1 ? 2 : 1;
			vector<pair<int, int>> del;
			removeDead(enemy, del);
			newStep.captureList = del;

			if (enemy == 2)
				captureWhite += del.size();
			else
				captureBlack += del.size();

			removeDead(goPlayer, del);
			stepStack.push_back(newStep);

			goPlayer = goPlayer == 1 ? 2 : 1;
			passCount = 0;
		}
	}
	closegraph();
}
