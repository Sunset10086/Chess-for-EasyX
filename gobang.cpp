#include <graphics.h>
#include <iostream>
#include <cstdio>
using namespace std;

#define SIZE 15
#define GRID 40
#define OFFSET 30
#define RADIUS 18

// 按钮坐标
#define BTN_Y  OFFSET + SIZE * GRID
#define BTN_H 35
#define BTN_RET_X1 330
#define BTN_RET_X2 470

// 全局变量仅作用于本文件
static int board[SIZE][SIZE] = {0};
static int player = 1;
static bool gameOver = false;
static int winner = 0;
static bool backToMenu = false;

static void drawBackground();
static void drawBoard();
static void drawAllChess();
static bool getGridPos(int mx, int my, int &row, int &col);
static void drawPreviewChess(int gridX, int gridY, int curPlayer);
static bool isWin(int r, int c, int chess);
static void showWinScreen(int winW, int winH);
static void drawGobangBtn();
static bool isBackBtnClicked(int mx,int my);

void resetGame() {
	for (int i = 0; i < SIZE; i++)
		for (int j = 0; j < SIZE; j++)
			board[i][j] = 0;
	player = 1;
	gameOver = false;
	winner = 0;
	backToMenu = false;
}

// 绘制返回主菜单按钮
void drawGobangBtn() {
	setfillcolor(RGB(235, 75, 75));
	solidrectangle(BTN_RET_X1, BTN_Y, BTN_RET_X2, BTN_Y + BTN_H);
	settextcolor(WHITE);
	settextstyle(16, 0, "宋体");
	setbkmode(TRANSPARENT);
	outtextxy(BTN_RET_X1 + 29, BTN_Y + 8, "返回主菜单");
}

// 判断按钮是否被左键点击
bool isBackBtnClicked(int mx,int my) {
	if(my < BTN_Y || my > BTN_Y + BTN_H) return false;
	return (mx >= BTN_RET_X1 && mx <= BTN_RET_X2);
}

void drawBackground() {
	setfillcolor(RGB(180, 180, 180));
	solidrectangle(0, 0, getwidth(), getheight());
}

void drawBoard() {
	setlinecolor(BLACK);
	setlinestyle(PS_SOLID, 2);
	for (int i = 0; i < SIZE; i++) {
		int y = OFFSET + i * GRID;
		line(OFFSET, y, OFFSET + (SIZE - 1) * GRID, y);
	}
	for (int i = 0; i < SIZE; i++) {
		int x = OFFSET + i * GRID;
		line(x, OFFSET, x, OFFSET + (SIZE - 1) * GRID);
	}
}

void drawAllChess() {
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			int x = OFFSET + j * GRID;
			int y = OFFSET + i * GRID;
			if (board[i][j] == 1) {
				setfillcolor(BLACK);
				solidcircle(x, y, RADIUS);
			} else if (board[i][j] == 2) {
				setfillcolor(WHITE);
				solidcircle(x, y, RADIUS);
				setlinecolor(BLACK);
				circle(x, y, RADIUS);
			}
		}
	}
}

bool getGridPos(int mx, int my, int &row, int &col) {
	row = -1, col = -1;
	int c = (mx - OFFSET + GRID / 2) / GRID;
	int r = (my - OFFSET + GRID / 2) / GRID;
	if (c >= 0 && c < SIZE && r >= 0 && r < SIZE) {
		row = r;
		col = c;
		return board[r][c] == 0;
	}
	return false;
}

void drawPreviewChess(int gridX, int gridY, int curPlayer) {
	if (curPlayer == 1)
		setfillcolor(RGB(80, 80, 80));
	else
		setfillcolor(RGB(230, 230, 230));
	solidcircle(gridX, gridY, RADIUS);
	if (curPlayer == 2) {
		setlinecolor(RGB(80, 80, 80));
		circle(gridX, gridY, RADIUS);
	}
}

bool isWin(int r, int c, int chess) {
	int dir[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
	for (int d = 0; d < 4; d++) {
		int cnt = 1;
		int nr = r + dir[d][0];
		int nc = c + dir[d][1];
		while (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE && board[nr][nc] == chess) {
			cnt++;
			nr += dir[d][0];
			nc += dir[d][1];
		}
		nr = r - dir[d][0];
		nc = c - dir[d][1];
		while (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE && board[nr][nc] == chess) {
			cnt++;
			nr -= dir[d][0];
			nc -= dir[d][1];
		}
		if (cnt >= 5)
			return true;
	}
	return false;
}

void showWinScreen(int winW, int winH) {
	char text[64];
	MOUSEMSG msg;
	while (true) {
		BeginBatchDraw();
		drawBackground();
		settextcolor(RED);
		settextstyle(40, 0, "宋体");
		setbkmode(TRANSPARENT);
		if (winner == 1)
			sprintf(text, "黑棋获胜！");
		else
			sprintf(text, "白棋获胜！");
		outtextxy(winW / 2 - 100, winH / 2 - 40, text);

		settextstyle(22, 0, "宋体");
		setbkmode(TRANSPARENT);
		settextcolor(RGB(0, 40, 180));
		sprintf(text, "鼠标点击窗口任意位置重新开始");
		outtextxy(winW / 2 - 200, winH / 2 + 20, text);
		EndBatchDraw();

		msg = GetMouseMsg();
		if (msg.uMsg == WM_LBUTTONDOWN)
			break;
	}
}

// 对外暴露的唯一接口：启动五子棋窗口
void runGobang() {
	resetGame();
	int winW = OFFSET * 2 + SIZE * GRID;
	int winH = OFFSET * 2 + SIZE * GRID;
	initgraph(winW, winH);
	HWND hwnd = GetHWnd(); // EasyX获取窗口句柄
	SetWindowText(hwnd, "五子棋");
	HICON hIcon = (HICON)LoadImage(NULL, "Icons/gobang.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	MOUSEMSG m;
	char text[64];
	int r, c;

	while (!backToMenu) {
		BeginBatchDraw();
		drawBackground();
		drawBoard();
		drawAllChess();
		drawGobangBtn();

		if (!gameOver) {
			if (player == 1)
				sprintf(text, "当前：黑棋 点击棋盘落子");
			else
				sprintf(text, "当前：白棋 点击棋盘落子");
			settextcolor(RGB(255, 100, 0));
			settextstyle(20, 0, "宋体");
			setbkmode(TRANSPARENT);
			outtextxy(10, winH - 25, text);

			m = GetMouseMsg();
			if(m.uMsg == WM_LBUTTONDOWN && isBackBtnClicked(m.x,m.y)) {
				backToMenu = true;
				break;
			}
			bool canDrop = getGridPos(m.x, m.y, r, c);
			if (canDrop) {
				int preX = OFFSET + c * GRID;
				int preY = OFFSET + r * GRID;
				drawPreviewChess(preX, preY, player);
			}

			EndBatchDraw();

			if (m.uMsg == WM_LBUTTONDOWN && canDrop) {
				board[r][c] = player;
				if (isWin(r, c, player)) {
					gameOver = true;
					winner = player;
					while (MouseHit())
						GetMouseMsg();
				}
				player = (player == 1) ? 2 : 1;
			}
		} else {
			EndBatchDraw();
			showWinScreen(winW, winH);
			resetGame();
		}
	}
	closegraph();
}
