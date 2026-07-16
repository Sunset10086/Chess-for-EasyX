#include <graphics.h>
#include <cstring>
#include <cstdio>
#include <cmath>

// 棋盘视觉真实列（画面不变，依旧15格）
#define COLS_REAL 15
// 逻辑计算总列：0(左球门) ~ 16(右球门) 合计17列
#define COLS_LOGIC 17
#define ROWS 11
#define BLOCK 45
#define OFFSET_X 60
#define OFFSET_Y 80
#define _USE_MATH_DEFINES
// 一键布阵按钮坐标
#define AUTO_RED_BTN_X  OFFSET_X + 0.3 * COLS_LOGIC * BLOCK
#define AUTO_RED_BTN_Y  OFFSET_Y + ROWS * BLOCK + 100
#define AUTO_BTN_W  140
#define AUTO_BTN_H  40
#define AUTO_BLUE_BTN_X AUTO_RED_BTN_X
#define AUTO_BLUE_BTN_Y AUTO_RED_BTN_Y

// 真实列转逻辑列：真实0 → 逻辑1
#define REAL_TO_LOGIC_C(realC) ((realC) + 1)
// 逻辑列转绘制X坐标
#define LOGIC_C_TO_X(logicC) ( \
                               ((logicC) == 0) ? (OFFSET_X - BLOCK / 2) : \
                               ((logicC) == 16) ? (OFFSET_X + COLS_REAL * BLOCK + BLOCK / 2) : \
                               (OFFSET_X + ((logicC)-1) * BLOCK + BLOCK / 2) \
                             )

// 棋子编号定义
#define GOALKEEPER 1
#define LIBACK 2
#define BACK1 3
#define BACK2 4
#define BACK3 5
#define BACK4 6
#define MID1 7
#define MID2 8
#define FOR1 9
#define CENTER 10
#define FOR2 11
#define BALL -1

// 棋盘数组：只存真实15列格子，0=空，正数红，100+蓝，-1球
static int board[ROWS][COLS_REAL] = {0};
static int selR = -1, selC = -1;
static bool validGrid[ROWS][COLS_REAL] = {false};
static int hoverR = -1, hoverC = -1;
static int turn = 0; // 0红 1蓝
static bool gameOver = false;
static int winSide = -1;
static char rowChar[11] = {'A','B','C','D','E','F','G','H','I','J','K'};

// 摆阵模式标记
static bool layoutMode = true;
static int redStock[11] = {1,2,3,4,5,6,7,8,9,10,11};
static int blueStock[11] = {101,102,103,104,105,106,107,108,109,110,111};
static int redStockCnt = 11;
static int blueStockCnt = 11;
static int stockSel = -1;
static int layoutTurn = 0;
static char layoutTip[128] = "";

static bool holdBall = false;
static int holdPiece = -1;
// 球门可达标记：E/F/G行索引0/1/2
static bool leftGoalReachable[3] = {false};
static bool rightGoalReachable[3] = {false};
// 鼠标悬浮球门：-1=不在球门，0/1/2对应E/F/G
static int hoverGoalIdx = -1;
static int hoverGoalSide = -1; // 0右球门 1左球门
static bool drawGoalBall = false;
static int goalBallR = 0;
static int goalLogicC = 0;
static int originR = -1; // 棋子最原始行
static int originC = -1; // 棋子最原始列
static bool firstTouchBall = true; // true=第一次碰球可取消
static bool inPassChain = false; // true=正在连续传球，禁止取消
bool layoutOk = false;
int redTitleY = OFFSET_Y - 20;
int redStartY = redTitleY + 60;
int blueTitleY = OFFSET_Y + 280;
int blueStartY = blueTitleY + 60;
int colGap = 60;
int rowGap = 65;

// 格式：行tr(A=0,B=1,C=2,D=3,E=4,F=5,G=6,H=7,I=8,J=9,K=10)
// 列tc 1~15对应0~14
struct Pos {
	int r;
	int c;
	int num;
};
Pos preset[] = {
	{0,5,11},  // A7 11
	{2,3,2},   // C5 2
	{3,5,8},   // D7 8
	{4,3,5},   // E5 5
	{5,1,1},   // F3 1
	{5,4,3},   // F6 3
	{6,3,4},   // G5 4
	{6,6,10},  // G8 10
	{8,3,6},   // I5 6
	{8,5,7},   // I7 7
	{10,4,9}   // K6 9
};
Pos bluePreset[] = {
	{0,8,11},   // A10 蓝11 → 111
	{2,9,8},    // C10 蓝8 → 108
	{2,11,2},   // C13 蓝2 → 102
	{4,11,6},   // E13 蓝6 → 106
	{5,8,10},   // F10 蓝10 → 110
	{5,10,3},   // F12 蓝3 → 103
	{5,13,1},   // F15 蓝1 → 101
	{6,11,5},   // G13 蓝5 → 105
	{7,9,7},    // H10 蓝7 → 107
	{8,11,4},   // I13 蓝4 → 104
	{10,9,9}    // K10 蓝9 → 109
};
// 单步棋局快照结构体
struct GameState {
	int board[ROWS][COLS_REAL];
	int selR, selC;
	int originR, originC;
	int turn;
	bool holdBall;
	int holdPiece;
	bool firstTouchBall;
	bool validGrid[ROWS][COLS_REAL];
	bool leftGoalReachable[3];
	bool rightGoalReachable[3];
};
#define MAX_STEP 200 // 最多存200步
static GameState history[MAX_STEP];
static int stepPtr = 0; // 当前存档步数指针

// ====================== 内部工具函数 ======================
static bool isBigArea(int r, int c) {
	int logicC = REAL_TO_LOGIC_C(c);
	if (logicC >= 1 && logicC <= 3)
		return (r >= 2 && r <= 8);
	else if (logicC >= 13 && logicC <= 15)
		return (r >= 2 && r <= 8);
	return false;
}
// 参数：r,c 坐标；side 0红/1蓝
static bool IsOwnBigArea(int r, int c, int side) {
	int logicC = REAL_TO_LOGIC_C(c);
	int inRange = (r >= 2 && r <= 8);
	if (side == 0) // 红方自家：左侧大禁区 logic1~3
		return inRange && (logicC >= 1 && logicC <= 3);
	else // 蓝方自家：右侧大禁区 logic13~15
		return inRange && (logicC >= 13 && logicC <= 15);
}
// 判断逻辑坐标是否为球门
static bool isGoalLogic(int r, int logicC, int side) {
	if (side == 0) {
		// 红进攻 逻辑16右球门
		return logicC == 16 && (r >= 4 && r <= 6);
	} else {
		// 蓝进攻 逻辑0左球门
		return logicC == 0 && (r >= 4 && r <= 6);
	}
}
static int getSide(int r, int c) {
	// 先判断行列是否越界，越界直接返回无棋子
	if (r < 0 || r >= ROWS || c < 0 || c >= COLS_REAL)
		return -1;
	int val = board[r][c];
	if (val >= 1 && val <= 11) return 0;
	if (val >= 101 && val <= 111) return 1;
	return -1;
}
static bool CheckLayoutRule(int r, int c, int selPiece) {

	if (c == 7)
		return false;

	// 规则1：门将只能在大禁区(c0~2 r2~8)，普通棋子无禁区限制
	bool isGK = (selPiece == 1 || selPiece == 101);
	bool inBig1 = (c >= 0 && c <= 2) && (r >= 2 && r <= 8);
	bool inBig2 = (c >= COLS_REAL - 3 && c < COLS_REAL) && (r >= 2 && r <= 8);
	if (layoutTurn == 0 && isGK && !inBig1)
		return false;
	if (layoutTurn != 0 && isGK && !inBig2)
		return false;

	// 规则2：不能和门将同列 / 门将后方(c更小)，同行无所谓
	if (isGK) {
		for (int tr = 0; tr < ROWS; tr++) {
			for (int tc = 0; tc < COLS_REAL; tc++) {
				int p = board[tr][tc];
				if (selPiece == 1 && p != 0 && tc <= c)
					return false;
				if (selPiece == 101 && p != 0 && tc >= c)
					return false;

			}
		}
	} else {
		if (c == 0 || c == COLS_REAL-1)
			return false;
		for (int tr = 0; tr < ROWS; tr++) {
			for (int tc = 0; tc < COLS_REAL; tc++) {
				int p = board[tr][tc];
				if (p == 1 && c <= tc)    // 同列禁止
					return false;
				if (p == 101 && c >= tc)    // 同列禁止
					return false;

			}
		}
	}

	// 规则3：当前列最多7枚棋子（统计本列已存在棋子）
	int colCnt = 0;
	for (int tr = 0; tr < ROWS; tr++) {
		if (board[tr][c] != 0 && board[tr][c] != BALL)
			colCnt++;
	}
	if (colCnt >= 7)
		return false;

	return true;
}
// 普通棋子移动：禁止走到逻辑0、16球门列
static bool canMove(int sr, int sc, int tr, int tc) {
	int logicTc = REAL_TO_LOGIC_C(tc);
	// 棋子禁止进入逻辑0/16球门列
	if (logicTc == 0 || logicTc == 16)
		return false;

	if (tr < 0 || tr >= ROWS || tc < 0 || tc >= COLS_REAL)
		return false;

	int srcSide = getSide(sr, sc);
	int tarSide = getSide(tr, tc);
	// 目标有己方棋子，禁止移动
	if (tarSide == srcSide)
		return false;
	// 目标有敌方棋子（非足球），禁止移动
	if (tarSide != -1 && board[tr][tc] != BALL)
		return false;

	int piece = board[sr][sc];
	int dr = tr - sr;
	int dc = tc - sc;
	int absDr = abs(dr);
	int absDc = abs(dc);
	bool srcInBig = isBigArea(sr, sc);
	bool tarInBig = isBigArea(tr, tc);

	// 门将：仅双方各自大区内可无限制斜线，敌方禁区只能一格
	if (piece == GOALKEEPER || piece == 101) {
		int srcSide = getSide(sr, sc);
		int absDr = abs(dr);
		int absDc = abs(dc);
		// 允许移动类型：纯横、纯竖、纯斜线
		bool isLine = (dr == 0 || dc == 0 || absDr == absDc);
		if (!isLine)
			return false;

		bool srcOwn = IsOwnBigArea(sr, sc, srcSide);
		bool tarOwn = IsOwnBigArea(tr, tc, srcSide);
		// 两端不全在己方大禁区 → 只能一格斜线，且不能长横竖
		if (!(srcOwn && tarOwn)) {
			// 不在自家大区，仅允许一格斜线；长横竖直接禁止
			if ((absDr > 1 || absDc > 1) || (dr > 1 || dc > 1))
				return false;
		}

		// 路径阻挡检测
		int stepR = dr == 0 ? 0 : dr / absDr;
		int stepC = dc == 0 ? 0 : dc / absDc;
		int nr = sr + stepR, nc = sc + stepC;
		while (nr != tr || nc != tc) {
			if (board[nr][nc] != 0 && board[nr][nc] == BALL)
				return false;
			nr += stepR;
			nc += stepC;
		}
		return true;
	}

	// 自由后卫（2/102）：上下左右斜向一格
	if (piece == LIBACK || piece == 102) {
		if (absDr <= 1 && absDc <= 1 && !(dr == 0 && dc == 0))
			return true;
		return false;
	}

	// 后卫（3/4/5/6 / 103~106）：仅横竖，一格距离
	if ((piece >= BACK1 && piece <= BACK4) || (piece >= 103 && piece <= 106)) {
		if (dr != 0 && dc != 0)
			return false;
		if (absDr > 1 || absDc > 1)
			return false;
		return true;
	}

	// 前卫（7/8 / 107/108）：斜线直线，无距离限制，不能穿棋子
	if ((piece == MID1 || piece == MID2) || (piece == 107 || piece == 108)) {
		if (absDr != absDc)
			return false;
		int stepR = dr == 0 ? 0 : dr / abs(dr);
		int stepC = dc == 0 ? 0 : dc / abs(dc);
		int nr = sr + stepR, nc = sc + stepC;
		while (nr != tr || nc != tc) {
			if (board[nr][nc] != 0 || board[nr][nc] == BALL)
				return false;
			nr += stepR;
			nc += stepC;
		}
		return true;
	}

	// 前锋（9/11 / 109/111）：横竖直线，无距离限制，不能穿棋子
	if ((piece == FOR1 || piece == FOR2) || (piece == 109 || piece == 111)) {
		if (dr != 0 && dc != 0)
			return false;
		int stepR = dr == 0 ? 0 : dr / absDr;
		int stepC = dc == 0 ? 0 : dc / absDc;
		int nr = sr + stepR, nc = sc + stepC;
		while (nr != tr || nc != tc) {
			if (board[nr][nc] != 0 || board[nr][nc] == BALL)
				return false;
			nr += stepR;
			nc += stepC;
		}
		return true;
	}

	// 中锋（10/110）：横竖/斜线直线，无距离限制，不能穿棋子
	if (piece == CENTER || piece == 110) {
		if (absDr != absDc && dr != 0 && dc != 0)
			return false;
		int stepR = dr == 0 ? 0 : dr / absDr;
		int stepC = dc == 0 ? 0 : dc / absDc;
		int nr = sr + stepR, nc = sc + stepC;
		while (nr != tr || nc != tc) {
			if (board[nr][nc] != 0 || board[nr][nc] == BALL)
				return false;
			nr += stepR;
			nc += stepC;
		}
		return true;
	}

	// 所有不匹配类型禁止移动
	return false;
}
// pr,pc：棋子坐标；返回true=该棋子开局可以一步走到足球持球
static bool CanTouchBall(int pr, int pc) {
	const int ballR = 5;
	const int ballC = 7;
	// 自己就是球，直接排除
	if (pr == ballR && pc == ballC)
		return false;
	// 调用现成移动规则：判断该棋子能否一步移动到球的位置
	return canMove(pr, pc, ballR, ballC);
}
// 足球在真实棋盘内移动判定（不含球门）
static bool canPassBall(int sr, int sc, int tr, int tc) {
	// 越界拦截
	if (tr < 0 || tr >= ROWS || tc < 0 || tc >= COLS_REAL)
		return false;

	int target = board[tr][tc];
	int side = turn; // 当前进攻方
	// 敌方棋子：禁止传球落点
	if (target != 0 && getSide(tr, tc) != side)
		return false;
	// 己方棋子 / 空地：允许
	int dr = tr - sr;
	int dc = tc - sc;
	int absDr = abs(dr);
	int absDc = abs(dc);
	// 仅允许横/竖/纯斜线
	if (absDr != absDc && dr != 0 && dc != 0)
		return false;
	int stepR = 0;
	if (dr != 0) stepR = dr / absDr;
	int stepC = 0;
	if (dc != 0) stepC = dc / absDc;
	int nr = sr + stepR;
	int nc = sc + stepC;
	while (nr != tr || nc != tc) {
		// 路径中间不能有任何棋子（敌我都挡路）
		if (board[nr][nc] != 0)
			return false;
		nr += stepR;
		nc += stepC;
	}
	return true;
}

static bool IsLeftHalf(int c) {
	return c <= 7;
}
static bool IsRightHalf(int c) {
	return c >= 8;
}

static void drawBoard() {
	cleardevice();
	setfillcolor(RGB(120,120,120));
	solidrectangle(0, 0, getwidth(), getheight());
	// 绘制15列真实棋盘（画面完全不变）
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS_REAL; c++) {
			int x = OFFSET_X + c * BLOCK;
			int y = OFFSET_Y + r * BLOCK;
			if ((r + c) % 2 == 0)
				setfillcolor(RGB(40, 120, 40));
			else
				setfillcolor(RGB(100, 180, 100));
			solidrectangle(x, y, x + BLOCK, y + BLOCK);
			setlinecolor(RGB(180, 180, 180));
			setlinestyle(PS_SOLID, 1);
			rectangle(x, y, x + BLOCK, y + BLOCK);
		}
	}
	setlinecolor(WHITE);
	setlinestyle(PS_SOLID, 2);
	int midColReal = 7;
	int midX = OFFSET_X + midColReal * BLOCK + BLOCK / 2;
	int topY = OFFSET_Y;
	int bottomY = OFFSET_Y + ROWS * BLOCK;
	line(midX, topY, midX, bottomY);
	circle(midX, OFFSET_Y + ROWS * BLOCK / 2, BLOCK * 3 / 2);

	// 左侧大小禁区 真实c0~2
	int leftPenaltyX1 = OFFSET_X;
	int leftPenaltyX2 = OFFSET_X + 3 * BLOCK;
	int leftPenaltyY1 = OFFSET_Y + 2 * BLOCK;
	int leftPenaltyY2 = OFFSET_Y + 9 * BLOCK;
	rectangle(leftPenaltyX1, leftPenaltyY1, leftPenaltyX2, leftPenaltyY2);
	int leftGoalAreaX1 = OFFSET_X;
	int leftGoalAreaX2 = OFFSET_X + BLOCK;
	int leftGoalAreaY1 = OFFSET_Y + 4 * BLOCK;
	int leftGoalAreaY2 = OFFSET_Y + 7 * BLOCK;
	rectangle(leftGoalAreaX1, leftGoalAreaY1, leftGoalAreaX2, leftGoalAreaY2);
	// 外侧左球门深色框
	int leftGoalX1 = OFFSET_X - BLOCK;
	int leftGoalX2 = OFFSET_X;
	int leftGoalY1 = OFFSET_Y + 4 * BLOCK;
	int leftGoalY2 = OFFSET_Y + 7 * BLOCK;
	setfillcolor(RGB(25, 90, 25));
	solidrectangle(leftGoalX1 + 2, leftGoalY1 + 2, leftGoalX2 - 2, leftGoalY2 - 2);
	setlinecolor(WHITE);
	rectangle(leftGoalX1, leftGoalY1, leftGoalX2, leftGoalY2);

	// 右侧大小禁区 真实c12~14
	int rightPenaltyX1 = OFFSET_X + 12 * BLOCK;
	int rightPenaltyX2 = OFFSET_X + 15 * BLOCK;
	int rightPenaltyY1 = OFFSET_Y + 2 * BLOCK;
	int rightPenaltyY2 = OFFSET_Y + 9 * BLOCK;
	rectangle(rightPenaltyX1, rightPenaltyY1, rightPenaltyX2, rightPenaltyY2);
	int rightGoalAreaX1 = OFFSET_X + 14 * BLOCK;
	int rightGoalAreaX2 = OFFSET_X + 15 * BLOCK;
	int rightGoalAreaY1 = OFFSET_Y + 4 * BLOCK;
	int rightGoalAreaY2 = OFFSET_Y + 7 * BLOCK;
	rectangle(rightGoalAreaX1, rightGoalAreaY1, rightGoalAreaX2, rightGoalAreaY2);
	// 外侧右球门深色框
	int rightGoalX1 = OFFSET_X + 15 * BLOCK;
	int rightGoalX2 = OFFSET_X + 16 * BLOCK;
	int rightGoalY1 = OFFSET_Y + 4 * BLOCK;
	int rightGoalY2 = OFFSET_Y + 7 * BLOCK;
	setfillcolor(RGB(25, 90, 25));
	solidrectangle(rightGoalX1 + 2, rightGoalY1 + 2, rightGoalX2 - 2, rightGoalY2 - 2);
	setlinecolor(WHITE);
	rectangle(rightGoalX1, rightGoalY1, rightGoalX2, rightGoalY2);

// 统一线宽，白色场地线条
	setlinecolor(WHITE);
	setlinestyle(PS_SOLID, 2);
	int halfBlock = BLOCK / 2;
	int arcPenaltyR = sqrt(2) * BLOCK * 3 / 2;
	rectangle(OFFSET_X, OFFSET_Y, OFFSET_X + 15 * BLOCK, OFFSET_Y + 11 * BLOCK);
	arc(OFFSET_X - 0.5 * BLOCK, OFFSET_Y - 0.5 * BLOCK, OFFSET_X + 0.5 * BLOCK, OFFSET_Y + 0.5 * BLOCK, 1.5*3.14, 2*3.14);
	arc(OFFSET_X + 14.5 * BLOCK, OFFSET_Y - 0.5 * BLOCK, OFFSET_X + 15.5 * BLOCK, OFFSET_Y + 0.5 * BLOCK, 3.14, 1.5*3.14);
	arc(OFFSET_X - 0.5 * BLOCK, OFFSET_Y + 10.5 * BLOCK, OFFSET_X + 0.5 * BLOCK, OFFSET_Y + 11.5 * BLOCK, 0, 0.5*3.14);
	arc(OFFSET_X + 14.5 * BLOCK, OFFSET_Y + 10.5 * BLOCK, OFFSET_X + 15.5 * BLOCK, OFFSET_Y + 11.5 * BLOCK, 0.5*3.14, 3.14);
	arc(OFFSET_X + 2 * BLOCK, OFFSET_Y + 4 * BLOCK, OFFSET_X + 4 * BLOCK, OFFSET_Y + 7 * BLOCK, 1.5*3.14, 0.5*3.14);
	arc(OFFSET_X + 11 * BLOCK, OFFSET_Y + 4 * BLOCK, OFFSET_X + 13 * BLOCK, OFFSET_Y + 7 * BLOCK, 0.5*3.14, 1.5*3.14);

	setlinestyle(PS_SOLID, 1); // 恢复默认细线

	setlinestyle(PS_SOLID, 1); // 恢复默认细线

	setlinestyle(PS_SOLID, 1); // 恢复默认线宽

	setlinestyle(PS_SOLID, 1);
	settextcolor(WHITE);
	settextstyle(20, 0, "微软雅黑");
	setbkmode(TRANSPARENT);

	// 左侧行字母
	for (int r = 0; r < ROWS; r++) {
		int y = OFFSET_Y + r * BLOCK + BLOCK / 2 - 10;
		int x = OFFSET_X - 30;
		char buf[2] = {rowChar[r], '\0'};
		outtextxy(x, y, buf);
	}
	// 底部真实列数字1~15
	for (int c = 0; c < COLS_REAL; c++) {
		int x = OFFSET_X + c * BLOCK + BLOCK / 2 - 8;
		int y = OFFSET_Y + ROWS * BLOCK + 8;
		char buf[3];
		sprintf(buf, "%d", c + 1);
		outtextxy(x, y, buf);
	}

	// 落点预览
	if (holdBall) {
		setlinecolor(WHITE);
		setlinestyle(PS_SOLID, 3);
		// 棋盘内格子预览
		for (int r = 0; r < ROWS; r++) {
			for (int c = 0; c < COLS_REAL; c++) {
				if (validGrid[r][c]) {
					int x = OFFSET_X + c * BLOCK + BLOCK / 2;
					int y = OFFSET_Y + r * BLOCK + BLOCK / 2;
					circle(x, y, BLOCK / 3);
				}
			}
		}
		// 左球门预览圈
		for (int i = 0; i < 3; i++) {
			if (leftGoalReachable[i]) {
				int x = LOGIC_C_TO_X(0);
				int y = OFFSET_Y + (4 + i) * BLOCK + BLOCK / 2;
				circle(x, y, BLOCK / 3);
			}
		}
		// 右球门预览圈
		for (int i = 0; i < 3; i++) {
			if (rightGoalReachable[i]) {
				int x = LOGIC_C_TO_X(16);
				int y = OFFSET_Y + (4 + i) * BLOCK + BLOCK / 2;
				circle(x, y, BLOCK / 3);
			}
		}
		setlinestyle(PS_SOLID, 1);
	} else {
		setfillcolor(RGB(100, 180, 255));
		for (int r = 0; r < ROWS; r++) {
			for (int c = 0; c < COLS_REAL; c++) {
				if (validGrid[r][c]) {
					int x = OFFSET_X + c * BLOCK;
					int y = OFFSET_Y + r * BLOCK;
					solidrectangle(x + 4, y + 4, x + BLOCK - 4, y + BLOCK - 4);
				}
			}
		}
	}

	// 胜利文字
	if (gameOver) {
		settextstyle(40, 0, "微软雅黑");
		setbkmode(TRANSPARENT);
		char msg[64];
		if (winSide == 0) {
			settextcolor(RED);
			sprintf(msg, "红方进球获胜！点击任意处重新开局");
		} else {
			settextcolor(BLUE);
			sprintf(msg, "蓝方进球获胜！点击任意处重新开局");
		}
		outtextxy(OFFSET_X + 100, OFFSET_Y + ROWS * BLOCK + 120, msg);
	}
}

static void drawStockPieces(int winW) {
	if (!layoutMode) return;
	int stockStartX = OFFSET_X + COLS_REAL * BLOCK + 120;
	int cirR = BLOCK / 3;
	settextstyle(24, 0, "微软雅黑");
	setbkmode(TRANSPARENT);

	// ========== 1. 红方库存（整体上移+缩小间距） ==========
	outtextxy(stockStartX, redTitleY, "红方待摆棋子");
	for (int i = 0; i < 11; i++) {
		if (redStock[i] == 0) continue;
		int px = stockStartX + (i % 3) * colGap;
		int py = redStartY + (i / 3) * rowGap;
		if (stockSel == redStock[i]) {
			setlinecolor(YELLOW);
			setlinestyle(PS_SOLID, 3);
			circle(px, py, cirR + 4);
			setlinecolor(BLACK);
		}
		setfillcolor(RGB(200, 0, 0));
		solidcircle(px, py, cirR);
		settextcolor(WHITE);
		char buf[4];
		sprintf(buf, "%d", redStock[i]);
		outtextxy(px - 12, py - 14, buf);
	}

	// ========== 2. 蓝方库存（同步上移、同间距） ==========
	outtextxy(stockStartX, blueTitleY, "蓝方待摆棋子");
	for (int i = 0; i < 11; i++) {
		if (blueStock[i] == 0) continue;
		int px = stockStartX + (i % 3) * colGap;
		int py = blueStartY + (i / 3) * rowGap;
		if (stockSel == blueStock[i]) {
			setlinecolor(YELLOW);
			setlinestyle(PS_SOLID, 3);
			circle(px, py, cirR + 4);
			setlinecolor(BLACK);
		}
		setfillcolor(RGB(30, 30, 180));
		solidcircle(px, py, cirR);
		settextcolor(WHITE);
		char buf[4];
		sprintf(buf, "%d", blueStock[i] - 100);
		outtextxy(px - 12, py - 14, buf);
	}
	// 红方一键布阵：仅layoutTurn==0绘制
	if(layoutTurn == 0) {
		setfillcolor(RED);
		solidrectangle(AUTO_RED_BTN_X, AUTO_RED_BTN_Y, AUTO_RED_BTN_X + AUTO_BTN_W, AUTO_RED_BTN_Y + AUTO_BTN_H);;
		settextcolor(WHITE);
		settextstyle(22,0,"微软雅黑");
		outtextxy(AUTO_RED_BTN_X + 20, AUTO_RED_BTN_Y + 8, _T("红方一键布阵"));
	}
// 蓝方一键布阵：仅layoutTurn==1绘制
	if(layoutTurn == 1) {
		setfillcolor(BLUE);
		solidrectangle(AUTO_BLUE_BTN_X, AUTO_BLUE_BTN_Y, AUTO_BLUE_BTN_X + AUTO_BTN_W, AUTO_BLUE_BTN_Y + AUTO_BTN_H);
		settextcolor(WHITE);
		settextstyle(22,0,"微软雅黑");
		outtextxy(AUTO_BLUE_BTN_X + 20, AUTO_BLUE_BTN_Y + 8, _T("蓝方一键布阵"));
	}
}

static void drawAllPieces() {
	int circleR = BLOCK / 3;
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS_REAL; c++) {
			int p = board[r][c];
			if (p == 0) continue;
			int x = OFFSET_X + c * BLOCK + BLOCK / 2;
			int y = OFFSET_Y + r * BLOCK + BLOCK / 2;
			if (r == selR && c == selC) {
				setlinecolor(YELLOW);
				setlinestyle(PS_SOLID, 3);
				circle(x, y, circleR + 4);
				setlinecolor(BLACK);
				setlinestyle(PS_SOLID, 1);
			}
			if (p == BALL) {
				setfillcolor(WHITE);
				solidcircle(x, y, circleR);
				setlinecolor(BLACK);
				setlinestyle(PS_SOLID, 2);
				circle(x, y, circleR);
				setlinecolor(BLACK);
				setlinestyle(PS_SOLID, 1);
				continue;
			}
			int side = getSide(r, c);
			if (side == 0) setfillcolor(RGB(200, 0, 0));
			else setfillcolor(RGB(30, 30, 180));
			solidcircle(x, y, circleR);
			settextcolor(WHITE);
			settextstyle(24, 0, "微软雅黑");
			setbkmode(TRANSPARENT);
			char buf[4];
			sprintf(buf, "%d", p > 100 ? p - 100 : p);
			outtextxy(x - 12, y - 14, buf);
		}
	}
	if ((stockSel != -1 || selR != -1) && hoverR != -1 && hoverC != -1 && validGrid[hoverR][hoverC]) {
		int x = OFFSET_X + hoverC * BLOCK + BLOCK / 2;
		int y = OFFSET_Y + hoverR * BLOCK + BLOCK / 2;
		setlinecolor(YELLOW);
		setlinestyle(PS_SOLID, 4);
		circle(x, y, BLOCK / 3 + 6);
		setlinestyle(PS_SOLID, 1);
	}
	if (holdBall && hoverGoalIdx != -1) {
		int x;
		if (hoverGoalSide == 0) x = LOGIC_C_TO_X(16);
		else x = LOGIC_C_TO_X(0);
		int y = OFFSET_Y + (4 + hoverGoalIdx) * BLOCK + BLOCK / 2;
		setlinecolor(YELLOW);
		setlinestyle(PS_SOLID, 4);
		circle(x, y, BLOCK / 3 + 6);
		setlinestyle(PS_SOLID, 1);
	}
	// 球门进球白球
	if (drawGoalBall) {
		int x = LOGIC_C_TO_X(goalLogicC);
		int y = OFFSET_Y + goalBallR * BLOCK + BLOCK / 2;
		int cirR = BLOCK / 3;
		setfillcolor(WHITE);
		solidcircle(x, y, cirR);
		setlinecolor(BLACK);
		setlinestyle(PS_SOLID, 2);
		circle(x, y, cirR);
		setlinecolor(BLACK);
		setlinestyle(PS_SOLID, 1);
	}
}

static void drawBtn() {
	settextstyle(22, 0, "微软雅黑");
	setbkmode(TRANSPARENT);
	// 悔棋按钮
	int undoX = OFFSET_X;
	int undoY = OFFSET_Y + ROWS * BLOCK + 40;
	setfillcolor(RGB(242, 198, 48));
	solidrectangle(undoX, undoY, undoX + 100, undoY + 40);
	settextcolor(BLACK);
	outtextxy(undoX + 30, undoY + 10, "悔棋");

	// 返回主菜单按钮
	int menuX = undoX + 110;
	int menuY = undoY;
	setfillcolor(RED);
	solidrectangle(menuX, menuY, menuX + 130, menuY + 40);
	settextcolor(WHITE);
	outtextxy(menuX + 20, menuY + 10, "返回主菜单");

	// 回合提示栏
	int tipX = OFFSET_X + 120 + 140;
	int tipY = OFFSET_Y + ROWS * BLOCK + 40;
	setfillcolor(RGB(80, 80, 80));
	solidrectangle(tipX, tipY, tipX + 280, tipY + 40);
	settextcolor(WHITE);
	if (turn == 0)
		outtextxy(tipX + 10, tipY + 10, "当前回合：红方");
	else
		outtextxy(tipX + 10, tipY + 10, "当前回合：蓝方");

}

static void resetBoard() {
	memset(board, 0, sizeof(board));
	board[5][7] = BALL; // 只保留中间足球
	selR = -1;
	memset(validGrid, 0, sizeof(validGrid));
	memset(leftGoalReachable, 0, sizeof(leftGoalReachable));
	memset(rightGoalReachable, 0, sizeof(rightGoalReachable));
	turn = 0;
	gameOver = false;
	winSide = -1;
	layoutMode = true;   // 开局强制摆阵
	layoutTurn = 0;
	holdBall = false;
	holdPiece = -1;
	int redNum = 1;
	int blueNum = 101;
	redStockCnt = 11;
	blueStockCnt = 11;
	stockSel = -1;
	// 重置库存数组
	int tempR[11] = {1,2,3,4,5,6,7,8,9,10,11};
	int tempB[11] = {101,102,103,104,105,106,107,108,109,110,111};
	memcpy(redStock, tempR, sizeof(redStock));
	memcpy(blueStock, tempB, sizeof(blueStock));
	layoutOk = false;
	originR = -1;
	originC = -1;
	inPassChain = false;
	firstTouchBall = true;
	stepPtr = 0;
	drawGoalBall = false;
	goalBallR = 0;
	goalLogicC = 0;
}
// 保存当前棋局到历史栈
static void saveState() {
	GameState st;
	memcpy(st.board, board, sizeof(board));
	st.selR = selR;
	st.selC = selC;
	st.originR = originR;
	st.originC = originC;
	st.turn = turn;
	st.holdBall = holdBall;
	st.holdPiece = holdPiece;
	st.firstTouchBall = firstTouchBall;
	memcpy(st.validGrid, validGrid, sizeof(validGrid));
	memcpy(st.leftGoalReachable, leftGoalReachable, sizeof(leftGoalReachable));
	memcpy(st.rightGoalReachable, rightGoalReachable, sizeof(rightGoalReachable));

	if (stepPtr < MAX_STEP) {
		history[stepPtr++] = st;
	}
}

// 回退一步
static void undoStep() {
	if (stepPtr <= 0) return;
	stepPtr--;
	GameState st = history[stepPtr];
	memcpy(board, st.board, sizeof(board));
	selR = st.selR;
	selC = st.selC;
	originR = st.originR;
	originC = st.originC;
	turn = st.turn;
	holdBall = st.holdBall;
	holdPiece = st.holdPiece;
	firstTouchBall = st.firstTouchBall;
	memcpy(validGrid, st.validGrid, sizeof(validGrid));
	memcpy(leftGoalReachable, st.leftGoalReachable, sizeof(leftGoalReachable));
	memcpy(rightGoalReachable, st.rightGoalReachable, sizeof(rightGoalReachable));
	// 悔棋清除进球画面、胜利状态
	drawGoalBall = false;
	goalBallR = 0;
	goalLogicC = 0;
	gameOver = false;
	winSide = -1;
}
// 摆阵阶段独立函数
// 返回true：摆阵完成，进入正式对局
// 返回false：用户点击返回主菜单，直接退出
static bool layoutPhase(int winW, int winH) {
	MOUSEMSG m;
	int stockStartX = OFFSET_X + COLS_REAL * BLOCK + 110;
	int hitR = BLOCK / 3 + 10;

	while (true) {
		// ========== 摆阵阶段绘制 ==========
		BeginBatchDraw();
		drawBoard();
		drawAllPieces();
		drawStockPieces(winW); // 仅摆阵阶段绘制右侧库存

		// 摆阵专用底部按钮
		settextstyle(22, 0, "微软雅黑");
		setbkmode(TRANSPARENT);
		// 返回主菜单按钮
		int menuX = OFFSET_X + 110;
		int menuY = OFFSET_Y + ROWS * BLOCK + 40;
		setfillcolor(RED);
		solidrectangle(menuX, menuY, menuX + 130, menuY + 40);
		settextcolor(WHITE);
		outtextxy(menuX + 20, menuY + 10, "返回主菜单");

		// 提示栏
		int tipX = OFFSET_X + 120 + 140;
		int tipY = OFFSET_Y + ROWS * BLOCK + 40;
		setfillcolor(RGB(80, 80, 80));
		solidrectangle(tipX, tipY, tipX + 280, tipY + 40);
		settextcolor(WHITE);
		if (strlen(layoutTip) > 0) {
			outtextxy(tipX + 10, tipY + 10, layoutTip);
		} else {
			if (layoutTurn == 0)
				outtextxy(tipX + 10, tipY + 10, "摆阵阶段：红方摆放左半场");
			else
				outtextxy(tipX + 10, tipY + 10, "摆阵阶段：蓝方摆放右半场");
		}

		// 列阵结束按钮
		int endLayoutX = OFFSET_X + 540;
		int endLayoutY = OFFSET_Y + ROWS * BLOCK + 40;
		setfillcolor(RGB(60, 160, 60));
		solidrectangle(endLayoutX, endLayoutY, endLayoutX + 120, endLayoutY + 40);
		settextcolor(WHITE);
		outtextxy(endLayoutX + 20, endLayoutY + 10, "列阵结束");
		EndBatchDraw();

		// ========== 鼠标事件处理 ==========
		while (MouseHit()) {
			m = GetMouseMsg();
			if (m.uMsg == WM_MOUSEMOVE) {
				int mx = m.x - OFFSET_X;
				int my = m.y - OFFSET_Y;
				hoverC = mx / BLOCK;
				hoverR = my / BLOCK;
				// 右侧库存区域清空棋盘hover
				if (m.x >= stockStartX) {
					hoverR = -1;
					hoverC = -1;
				}
				// 棋盘越界清空hover
				if (hoverR < 0 || hoverR >= ROWS || hoverC < 0 || hoverC >= COLS_REAL) {
					hoverR = -1;
					hoverC = -1;
				}
				continue;
			}
// 一键布阵按钮判定（仅红摆阶段生效）
			if(layoutTurn == 0
			        && m.x >= AUTO_RED_BTN_X && m.x <= AUTO_RED_BTN_X + AUTO_BTN_W
			        && m.y >= AUTO_RED_BTN_Y && m.y <= AUTO_RED_BTN_Y + AUTO_BTN_H) {
				// 1、清空棋盘所有红棋，重置库存
				for(int tr=0; tr<ROWS; tr++)
					for(int tc=0; tc<COLS_REAL; tc++)
						if(board[tr][tc] >=1 && board[tr][tc] <=11)
							board[tr][tc] = 0;
				int tempRed[11] = {1,2,3,4,5,6,7,8,9,10,11};
				memcpy(redStock, tempRed, sizeof(redStock));
				redStockCnt = 11;

				// 2、写入图中预设红棋坐标
				int presetNum = sizeof(preset)/sizeof(Pos);

				// 批量摆放预设棋子到棋盘，并从库存移除
				for(int i=0; i<presetNum; i++) {
					int r = preset[i].r;
					int c = preset[i].c;
					int num = preset[i].num;
					// 棋盘写入棋子
					board[r][c] = num;
					// 库存移除该棋子
					for(int k=0; k<11; k++) {
						if(redStock[k] == num) {
							redStock[k] = 0;
							redStockCnt--;
							break;
						}
					}
				}
				// 清空选中、预览网格
				stockSel = -1;
				memset(validGrid, 0, sizeof(validGrid));
				strcpy(layoutTip, "已自动加载预设红方阵型");
				continue;
			}
// 蓝方一键布阵按钮判定（仅蓝摆阶段生效）
			if(layoutTurn == 1
			        && m.x >= AUTO_BLUE_BTN_X && m.x <= AUTO_BLUE_BTN_X + AUTO_BTN_W
			        && m.y >= AUTO_BLUE_BTN_Y && m.y <= AUTO_BLUE_BTN_Y + AUTO_BTN_H) {
				// 1、清空棋盘所有蓝棋，重置完整蓝库存
				for(int tr=0; tr<ROWS; tr++)
					for(int tc=0; tc<COLS_REAL; tc++)
						if(board[tr][tc] >=101 && board[tr][tc] <=111)
							board[tr][tc] = 0;
				// 重置蓝库存完整数组
				int tempBlue[11] = {101,102,103,104,105,106,107,108,109,110,111};
				memcpy(blueStock, tempBlue, sizeof(blueStock));
				blueStockCnt = 11;

				// 2、截图蓝棋坐标对照表（r行A=0~K=10，c列1~15对应0~14，棋子数字+100）
				int presetNum = sizeof(bluePreset)/sizeof(Pos);

				// 批量摆放蓝棋，同步扣除库存
				for(int i=0; i<presetNum; i++) {
					int r = bluePreset[i].r;
					int c = bluePreset[i].c;
					int num = bluePreset[i].num + 100; // 蓝棋统一+100
					board[r][c] = num;
					// 库存移除对应棋子
					for(int k=0; k<11; k++) {
						if(blueStock[k] == num) {
							blueStock[k] = 0;
							blueStockCnt--;
							break;
						}
					}
				}
				stockSel = -1;
				memset(validGrid, 0, sizeof(validGrid));
				strcpy(layoutTip, "已自动加载预设蓝方阵型");
				continue;
			}
			if (m.uMsg == WM_LBUTTONDOWN) {
				strcpy(layoutTip, ""); // 点击任意位置清空错误提示

				// 1. 返回主菜单按钮
				int menuX = OFFSET_X + 110;
				int menuY = OFFSET_Y + ROWS * BLOCK + 40;
				if (m.x >= menuX && m.x <= menuX + 130 && m.y >= menuY && m.y <= menuY + 40) {
					closegraph();
					return false;
				}

				// 2. 列阵结束按钮
				int endLayoutX = OFFSET_X + 540;
				int endLayoutY = OFFSET_Y + ROWS * BLOCK + 40;
				if (m.x >= endLayoutX && m.x <= endLayoutX + 120 && m.y >= endLayoutY && m.y <= endLayoutY + 40) {
					int blueMidCnt = 0;
					int redMidCnt = 0;
					if (layoutTurn == 0) {
						for(int tr=4; tr<=6; tr++) {
							if(CanTouchBall(tr,6)) redMidCnt++;
						}
						if (redStockCnt != 0) {
							strcpy(layoutTip, "红方棋子尚未全部摆放完成！");
							continue;
						}
						if(redMidCnt < 1) {
							strcpy(layoutTip, "红方中圈至少放置1枚可触球棋子！");
							continue;
						}
						layoutTurn = 1;
						strcpy(layoutTip, "");
						stockSel = -1;
						memset(validGrid, 0, sizeof(validGrid));

					} else {
						for(int tr=4; tr<=6; tr++) {
							if(CanTouchBall(tr,8)) blueMidCnt++;
						}
						if (blueStockCnt != 0) {
							strcpy(layoutTip, "蓝方棋子尚未全部摆放完成！");
							continue;
						}
						if (blueMidCnt < 1) {
							strcpy(layoutTip, "蓝方中圈至少放置1枚可触球棋子！");
							continue;
						}
						layoutMode = false;
						turn = 0;
						strcpy(layoutTip, "");
						stockSel = -1;
						memset(validGrid, 0, sizeof(validGrid));
						// 全部校验通过，退出摆阵函数进入对局
						return true;
					}
				}

				// 3. 点击右侧库存棋子
				if (m.x >= stockStartX) {
					// 红方库存
					if (m.y >= redStartY - 40 && m.y <= redStartY + 4 * 60) {
						for (int i = 0; i < 11; i++) {
							if (redStock[i] == 0) continue;
							if (layoutTurn != 0) continue;
							int px = stockStartX + (i % 3) * colGap;
							int py = redStartY + (i / 3) * rowGap;
							int dx = m.x - px;
							int dy = m.y - py;
							if (dx * dx + dy * dy <= hitR * hitR) {
								stockSel = redStock[i];
								selR = -1;
								selC = -1;
								memset(validGrid, 0, sizeof(validGrid));
								// 左半场空白格可摆放
								for (int tr = 0; tr < ROWS; tr++)
									for (int tc = 0; tc < COLS_REAL; tc++) {
										if (board[tr][tc] != 0) continue;
										if (tr == 5 && tc == 7) continue;
										if (tc <= 7) {
											// 叠加列阵规则校验
											if(CheckLayoutRule(tr, tc, stockSel))
												validGrid[tr][tc] = true;
											else
												validGrid[tr][tc] = false;
										}
									}
								break;
							}
						}
						continue;
					}
					// 蓝方库存
					if (m.y >= blueStartY - 20 && m.y <= blueStartY + 4 * 60) {
						for (int i = 0; i < 11; i++) {
							if (blueStock[i] == 0) continue;
							if (layoutTurn != 1) continue;
							int px = stockStartX + (i % 3) * colGap;
							int py = blueStartY + (i / 3) * rowGap;
							int dx = m.x - px;
							int dy = m.y - py;
							if (dx * dx + dy * dy <= hitR * hitR) {
								stockSel = blueStock[i];
								selR = -1;
								selC = -1;
								memset(validGrid, 0, sizeof(validGrid));
								// 右半场空白格可摆放
								for (int tr = 0; tr < ROWS; tr++)
									for (int tc = 0; tc < COLS_REAL; tc++) {
										if (board[tr][tc] != 0) continue;
										if (tr == 5 && tc == 7) continue;
										if (tc >= 8) {
											if(CheckLayoutRule(tr, tc, stockSel))
												validGrid[tr][tc] = true;
											else
												validGrid[tr][tc] = false;
										}
									}
								break;
							}
						}
						continue;
					}
				}

				// 4. 点击棋盘格子：放置/收回棋子
				int mx = m.x - OFFSET_X;
				int my = m.y - OFFSET_Y;
				int c = mx / BLOCK;
				int r = my / BLOCK;
				if (r < 0 || r >= ROWS || c < 0 || c >= COLS_REAL) continue;
				int cell = board[r][c];

				// 放置选中的库存棋子
				if (stockSel != -1 && cell == 0 && validGrid[r][c]) {
					board[r][c] = stockSel;
					if (stockSel <= 11) {
						for (int i = 0; i < 11; i++) {
							if (redStock[i] == stockSel) {
								redStock[i] = 0;
								redStockCnt--;
								break;
							}
						}
					} else {
						for (int i = 0; i < 11; i++) {
							if (blueStock[i] == stockSel) {
								blueStock[i] = 0;
								blueStockCnt--;
								break;
							}
						}
					}
					stockSel = -1;
					memset(validGrid, 0, sizeof(validGrid));
					continue;
				}

				// 点击棋盘上的己方棋子，收回库存
				if (cell != 0 && cell != BALL) {
					if (cell <= 11 && layoutTurn == 0) {
						for (int i = 0; i < 11; i++) {
							if (redStock[i] == 0) {
								redStock[i] = cell;
								redStockCnt++;
								break;
							}
						}
						board[r][c] = 0;
						stockSel = -1;
						memset(validGrid, 0, sizeof(validGrid));
					} else if (cell > 100 && layoutTurn == 1) {
						for (int i = 0; i < 11; i++) {
							if (blueStock[i] == 0) {
								blueStock[i] = cell;
								blueStockCnt++;
								break;
							}
						}
						board[r][c] = 0;
						stockSel = -1;
						memset(validGrid, 0, sizeof(validGrid));
					}
					continue;
				}
				continue;
			}
		}
	}
}

void runFootball() {
	resetBoard();
	int winW = OFFSET_X * 2 + COLS_REAL * BLOCK + 5 * BLOCK;
	int winH = OFFSET_Y * 2 + ROWS * BLOCK + 100;
	initgraph(winW, winH, 0);
	HWND hwnd = GetHWnd(); // EasyX获取窗口句柄
	SetWindowText(hwnd, "高仕足球棋");
	HICON hIcon = (HICON)LoadImage(NULL, "Icons/football.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	// ========== 正式对局阶段（纯走棋/传球，无任何摆阵逻辑） ==========
	MOUSEMSG m;
	while (true) {
		BeginBatchDraw();
		drawBoard();
		drawAllPieces();
		drawBtn();
		EndBatchDraw();
		if (!layoutOk) {
			layoutOk = layoutPhase(winW, winH);
			if (!layoutOk) return;
		}
		while (MouseHit()) {
			m = GetMouseMsg();
			if (m.uMsg == WM_MOUSEMOVE) {
				int mx = m.x - OFFSET_X;
				int my = m.y - OFFSET_Y;
				hoverC = mx / BLOCK;
				hoverR = my / BLOCK;
				hoverGoalIdx = -1;
				hoverGoalSide = -1;
				// 判断左球门悬浮
				if (mx < 0) {
					int idx = my / BLOCK - 4;
					if (idx >= 0 && idx <= 2) {
						hoverGoalIdx = idx;
						hoverGoalSide = 1;
					}
				}
				// 判断右球门悬浮
				if (mx > COLS_REAL * BLOCK) {
					int idx = my / BLOCK - 4;
					if (idx >= 0 && idx <= 2) {
						hoverGoalIdx = idx;
						hoverGoalSide = 0;
					}
				}
				if (hoverR < 0 || hoverR >= ROWS || hoverC < 0 || hoverC >= COLS_REAL) {
					hoverR = -1;
					hoverC = -1;
				}
				continue;
			}

			if (m.uMsg == WM_LBUTTONDOWN) {
				// 悔棋按钮
				int undoX = OFFSET_X;
				int undoY = OFFSET_Y + ROWS * BLOCK + 40;
				if (m.x >= undoX && m.x <= undoX + 100 && m.y >= undoY && m.y <= undoY + 40) {
					undoStep();
					continue;
				}
				if (gameOver) {
					resetBoard();
					break;
				}
				// 返回主菜单按钮
				int menuX = undoX + 110;
				int menuY = undoY;
				if (m.x >= menuX && m.x <= menuX + 130 && m.y >= menuY && m.y <= menuY + 40) {
					closegraph();
					return;
				}

				int mx = m.x - OFFSET_X;
				int my = m.y - OFFSET_Y;
				int c = mx / BLOCK;
				int r = my / BLOCK;
				if (r < 0 || r >= ROWS || c < 0 || c >= COLS_REAL + 1) continue;
				//if ((c < 0 || c == 16) && (r > 7 || r < 5)) continue;
				int clickSide = getSide(r, c);

				if (holdBall) {
					// 球门射门判断
					int gRowIdx = my / BLOCK - 4;
					if (turn == 0) {
						if (mx > COLS_REAL * BLOCK && gRowIdx >= 0 && gRowIdx <= 2) {
							if (rightGoalReachable[gRowIdx]) {
								saveState(); // 进球前先存当前局面，用于悔撤销进球
								gameOver = true;
								winSide = 0;
								drawGoalBall = true;
								goalBallR = 4 + gRowIdx;
								goalLogicC = 16;
								board[selR][selC] = holdPiece;
								selR = -1;
								holdBall = false;
								holdPiece = -1;
								firstTouchBall = true;
								memset(validGrid, 0, sizeof(validGrid));
								memset(rightGoalReachable, 0, sizeof(rightGoalReachable));
								turn = 1 - turn;
								continue;

							}
							continue;
						}
					}
					if (turn == 1) {
						if (mx < 0 && gRowIdx >= 0 && gRowIdx <= 2) {
							if (leftGoalReachable[gRowIdx]) {
								saveState(); // 进球前先存当前局面，用于悔撤销进球
								gameOver = true;
								winSide = 1;
								drawGoalBall = true;
								goalBallR = 4 + gRowIdx;
								goalLogicC = 0;
								board[selR][selC] = holdPiece;
								selR = -1;
								holdBall = false;
								holdPiece = -1;
								firstTouchBall = true;
								memset(validGrid, 0, sizeof(validGrid));
								memset(leftGoalReachable, 0, sizeof(leftGoalReachable));
								turn = 1 - turn;
								continue;
							}
							continue;
						}
					}

					// 棋盘内传球（支持连续传己方棋子）
					if (r >= 0 && r < ROWS && c >= 0 && c < COLS_REAL && validGrid[r][c]) {
						int targetP = board[r][c];
						board[selR][selC] = holdPiece;
						if (targetP != 0) {
							saveState();
							firstTouchBall = false;
							board[r][c] = BALL;
							holdPiece = targetP;
							selR = r;
							selC = c;
							memset(validGrid, 0, sizeof(validGrid));
							memset(leftGoalReachable, 0, sizeof(leftGoalReachable));
							memset(rightGoalReachable, 0, sizeof(rightGoalReachable));
							for (int tr = 0; tr < ROWS; tr++)
								for (int tc = 0; tc < COLS_REAL; tc++)
									if (canPassBall(selR, selC, tr, tc))
										validGrid[tr][tc] = true;
							// 刷新右球门可达
							if (turn == 0) {
								for (int i = 0; i < 3; i++) {
									int gr = 4 + i;
									int dr = gr - selR;
									int dc = 16 - REAL_TO_LOGIC_C(selC);
									int absDr = abs(dr), absDc = abs(dc);
									bool lineOk = (absDr == absDc || dr == 0 || dc == 0);
									bool block = false;
									if (lineOk) {
										int stepR = dr == 0 ? 0 : dr / absDr;
										int stepC = dc == 0 ? 0 : dc / absDc;
										int nr = selR + stepR, nc = selC + stepC;
										while (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS_REAL) {
											if (board[nr][nc] != 0) {
												block = true;
												break;
											}
											nr += stepR;
											nc += stepC;
										}
									}
									rightGoalReachable[i] = lineOk && !block;
								}
							}
							// 刷新左球门可达
							if (turn == 1) {
								for (int i = 0; i < 3; i++) {
									int gr = 4 + i;
									int dr = gr - selR;
									int dc = 0 - REAL_TO_LOGIC_C(selC);
									int absDr = abs(dr), absDc = abs(dc);
									bool lineOk = (absDr == absDc || dr == 0 || dc == 0);
									bool block = false;
									if (lineOk) {
										int stepR = dr == 0 ? 0 : dr / absDr;
										int stepC = dc == 0 ? 0 : dc / absDc;
										int nr = selR + stepR, nc = selC + stepC;
										while (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS_REAL) {
											if (board[nr][nc] != 0) {
												block = true;
												break;
											}
											nr += stepR;
											nc += stepC;
										}
									}
									leftGoalReachable[i] = lineOk && !block;
								}
							}
							continue;
						} else {
							saveState();
							board[r][c] = BALL;
							selR = -1;
							holdBall = false;
							holdPiece = -1;
							firstTouchBall = true;
							memset(validGrid, 0, sizeof(validGrid));
							memset(leftGoalReachable, 0, sizeof(leftGoalReachable));
							memset(rightGoalReachable, 0, sizeof(rightGoalReachable));
							turn = 1 - turn;
							continue;
						}
					}

					// 初次持球可取消
					if (firstTouchBall && (r != selR || c != selC)) {
						board[originR][originC] = holdPiece;
						board[selR][selC] = BALL;
						selR = -1;
						holdBall = false;
						holdPiece = -1;
						firstTouchBall = true;
						memset(validGrid, 0, sizeof(validGrid));
						memset(leftGoalReachable, 0, sizeof(leftGoalReachable));
						memset(rightGoalReachable, 0, sizeof(rightGoalReachable));
						continue;
					}
					continue;
				}

				// 普通走棋逻辑
				if (!holdBall) {
					if (selR == -1) {
						if (clickSide == turn && board[r][c] != BALL) {
							memset(validGrid, 0, sizeof(validGrid));
							selR = r;
							selC = c;
							originR = r;
							originC = c;
							for (int tr = 0; tr < ROWS; tr++)
								for (int tc = 0; tc < COLS_REAL; tc++)
									if (canMove(r, c, tr, tc))
										validGrid[tr][tc] = true;
						}
					} else {
						if (r == selR && c == selC) {
							selR = -1;
							selC = -1;
							memset(validGrid, 0, sizeof(validGrid));
							continue;
						}
						if (clickSide == turn && board[r][c] != BALL) {
							originR = r;
							originC = c;
							selR = r;
							selC = c;
							memset(validGrid, 0, sizeof(validGrid));
							for (int tr = 0; tr < ROWS; tr++)
								for (int tc = 0; tc < COLS_REAL; tc++)
									if (canMove(r, c, tr, tc))
										validGrid[tr][tc] = true;
							continue;
						}
						if (validGrid[r][c]) {
							int srcP = board[selR][selC];
							saveState();
							if (board[r][c] == BALL) {
								holdPiece = srcP;
								board[r][c] = srcP;
								board[selR][selC] = 0;
								holdBall = true;
								selR = r;
								selC = c;
								firstTouchBall = true;
								memset(validGrid, 0, sizeof(validGrid));
								memset(leftGoalReachable, 0, sizeof(leftGoalReachable));
								memset(rightGoalReachable, 0, sizeof(rightGoalReachable));
								for (int tr = 0; tr < ROWS; tr++)
									for (int tc = 0; tc < COLS_REAL; tc++)
										if (canPassBall(selR, selC, tr, tc))
											validGrid[tr][tc] = true;
								// 右球门可达
								if (turn == 0) {
									for (int i = 0; i < 3; i++) {
										int gr = 4 + i;
										int dr = gr - selR;
										int dc = 16 - REAL_TO_LOGIC_C(selC);
										int absDr = abs(dr), absDc = abs(dc);
										bool lineOk = (absDr == absDc || dr == 0 || dc == 0);
										bool block = false;
										if (lineOk) {
											int stepR = dr == 0 ? 0 : dr / absDr;
											int stepC = dc == 0 ? 0 : dc / absDc;
											int nr = selR + stepR, nc = selC + stepC;
											while (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS_REAL) {
												if (board[nr][nc] != 0) {
													block = true;
													break;
												}
												nr += stepR;
												nc += stepC;
											}
										}
										rightGoalReachable[i] = lineOk && !block;
									}
								}
								// 左球门可达
								if (turn == 1) {
									for (int i = 0; i < 3; i++) {
										int gr = 4 + i;
										int dr = gr - selR;
										int dc = 0 - REAL_TO_LOGIC_C(selC);
										int absDr = abs(dr), absDc = abs(dc);
										bool lineOk = (absDr == absDc || dr == 0 || dc == 0);
										bool block = false;
										if (lineOk) {
											int stepR = dr == 0 ? 0 : dr / absDr;
											int stepC = dc == 0 ? 0 : dc / absDc;
											int nr = selR + stepR, nc = selC + stepC;
											while (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS_REAL) {
												if (board[nr][nc] != 0) {
													block = true;
													break;
												}
												nr += stepR;
												nc += stepC;
											}
										}
										leftGoalReachable[i] = lineOk && !block;
									}
								}
							} else {
								board[r][c] = srcP;
								board[selR][selC] = 0;
								selR = -1;
								selC = -1;
								memset(validGrid, 0, sizeof(validGrid));
								turn = 1 - turn;
							}
						}
					}
				}
			}
		}
	}
	closegraph();
}
